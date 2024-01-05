#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "imgui/cimgui.h"
#include "imgui/cimgui_impl_vulkan.h"
#include "imgui/imgui_impl_modeler.h"

#include "utils.h"
#include "vulkan_utils.h"

#include "renderloop.h"

void draw(VkDevice device, VkRenderPass renderPass, VkPipeline pipeline, VkFramebuffer *framebuffers, VkCommandBuffer *commandBuffers, SynchronizationInfo synchronizationInfo, SwapchainInfo swapchainInfo, VkImageView *imageViews, uint32_t imageViewCount, VkQueue graphicsQueue, VkQueue presentationQueue, uint32_t graphicsQueueFamilyIndex, const char *resourcePath, Queue *inputQueue, ImGui_ImplVulkan_InitInfo imVulkanInitInfo)
{
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

	for (uint32_t cur_frame = 0;; cur_frame = (cur_frame + 1) % MAX_FRAMES_IN_FLIGHT) {
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
		vkWaitForFences(device, 1, synchronizationInfo.frameInFlightFences + cur_frame, VK_TRUE, UINT64_MAX);

		uint32_t img_index = 0;
		vkAcquireNextImageKHR(device, swapchainInfo.swapchain, UINT64_MAX, synchronizationInfo.imageAvailableSemaphores[cur_frame], VK_NULL_HANDLE, &img_index);

		/* render */
		uint32_t i = img_index;
		cmd_buff_begin_infos[i].sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmd_buff_begin_infos[i].pNext = NULL;
		cmd_buff_begin_infos[i].flags = 0;
		cmd_buff_begin_infos[i].pInheritanceInfo = NULL;

		rendp_begin_infos[i].sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rendp_begin_infos[i].pNext = NULL;
		rendp_begin_infos[i].renderPass = renderPass;
		rendp_begin_infos[i].framebuffer = framebuffers[i];
		rendp_begin_infos[i].renderArea = rendp_area;
		rendp_begin_infos[i].clearValueCount = 1;
		rendp_begin_infos[i].pClearValues = &clear_val;

		vkBeginCommandBuffer(commandBuffers[i], &cmd_buff_begin_infos[i]);

		vkCmdBeginRenderPass(commandBuffers[i], &(rendp_begin_infos[i]), VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);

		cImGui_ImplVulkan_NewFrame();
		ImGui_ImplModeler_NewFrame();
		ImGui_NewFrame();
		ImGui_Begin("A Window", NULL, 0);
		ImGui_Text("Hello, Window!");
		ImGui_End();
		ImGui_Render();
		ImDrawData *drawData = ImGui_GetDrawData();
		cImGui_ImplVulkan_RenderDrawData(drawData, commandBuffers[i]);

		vkCmdEndRenderPass(commandBuffers[i]);

		vkEndCommandBuffer(commandBuffers[i]);
		/* end render*/

		VkSubmitInfo sub_info;
		sub_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		sub_info.pNext = NULL;

		VkSemaphore semps_wait[1];
		semps_wait[0] = synchronizationInfo.imageAvailableSemaphores[cur_frame];
		VkPipelineStageFlags wait_stages[1];
		wait_stages[0] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		sub_info.waitSemaphoreCount = 1;
		sub_info.pWaitSemaphores = &(semps_wait[0]);
		sub_info.pWaitDstStageMask = &(wait_stages[0]);
		sub_info.commandBufferCount = 1;
		sub_info.pCommandBuffers = &(commandBuffers[img_index]);

		VkSemaphore semps_sig[1];
		semps_sig[0] = synchronizationInfo.renderFinishedSemaphores[cur_frame];

		sub_info.signalSemaphoreCount = 1;
		sub_info.pSignalSemaphores = &(semps_sig[0]);

		vkResetFences(device, 1, &(synchronizationInfo.frameInFlightFences[cur_frame]));

		vkQueueSubmit(graphicsQueue, 1, &sub_info, synchronizationInfo.frameInFlightFences[cur_frame]);
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
	}

cancelMainLoop:
	vkDeviceWaitIdle(device);
}
