#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "imgui/cimgui.h"
#include "imgui/cimgui_impl_vulkan.h"
#include "imgui/imgui_impl_modeler.h"

#include "utils.h"
#include "vulkan_utils.h"

#include "renderloop.h"

bool draw(VkDevice device, VkRenderPass renderPass, VkPipeline pipeline, VkFramebuffer *framebuffers, VkCommandBuffer *commandBuffers, SynchronizationInfo synchronizationInfo, SwapchainInfo swapchainInfo, VkImageView *imageViews, uint32_t imageViewCount, VkQueue graphicsQueue, VkQueue presentationQueue, uint32_t graphicsQueueFamilyIndex, const char *resourcePath, Queue *inputQueue, ImGui_ImplVulkan_InitInfo imVulkanInitInfo, char **error)
{
	VkCommandBufferBeginInfo cmd_buff_begin_infos[imageViewCount];
	VkRenderPassBeginInfo rendp_begin_infos[imageViewCount];
	VkRect2D rendp_area;
	rendp_area.offset.x = 0;
	rendp_area.offset.y = 0;
	rendp_area.extent = swapchainInfo.extent;
	VkClearValue clear_val = {0.1f, 0.3f, 0.3f, 1.0f};
	for (uint32_t i = 0; i < imageViewCount; ++i) {
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
	}
	cImGui_ImplVulkan_Init(&imVulkanInitInfo, renderPass);

	struct timespec previousTime = {};
	const size_t queueLength = 10;
	long elapsedQueue[queueLength];
	size_t head = 0;
	long elapsed = 1;
	for (uint32_t currentFrame = 0; true; currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT) {
		VkResult result;

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

		struct timespec spec;
		clock_gettime(CLOCK_MONOTONIC, &spec);
			head = (head + 1) % queueLength;
			elapsedQueue[head] = spec.tv_nsec + ((1000000000 * (spec.tv_sec - previousTime.tv_sec)) - previousTime.tv_nsec);
		if (spec.tv_sec > previousTime.tv_sec) {
			for (size_t i = 0; i < queueLength; ++i) {
				elapsed = (elapsed + elapsedQueue[i]) / 2;
			}
		}
		previousTime.tv_sec = spec.tv_sec;
		previousTime.tv_nsec = spec.tv_nsec;

		vkWaitForFences(device, 1, synchronizationInfo.frameInFlightFences + currentFrame, VK_TRUE, UINT64_MAX);
		vkResetFences(device, 1, synchronizationInfo.frameInFlightFences + currentFrame);

		uint32_t imageIndex = 0;
		vkAcquireNextImageKHR(device, swapchainInfo.swapchain, UINT64_MAX, synchronizationInfo.imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

		vkResetCommandBuffer(commandBuffers[imageIndex], 0);
		vkBeginCommandBuffer(commandBuffers[imageIndex], &cmd_buff_begin_infos[imageIndex]);
		vkCmdBeginRenderPass(commandBuffers[imageIndex], &(rendp_begin_infos[imageIndex]), VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		vkCmdDraw(commandBuffers[imageIndex], 3, 1, 0, 0);
		cImGui_ImplVulkan_NewFrame();
		ImGui_ImplModeler_NewFrame();
		ImGui_NewFrame();
		ImGui_Begin("A Window", NULL, 0);
		ImGui_Text("fps: %ld", 1000000000 / elapsed);
		ImGui_End();
		ImGui_Render();
		ImDrawData *drawData = ImGui_GetDrawData();
		cImGui_ImplVulkan_RenderDrawData(drawData, commandBuffers[imageIndex]);

		vkCmdEndRenderPass(commandBuffers[imageIndex]);
		vkEndCommandBuffer(commandBuffers[imageIndex]);

		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		VkSubmitInfo submitInfo = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = NULL,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = synchronizationInfo.imageAvailableSemaphores + currentFrame,
			.pWaitDstStageMask = waitStages,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = synchronizationInfo.renderFinishedSemaphores + currentFrame,
			.commandBufferCount = 1,
			.pCommandBuffers = commandBuffers + imageIndex
		};

		if ((result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, synchronizationInfo.frameInFlightFences[currentFrame])) != VK_SUCCESS) {
			asprintf(error, "Failed to submit queue: %s", string_VkResult(result));
			return false;
		}

		VkPresentInfoKHR presentInfo = {
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.pNext = NULL,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = synchronizationInfo.renderFinishedSemaphores + currentFrame,
			.swapchainCount = 1,
			.pSwapchains = &swapchainInfo.swapchain,
			.pImageIndices = &imageIndex,
			.pResults = NULL
		};

		if ((result = vkQueuePresentKHR(presentationQueue, &presentInfo)) != VK_SUCCESS) {
			asprintf(error, "Failed to present framebuffer: %s", string_VkResult(result));
			return false;
		}
	}

cancelMainLoop:
	vkDeviceWaitIdle(device);
	return true;
}
