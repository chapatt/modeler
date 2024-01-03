#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "imgui/cimgui.h"
#include "imgui/cimgui_impl_vulkan.h"
#include "imgui/imgui_impl_modeler.h"

#include "utils.h"
#include "vulkan_utils.h"

#include "renderloop.h"

void draw(VkDevice device, VkRenderPass renderPass, VkPipeline pipeline, SwapchainInfo swapchainInfo, VkImageView *imageViews, uint32_t imageViewCount, VkQueue graphicsQueue, VkQueue presentationQueue, uint32_t graphicsQueueFamilyIndex, const char *resourcePath, Queue *inputQueue, ImGui_ImplVulkan_InitInfo imVulkanInitInfo)
{
//
//
//framebuffer creation		line_936 to line_967
//
//create framebuffer
//
	VkFramebufferCreateInfo frame_buff_cre_infos[imageViewCount];
	VkFramebuffer frame_buffs[imageViewCount];
	VkImageView image_attachs[imageViewCount];
	for (uint32_t i = 0; i < imageViewCount; i++) {
		image_attachs[i] = imageViews[i];
		frame_buff_cre_infos[i].sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frame_buff_cre_infos[i].pNext = NULL;
		frame_buff_cre_infos[i].flags = 0;
		frame_buff_cre_infos[i].renderPass = renderPass;
		frame_buff_cre_infos[i].attachmentCount = 1;
		frame_buff_cre_infos[i].pAttachments = &(image_attachs[i]);
		frame_buff_cre_infos[i].width = swapchainInfo.extent.width;
		frame_buff_cre_infos[i].height = swapchainInfo.extent.height;
		frame_buff_cre_infos[i].layers = 1;

		vkCreateFramebuffer(device, &(frame_buff_cre_infos[i]), NULL, &(frame_buffs[i]));
		printf("framebuffer %d created.\n", i);
	}
//
//
//command buffer creation		line_968 to line_1001
//
//create command pool
//
	VkCommandPoolCreateInfo cmd_pool_cre_info;
	cmd_pool_cre_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmd_pool_cre_info.pNext = NULL;
	cmd_pool_cre_info.flags = 0;
	cmd_pool_cre_info.queueFamilyIndex = graphicsQueueFamilyIndex;

	VkCommandPool cmd_pool;
	vkCreateCommandPool(device, &cmd_pool_cre_info, NULL, &cmd_pool);
	printf("command pool created.\n");
//
//allocate command buffers
//
	VkCommandBufferAllocateInfo cmd_buff_alloc_info;
	cmd_buff_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmd_buff_alloc_info.pNext = NULL;
	cmd_buff_alloc_info.commandPool = cmd_pool;
	cmd_buff_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmd_buff_alloc_info.commandBufferCount = imageViewCount;

	VkCommandBuffer cmd_buffers[imageViewCount];
	vkAllocateCommandBuffers(device, &cmd_buff_alloc_info, cmd_buffers);
	printf("command buffers allocated.\n");
//
//
//imgui init
//
	cImGui_ImplVulkan_Init(&imVulkanInitInfo, renderPass);
//
//
//render preparation		line1002 to line1062
//
	VkCommandBufferBeginInfo cmd_buff_begin_infos[imageViewCount];
	VkRenderPassBeginInfo rendp_begin_infos[imageViewCount];
	VkRect2D rendp_area;
	rendp_area.offset.x = 0;
	rendp_area.offset.y = 0;
	rendp_area.extent = swapchainInfo.extent;
	VkClearValue clear_val = {0.1f, 0.3f, 0.3f, 1.0f};
//
//
//semaphores and fences creation part		line_1063 to line_1103
//
	uint32_t max_frames = 2;
	VkSemaphore semps_img_avl[max_frames];
	VkSemaphore semps_rend_fin[max_frames];
	VkFence fens[max_frames];

	VkSemaphoreCreateInfo semp_cre_info;
	semp_cre_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semp_cre_info.pNext = NULL;
	semp_cre_info.flags = 0;

	VkFenceCreateInfo fen_cre_info;
	fen_cre_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fen_cre_info.pNext = NULL;
	fen_cre_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (uint32_t i = 0; i < max_frames; i++) {
		vkCreateSemaphore(device, &semp_cre_info, NULL, &(semps_img_avl[i]));
		vkCreateSemaphore(device, &semp_cre_info, NULL, &(semps_rend_fin[i]));
		vkCreateFence(device, &fen_cre_info, NULL, &(fens[i]));
	}
	printf("semaphores and fences created.\n");

	uint32_t cur_frame = 0;
	VkFence fens_img[imageViewCount];
	for (uint32_t i = 0; i < imageViewCount; i++) {
		fens_img[i] = VK_NULL_HANDLE;
	}
//
//
//main present part		line_1104 to line_1197
//
	printf("\n");
	for (;;) {
		InputEvent *inputEvent;
		while (dequeue(inputQueue, (void **) &inputEvent)) {
			InputEventType type = inputEvent->type;
			void *data = inputEvent->data;

			switch(type) {
			case POINTER_LEAVE: case BUTTON_DOWN: case BUTTON_UP:
				ImGui_ImplModeler_HandleInput(inputEvent);
				free(inputEvent);
				break;
			case POINTER_MOVE:
				ImGui_ImplModeler_HandleInput(inputEvent);
				free(data);
				free(inputEvent);
				break;
			case TERMINATE:
				free(inputEvent);
				goto cancelMainLoop;
			}
		}


//
//submit
//
		vkWaitForFences(device, 1, &(fens[cur_frame]), VK_TRUE, UINT64_MAX);

		uint32_t img_index = 0;
		vkAcquireNextImageKHR(device, swapchainInfo.swapchain, UINT64_MAX, semps_img_avl[cur_frame], VK_NULL_HANDLE, &img_index);

		/* render */
		uint32_t i = img_index;
		cmd_buff_begin_infos[i].sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmd_buff_begin_infos[i].pNext = NULL;
		cmd_buff_begin_infos[i].flags = 0;
		cmd_buff_begin_infos[i].pInheritanceInfo = NULL;

		rendp_begin_infos[i].sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rendp_begin_infos[i].pNext = NULL;
		rendp_begin_infos[i].renderPass = renderPass;
		rendp_begin_infos[i].framebuffer = frame_buffs[i];
		rendp_begin_infos[i].renderArea = rendp_area;
		rendp_begin_infos[i].clearValueCount = 1;
		rendp_begin_infos[i].pClearValues = &clear_val;

		vkBeginCommandBuffer(cmd_buffers[i], &cmd_buff_begin_infos[i]);

		vkCmdBeginRenderPass(cmd_buffers[i], &(rendp_begin_infos[i]), VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		vkCmdDraw(cmd_buffers[i], 3, 1, 0, 0);

		cImGui_ImplVulkan_NewFrame();
		ImGui_ImplModeler_NewFrame();
		ImGui_NewFrame();
		ImGui_Begin("A Window", NULL, 0);
		ImGui_Text("Hello, Window!");
		ImGui_End();
		ImGui_Render();
		ImDrawData *drawData = ImGui_GetDrawData();
		cImGui_ImplVulkan_RenderDrawData(drawData, cmd_buffers[i]);

		vkCmdEndRenderPass(cmd_buffers[i]);

		vkEndCommandBuffer(cmd_buffers[i]);
		/* end render*/

		if (fens_img[img_index] != VK_NULL_HANDLE) {
			vkWaitForFences(device, 1, &(fens_img[img_index]), VK_TRUE, UINT64_MAX);
		}

		fens_img[img_index] = fens[cur_frame];

		VkSubmitInfo sub_info;
		sub_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		sub_info.pNext = NULL;

		VkSemaphore semps_wait[1];
		semps_wait[0] = semps_img_avl[cur_frame];
		VkPipelineStageFlags wait_stages[1];
		wait_stages[0] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		sub_info.waitSemaphoreCount = 1;
		sub_info.pWaitSemaphores = &(semps_wait[0]);
		sub_info.pWaitDstStageMask = &(wait_stages[0]);
		sub_info.commandBufferCount = 1;
		sub_info.pCommandBuffers = &(cmd_buffers[img_index]);

		VkSemaphore semps_sig[1];
		semps_sig[0] = semps_rend_fin[cur_frame];

		sub_info.signalSemaphoreCount = 1;
		sub_info.pSignalSemaphores = &(semps_sig[0]);

		vkResetFences(device, 1, &(fens[cur_frame]));

		vkQueueSubmit(graphicsQueue, 1, &sub_info, fens[cur_frame]);
//
//present
//
		VkPresentInfoKHR pres_info;

		pres_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		pres_info.pNext = NULL;
		pres_info.waitSemaphoreCount = 1;
		pres_info.pWaitSemaphores = &(semps_sig[0]);

		VkSwapchainKHR swaps[1];
		swaps[0] = swapchainInfo.swapchain;
		pres_info.swapchainCount = 1;
		pres_info.pSwapchains = &(swaps[0]);
		pres_info.pImageIndices = &img_index;
		pres_info.pResults = NULL;

		vkQueuePresentKHR(presentationQueue, &pres_info);

		cur_frame = (cur_frame + 1) % max_frames;
	}

cancelMainLoop:
	vkDeviceWaitIdle(device);
}
