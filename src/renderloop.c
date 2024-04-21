#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "imgui/cimgui.h"
#include "imgui/cimgui_impl_vulkan.h"
#include "imgui/imgui_impl_modeler.h"

#include "modeler.h"
#include "pipeline.h"
#include "utils.h"
#include "vulkan_utils.h"

#include "renderloop.h"

bool draw(VkDevice device, WindowDimensions initialWindowDimensions, VkDescriptorSet **descriptorSets, VkRenderPass renderPass, VkPipeline *pipelines, VkPipelineLayout *pipelineLayouts, VkFramebuffer **framebuffers, VkCommandBuffer **commandBuffers, SynchronizationInfo synchronizationInfo, SwapchainInfo *swapchainInfo, VkQueue graphicsQueue, VkQueue presentationQueue, uint32_t graphicsQueueFamilyIndex, const char *resourcePath, Queue *inputQueue, ImGui_ImplVulkan_InitInfo imVulkanInitInfo, SwapchainCreateInfo swapchainCreateInfo, char **error)
{
	WindowDimensions windowDimensions = initialWindowDimensions;
	VkCommandBufferBeginInfo commandBufferBeginInfos[swapchainInfo->imageCount];
	VkRenderPassBeginInfo renderPassBeginInfos[swapchainInfo->imageCount];
	VkClearValue clearValue = {0.1f, 0.3f, 0.3f, 1.0f};
	VkClearValue secondClearValue = {0.0f, 0.0f, 0.0f, 0.0f};
	VkClearValue clearValues[] = {clearValue, secondClearValue};
	for (uint32_t i = 0; i < swapchainInfo->imageCount; ++i) {
		commandBufferBeginInfos[i].sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandBufferBeginInfos[i].pNext = NULL;
		commandBufferBeginInfos[i].flags = 0;
		commandBufferBeginInfos[i].pInheritanceInfo = NULL;
	}

	struct timespec previousTime = {};
	const size_t queueLength = 10;
	long elapsedQueue[queueLength];
	size_t head = 0;
	long elapsed = 1;
	for (uint32_t currentFrame = 0; true; currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT) {
		VkResult result;
		bool windowResized = false;

		InputEvent *inputEvent;
		while (dequeue(inputQueue, (void **) &inputEvent)) {
			InputEventType type = inputEvent->type;
			void *data = inputEvent->data;

			switch(type) {
			case POINTER_LEAVE: case BUTTON_DOWN: case BUTTON_UP:
				// ImGui_ImplModeler_HandleInput(inputEvent);
				free(inputEvent);
				break;
			case POINTER_MOVE:
				// ImGui_ImplModeler_HandleInput(inputEvent);
				free(data);
				free(inputEvent);
				break;
			case RESIZE:
				windowDimensions = *((WindowDimensions *) data);
				windowResized = true;
				free(data);
				free(inputEvent);
				break;
			case TERMINATE:
				free(inputEvent);
				goto cancelMainLoop;
			}
		}

		if ((result = vkWaitForFences(device, 1, synchronizationInfo.frameInFlightFences + currentFrame, VK_TRUE, UINT64_MAX)) != VK_SUCCESS) {
			asprintf(error, "Failed to wait for fences: %s", string_VkResult(result));
			return false;
		}

		uint32_t imageIndex = 0;
		result = vkAcquireNextImageKHR(device, swapchainInfo->swapchain, UINT64_MAX, synchronizationInfo.imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			if (!recreateSwapchain(swapchainCreateInfo, windowDimensions.surfaceArea, error)) {
				return false;
			}
			continue;
		} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			asprintf(error, "Failed to acquire swapchain image: %s", string_VkResult(result));
			return false;
		}

		if ((result = vkResetFences(device, 1, synchronizationInfo.frameInFlightFences + currentFrame)) != VK_SUCCESS) {
			asprintf(error, "Failed to reset fences: %s", string_VkResult(result));
			return false;
		}

		VkRect2D renderArea = {
			.offset = {},
			.extent = swapchainInfo->extent
		};
		renderPassBeginInfos[imageIndex].sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfos[imageIndex].pNext = NULL;
		renderPassBeginInfos[imageIndex].renderPass = renderPass;
		renderPassBeginInfos[imageIndex].framebuffer = (*framebuffers)[imageIndex];
		renderPassBeginInfos[imageIndex].renderArea = renderArea;
		renderPassBeginInfos[imageIndex].clearValueCount = 2;
		renderPassBeginInfos[imageIndex].pClearValues = clearValues;

		vkResetCommandBuffer((*commandBuffers)[imageIndex], 0);
		vkBeginCommandBuffer((*commandBuffers)[imageIndex], &commandBufferBeginInfos[imageIndex]);
		vkCmdBeginRenderPass((*commandBuffers)[imageIndex], &(renderPassBeginInfos[imageIndex]), VK_SUBPASS_CONTENTS_INLINE);

		// struct timespec spec;
		// clock_gettime(CLOCK_MONOTONIC, &spec);
		// 	head = (head + 1) % queueLength;
		// 	elapsedQueue[head] = spec.tv_nsec + ((1000000000 * (spec.tv_sec - previousTime.tv_sec)) - previousTime.tv_nsec);
		// if (spec.tv_sec > previousTime.tv_sec) {
		// 	for (size_t i = 0; i < queueLength; ++i) {
		// 		elapsed = (elapsed + elapsedQueue[i]) / 2;
		// 	}
		// }
		// previousTime.tv_sec = spec.tv_sec;
		// previousTime.tv_nsec = spec.tv_nsec;
#ifdef ENABLE_IMGUI
		cImGui_ImplVulkan_NewFrame();
		ImGui_ImplModeler_NewFrame();
		ImGui_NewFrame();
		ImGui_Begin("A Window", NULL, 0);
		ImGui_Text("fps: %ld", 1000000000 / elapsed);
		ImGui_End();
		ImGui_Render();
		ImDrawData *drawData = ImGui_GetDrawData();
		cImGui_ImplVulkan_RenderDrawData(drawData, (*commandBuffers)[imageIndex]);
#endif /* ENABLE_IMGUI */

		vkCmdBindPipeline((*commandBuffers)[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[0]);
		VkViewport viewport = {
			.x = windowDimensions.activeArea.offset.x,
			.y = windowDimensions.activeArea.offset.y,
			.width = windowDimensions.activeArea.extent.width,
			.height = windowDimensions.activeArea.extent.height,
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};
		VkOffset2D scissorOffset = {
			.x = 0,
			.y = 0
		};
		VkRect2D scissor = {
			.offset = scissorOffset,
			.extent = swapchainInfo->extent
		};
		vkCmdSetViewport((*commandBuffers)[imageIndex], 0, 1, &viewport);
		vkCmdSetScissor((*commandBuffers)[imageIndex], 0, 1, &scissor);
		PushConstants pushConstants = {
			.extent = {windowDimensions.activeArea.extent.width, windowDimensions.activeArea.extent.height},
			.offset = {windowDimensions.activeArea.offset.x, windowDimensions.activeArea.offset.y},
			.cornerRadius = windowDimensions.cornerRadius
		};
		vkCmdPushConstants((*commandBuffers)[imageIndex], pipelineLayouts[0], VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);
		vkCmdDraw((*commandBuffers)[imageIndex], 3, 1, 0, 0);
#if DRAW_WINDOW_DECORATION
		vkCmdNextSubpass((*commandBuffers)[imageIndex], VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline((*commandBuffers)[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[1]);
		vkCmdBindDescriptorSets((*commandBuffers)[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts[1], 0, 1, *descriptorSets, 0, NULL);
		VkViewport secondViewport = {
			.x = 0.0f,
			.y = 0.0f,
			.width = swapchainInfo->extent.width,
			.height = swapchainInfo->extent.height,
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};
		VkOffset2D secondScissorOffset = {
			.x = 0,
			.y = 0
		};
		VkRect2D secondScissor = {
			.offset = scissorOffset,
			.extent = swapchainInfo->extent
		};
		vkCmdSetViewport((*commandBuffers)[imageIndex], 0, 1, &secondViewport);
		vkCmdSetScissor((*commandBuffers)[imageIndex], 0, 1, &secondScissor);
		PushConstants secondPushConstants = {
			.extent = {windowDimensions.activeArea.extent.width, windowDimensions.activeArea.extent.height},
			.offset = {windowDimensions.activeArea.offset.x, windowDimensions.activeArea.offset.y},
			.cornerRadius = windowDimensions.cornerRadius
		};
		vkCmdPushConstants((*commandBuffers)[imageIndex], pipelineLayouts[1], VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(secondPushConstants), &secondPushConstants);
		vkCmdDraw((*commandBuffers)[imageIndex], 3, 1, 0, 0);
#endif

		vkCmdEndRenderPass((*commandBuffers)[imageIndex]);
		vkEndCommandBuffer((*commandBuffers)[imageIndex]);

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
			.pCommandBuffers = *commandBuffers + imageIndex
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
			.pSwapchains = &swapchainInfo->swapchain,
			.pImageIndices = &imageIndex,
			.pResults = NULL
		};

		result = vkQueuePresentKHR(presentationQueue, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || windowResized) {
			if (!recreateSwapchain(swapchainCreateInfo, windowDimensions.surfaceArea, error)) {
				return false;
			}
			windowResized = false;
			continue;
		} else if (result != VK_SUCCESS) {
			asprintf(error, "Failed to present swapchain image: %s", string_VkResult(result));
			return false;
		}
	}

cancelMainLoop:
	vkDeviceWaitIdle(device);
	return true;
}
