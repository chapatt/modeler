#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "imgui/cimgui.h"
#include "imgui/cimgui_impl_vulkan.h"
#include "imgui/imgui_impl_modeler.h"

#include "pipeline.h"
#include "utils.h"
#include "vulkan_utils.h"

#include "renderloop.h"

typedef struct font_t {
	ImFont *font;
	float scale;
} Font;

static void pushFont(Font **fonts, size_t *fontCount, ImFont *font, float scale);
static ImFont *findFontWithScale(Font *fonts, size_t fontCount, float scale);
static void rescaleImGui(Font **fonts, size_t *fontCount, ImFont **currentFont, float scale, const char *resourcePath);

typedef struct component_t {
	void *object;
	VkViewport viewport;
	void (*handleInputEvent)(void *, InputEventType);
} Component;

bool isPointerOnViewport(VkViewport viewport, MousePosition position) {
	return position.x > viewport.x &&
		position.x < viewport.x + viewport.width &&
		position.y > viewport.y &&
		position.y < viewport.y + viewport.height;
}

static void sendInputToComponent(Component *components, size_t componentCount, InputEventType type, MousePosition pointerPosition) {
	for (size_t i = 0; i < componentCount; ++i) {
		if (isPointerOnViewport(components[i].viewport, pointerPosition)) {
			components[i].handleInputEvent(components[i].object, type);
			break;
		}
	}
}

bool draw(VkDevice device, void *platformWindow, WindowDimensions initialWindowDimensions, VkDescriptorSet **descriptorSets, VkRenderPass *renderPass, VkPipeline *pipelines, VkPipelineLayout *pipelineLayouts, VkFramebuffer **framebuffers, VkCommandBuffer **commandBuffers, SynchronizationInfo synchronizationInfo, SwapchainInfo *swapchainInfo, VkQueue graphicsQueue, VkQueue presentationQueue, uint32_t graphicsQueueFamilyIndex, const char *resourcePath, Queue *inputQueue, SwapchainCreateInfo swapchainCreateInfo, ChessBoard chessBoard, char **error)
{
	Font *fonts = NULL;
	size_t fontCount = 0;
	ImFont *currentFont = NULL;
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
	bool updateBoard = false;
	MousePosition pointerPosition;

	rescaleImGui(&fonts, &fontCount, &currentFont, windowDimensions.scale, resourcePath);

	for (uint32_t currentFrame = 0; true; currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT) {
		VkResult result;
		bool windowResized = false;

		VkViewport chessBoardViewport = {
			.x = windowDimensions.activeArea.offset.x + (windowDimensions.activeArea.extent.width / 4),
			.y = windowDimensions.activeArea.offset.y + (windowDimensions.activeArea.extent.height / 4),
			.width = windowDimensions.activeArea.extent.width / 2,
			.height = windowDimensions.activeArea.extent.width / 2,
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};

		Component components[] = {
			{
				.object = chessBoard,
				.viewport = chessBoardViewport,
				.handleInputEvent = &chessBoardHandleInputEvent
			}
		};

		InputEvent *inputEvent;
		while (dequeue(inputQueue, (void **) &inputEvent)) {
			InputEventType type = inputEvent->type;
			void *data = inputEvent->data;
			ResizeInfo *resizeInfo;

			switch(type) {
			case POINTER_LEAVE: case BUTTON_DOWN: case BUTTON_UP:
#ifdef ENABLE_IMGUI
				ImGui_ImplModeler_HandleInput(inputEvent);
#endif
				sendInputToComponent(components, sizeof(components) / sizeof(components[0]), type, pointerPosition);
				free(inputEvent);
				break;
			case POINTER_MOVE:
#ifdef ENABLE_IMGUI
				ImGui_ImplModeler_HandleInput(inputEvent);
#endif
				pointerPosition = *((MousePosition *) inputEvent->data);
				free(data);
				free(inputEvent);
				break;
			case RESIZE:
				resizeInfo = (ResizeInfo *) data;
				if (resizeInfo->windowDimensions.scale != windowDimensions.scale) {
					rescaleImGui(&fonts, &fontCount, &currentFont, resizeInfo->windowDimensions.scale, resourcePath);
				}
				windowDimensions = resizeInfo->windowDimensions;
				if (swapchainInfo->extent.width != windowDimensions.surfaceArea.width || swapchainInfo->extent.height != windowDimensions.surfaceArea.height) {
					windowResized = true;
				}
				ackResize(resizeInfo);
				free(resizeInfo->platformWindow);
				free(data);
				free(inputEvent);
				break;
			case TERMINATE:
				free(inputEvent);
				goto cancelMainLoop;
			}
		}

		if (updateBoard) {
			Board8x8 newSetup = {
				BLACK_ROOK, BLACK_KNIGHT, BLACK_BISHOP, BLACK_QUEEN, BLACK_KING, BLACK_BISHOP, BLACK_KNIGHT, BLACK_ROOK,
				BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN,
				EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
				EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
				EMPTY, EMPTY, EMPTY, EMPTY, WHITE_PAWN, EMPTY, EMPTY, EMPTY,
				EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
				WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, EMPTY, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN,
				WHITE_ROOK, WHITE_KNIGHT, WHITE_BISHOP, WHITE_QUEEN, WHITE_KING, WHITE_BISHOP, WHITE_KNIGHT, WHITE_ROOK
			};

			setBoard(chessBoard, newSetup);
			if (!updateChessBoard(chessBoard, error)) {
				asprintf(error, "Failed to update chess board.\n");
				return false;
			}
			updateBoard = false;
		}

		if (windowResized) {
			if (!recreateSwapchain(swapchainCreateInfo, windowDimensions, error)) {
				return false;
			}
			windowResized = false;
		}

		if ((result = vkWaitForFences(device, 1, synchronizationInfo.frameInFlightFences + currentFrame, VK_TRUE, UINT64_MAX)) != VK_SUCCESS) {
			asprintf(error, "Failed to wait for fences: %s", string_VkResult(result));
			return false;
		}

		uint32_t imageIndex = 0;
		result = vkAcquireNextImageKHR(device, swapchainInfo->swapchain, UINT64_MAX, synchronizationInfo.imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			if (!recreateSwapchain(swapchainCreateInfo, windowDimensions, error)) {
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
		renderPassBeginInfos[imageIndex].renderPass = *renderPass;
		renderPassBeginInfos[imageIndex].framebuffer = (*framebuffers)[imageIndex];
		renderPassBeginInfos[imageIndex].renderArea = renderArea;
		renderPassBeginInfos[imageIndex].clearValueCount = 2;
		renderPassBeginInfos[imageIndex].pClearValues = clearValues;

		vkResetCommandBuffer((*commandBuffers)[imageIndex], 0);
		vkBeginCommandBuffer((*commandBuffers)[imageIndex], &commandBufferBeginInfos[imageIndex]);
		vkCmdBeginRenderPass((*commandBuffers)[imageIndex], &(renderPassBeginInfos[imageIndex]), VK_SUBPASS_CONTENTS_INLINE);

		VkRect2D scissor = {
			.offset = {
				.x = 0,
				.y = 0
			},
			.extent = windowDimensions.activeArea.extent
		};
		vkCmdSetViewport((*commandBuffers)[imageIndex], 0, 1, &chessBoardViewport);
		vkCmdSetScissor((*commandBuffers)[imageIndex], 0, 1, &scissor);
		if (!drawChessBoard(chessBoard, (*commandBuffers)[imageIndex], error)) {
			return false;
		}

#ifdef ENABLE_IMGUI
		vkCmdNextSubpass((*commandBuffers)[imageIndex], VK_SUBPASS_CONTENTS_INLINE);

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

		cImGui_ImplVulkan_NewFrame();
		ImGui_ImplModeler_NewFrame();
		ImGui_NewFrame();
		ImGui_PushFont(currentFont);
		ImGui_Begin("Debug", NULL, 0);
		ImGui_Text("fps: %ld", 1000000000 / elapsed);
		if (ImGui_Button("Fullscreen")) {
			sendFullscreenSignal(platformWindow);
		}
		if (ImGui_Button("Exit Fullscreen")) {
			sendExitFullscreenSignal(platformWindow);
		}
		if (ImGui_Button("Move Pawn")) {
			updateBoard = true;
		}
		ImGui_End();
		ImGui_PopFont();
		ImGui_Render();
		ImDrawData *drawData = ImGui_GetDrawData();
		cImGui_ImplVulkan_RenderDrawData(drawData, (*commandBuffers)[imageIndex]);
#endif /* ENABLE_IMGUI */

#if DRAW_WINDOW_DECORATION
		vkCmdNextSubpass((*commandBuffers)[imageIndex], VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline((*commandBuffers)[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[0]);
		vkCmdBindDescriptorSets((*commandBuffers)[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts[1], 0, 1, *descriptorSets, 0, NULL);
		VkViewport secondViewport = {
			.x = 0.0f,
			.y = 0.0f,
			.width = swapchainInfo->extent.width,
			.height = swapchainInfo->extent.height,
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};
		VkRect2D secondScissor = {
			.offset = {
				.x = 0,
				.y = 0
			},
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
#endif /* DRAW_WINDOW_DECORATION */

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
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
			if (!recreateSwapchain(swapchainCreateInfo, windowDimensions, error)) {
				return false;
			}
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

static void pushFont(Font **fonts, size_t *fontCount, ImFont *font, float scale)
{
	*fonts = realloc(*fonts, sizeof(**fonts) * ++*fontCount);
	(*fonts)[*fontCount - 1] = (Font) {
		.font = font,
		.scale = scale
	};
}

static ImFont *findFontWithScale(Font *fonts, size_t fontCount, float scale)
{
	for (size_t i = 0; i < fontCount; ++i) {
		if (fonts[i].scale == scale) {
			return fonts[i].font;
		}
	}

	return NULL;
}

static void rescaleImGui(Font **fonts, size_t *fontCount, ImFont **currentFont, float scale, const char *resourcePath)
{
#ifdef ENABLE_IMGUI
	if (!(*currentFont = findFontWithScale(*fonts, *fontCount, scale))) {
		ImGuiIO *io = ImGui_GetIO();
		char *fontPath;
		asprintf(&fontPath, "%s/%s", resourcePath, "roboto.ttf");
		ImFont *font = ImFontAtlas_AddFontFromFileTTF(io->Fonts, fontPath, 16 * scale, NULL, NULL);
		pushFont(fonts, fontCount, font, scale);
		ImFontAtlas_Build(io->Fonts);
		cImGui_ImplVulkan_CreateFontsTexture();
		*currentFont = font;
	}
#endif /* ENABLE_IMGUI */
}
