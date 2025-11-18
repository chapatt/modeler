#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "pipeline.h"
#include "utils.h"
#include "vulkan_utils.h"
#include "window.h"

#include "renderloop.h"


#ifdef ENABLE_IMGUI
#include "imgui/dcimgui.h"
#include "imgui/dcimgui_impl_vulkan.h"
#include "imgui/imgui_impl_modeler.h"
#endif /* ENABLE_IMGUI */

#ifdef EMBED_FONTS
#include "../font_roboto.h"
#endif /* EMBED_FONTS */

typedef struct component_t {
	void *object;
	VkViewport *viewport;
	void (*handleInputEvent)(void *, InputEvent *);
} Component;

#ifdef ENABLE_IMGUI
typedef struct font_t {
	ImFont *font;
	float scale;
} Font;
#endif /* ENABLE_IMGUI */

static inline Orientation negateRotation(Orientation orientation);
static void sendInputToComponent(Component *components, size_t componentCount, InputEvent *inputEvent, PointerPosition pointerPosition);
static void updateViewports(WindowDimensions *windowDimensions, VkViewport *chessBoardViewport, VkViewport *titlebarViewport);
#ifdef ENABLE_IMGUI
static void pushFont(Font **fonts, size_t *fontCount, ImFont *font, float scale);
static ImFont *findFontWithScale(Font *fonts, size_t fontCount, float scale);
static bool rescaleImGui(Font **fonts, size_t *fontCount, ImFont **currentFont, float scale, const char *resourcePath, char **error);
#endif /* ENABLE_IMGUI */

bool draw(VkDevice device, void *platformWindow, WindowDimensions *windowDimensions, VkDescriptorSet *descriptorSets, VkRenderPass *renderPass, VkPipeline *pipelines, VkPipelineLayout *pipelineLayouts, VkFramebuffer **framebuffers, VkCommandBuffer *commandBuffers, SynchronizationInfo *synchronizationInfo, SwapchainInfo *swapchainInfo, VkQueue graphicsQueue, VkQueue presentationQueue, uint32_t graphicsQueueFamilyIndex, const char *resourcePath, Queue *inputQueue, SwapchainCreateInfo swapchainCreateInfo, ChessBoard chessBoard, ChessEngine chessEngine, Titlebar titlebar, char **error)
{
#ifdef ENABLE_IMGUI
	Font *fonts = NULL;
	size_t fontCount = 0;
	ImFont *currentFont = NULL;
#endif /* ENABLE_IMGUI */
	VkClearValue clearValue = {0.008f, 0.008f, 0.008f, 1.0f};
	VkClearValue stencilClearValue = {1.0f, 0.0f};
	VkClearValue secondClearValue = {0.0f, 0.0f, 0.0f, 0.0f};
#ifdef DRAW_WINDOW_BORDER
	VkClearValue clearValues[] = {clearValue, stencilClearValue, secondClearValue};
#else
	VkClearValue clearValues[] = {clearValue, stencilClearValue};
#endif /* DRAW_WINDOW_BORDER */

	struct timespec previousTime = {};
	const size_t queueLength = 10;
	long elapsedQueue[queueLength];
	size_t head = 0;
	long elapsed = 1;
	PointerPosition pointerPosition;
	bool enable3d = chessBoardGetEnable3d(chessBoard);
	bool enablePerspectiveProjection = chessBoardGetProjection(chessBoard) == PERSPECTIVE;
	bool windowResized = false;
	bool swapchainOutOfDate = false;
	bool insetsChanged = false;

#ifdef ENABLE_IMGUI
	if (!rescaleImGui(&fonts, &fontCount, &currentFont, windowDimensions->scale, resourcePath, error)) {
		return false;
	}
#endif /* ENABLE_IMGUI */

	VkViewport chessBoardViewport;
	VkViewport titlebarViewport;
	Component components[] = {
		{
			.object = chessBoard,
			.viewport = &chessBoardViewport,
			.handleInputEvent = &chessBoardHandleInputEvent
		},
		{
			.object = titlebar,
			.viewport = &titlebarViewport,
			.handleInputEvent = &titlebarHandleInputEvent
		}
	};
	updateViewports(windowDimensions, &chessBoardViewport, &titlebarViewport);

	for (uint32_t currentFrame = 0; true; currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT) {
		VkResult result;

		VkRenderPassBeginInfo renderPassBeginInfos[MAX_FRAMES_IN_FLIGHT];
		VkCommandBufferBeginInfo commandBufferBeginInfos[MAX_FRAMES_IN_FLIGHT];
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			commandBufferBeginInfos[i] = (VkCommandBufferBeginInfo) {
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				.pNext = NULL,
				.flags = 0,
				.pInheritanceInfo = NULL
			};
			renderPassBeginInfos[i] = (VkRenderPassBeginInfo) {
				.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
				.pNext = NULL,
				.clearValueCount = sizeof(clearValues),
				.pClearValues = clearValues
			};
		}

		InputEvent *inputEvent;
		while (dequeue(inputQueue, (void **) &inputEvent)) {
			InputEventType type = inputEvent->type;
			void *data = inputEvent->data;
			ResizeInfo *resizeInfo;
			Insets insets;

			switch(type) {
			case POINTER_LEAVE: case BUTTON_DOWN: case BUTTON_UP:
#ifdef ENABLE_IMGUI
				ImGui_ImplModeler_HandleInput(inputEvent);
#endif
				sendInputToComponent(components, sizeof(components) / sizeof(components[0]), inputEvent, pointerPosition);
				free(inputEvent);
				break;
			case POINTER_MOVE:
#ifdef ENABLE_IMGUI
				ImGui_ImplModeler_HandleInput(inputEvent);
#endif
				pointerPosition = *((PointerPosition *) inputEvent->data);
				int x = pointerPosition.x;
				switch (windowDimensions->orientation) {
				case ROTATE_90:
					pointerPosition.x = windowDimensions->activeArea.extent.width - pointerPosition.y;
					pointerPosition.y = x;
					break;
				case ROTATE_180:
					pointerPosition.x = windowDimensions->activeArea.extent.width - pointerPosition.x;
					pointerPosition.y = windowDimensions->activeArea.extent.height - pointerPosition.y;
					break;
				case ROTATE_270:
					pointerPosition.x = pointerPosition.y;
					pointerPosition.y = windowDimensions->activeArea.extent.height - x;
					break;
				}
				sendInputToComponent(components, sizeof(components) / sizeof(components[0]), inputEvent, pointerPosition);
				free(data);
				free(inputEvent);
				break;
			case RESIZE:
				windowResized = true;
				resizeInfo = (ResizeInfo *) data;
#ifdef ENABLE_IMGUI
				if (resizeInfo->windowDimensions.scale != windowDimensions->scale) {
					if (!rescaleImGui(&fonts, &fontCount, &currentFont, resizeInfo->windowDimensions.scale, resourcePath, error)) {
						return false;
					}
				}
#endif /* ENABLE_IMGUI */
				*windowDimensions = resizeInfo->windowDimensions;
				ackResize(resizeInfo);
				free(resizeInfo->platformWindow);
				free(data);
				free(inputEvent);
				break;
			case INSET_CHANGE:
				insetsChanged = true;
				insets = *(Insets *) data;
				free(data);
				free(inputEvent);
				rotateInsets(&insets, negateRotation(windowDimensions->orientation));
				windowDimensions->insets = insets;
				updateWindowDimensionsInsets(windowDimensions, insets);
				break;
			case TERMINATE:
				free(inputEvent);
				goto cancelMainLoop;
			}
		}

		if (windowResized || swapchainOutOfDate) {
			if (!recreateSwapchain(swapchainCreateInfo, windowResized, error)) {
				return false;
			}
		}

		if (windowResized || swapchainOutOfDate || insetsChanged) {
			Insets insets = getInsets(platformWindow);
			rotateInsets(&insets, negateRotation(windowDimensions->orientation));
			windowDimensions->insets = insets;
			updateWindowDimensionsInsets(windowDimensions, insets);

			updateViewports(windowDimensions, &chessBoardViewport, &titlebarViewport);

			float aspectRatio = (float) windowDimensions->surfaceArea.width / windowDimensions->titlebarHeight;
			titlebarSetAspectRatio(titlebar, aspectRatio);
			chessBoardSetOrientation(chessBoard, negateRotation(windowDimensions->orientation));
			if (!updateChessBoard(chessBoard, error)) {
				return false;
			}
		}

		windowResized = swapchainOutOfDate = insetsChanged = false;

		if ((result = vkWaitForFences(device, 1, synchronizationInfo->frameInFlightFences + currentFrame, VK_TRUE, UINT64_MAX)) != VK_SUCCESS) {
			asprintf(error, "Failed to wait for fences: %s", string_VkResult(result));
			return false;
		}

		uint32_t imageIndex = 0;
		result = vkAcquireNextImageKHR(device, swapchainInfo->swapchain, UINT64_MAX, synchronizationInfo->imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
#ifdef __APPLE__
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
#else /* __APPLE__ */
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
#endif /* __APPLE__ */
			swapchainOutOfDate = true;
			continue;
#ifdef __APPLE__
		} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
#else /* __APPLE__ */
        } else if (result != VK_SUCCESS) {
#endif /* __APPLE__ */
			asprintf(error, "Failed to acquire swapchain image: %s", string_VkResult(result));
			return false;
		}

		if ((result = vkResetFences(device, 1, synchronizationInfo->frameInFlightFences + currentFrame)) != VK_SUCCESS) {
			asprintf(error, "Failed to reset fences: %s", string_VkResult(result));
			return false;
		}

		VkRect2D renderArea = {
			.offset = {},
			.extent = swapchainInfo->extent
		};
		renderPassBeginInfos[currentFrame].renderPass = *renderPass;
		renderPassBeginInfos[currentFrame].framebuffer = (*framebuffers)[imageIndex];
		renderPassBeginInfos[currentFrame].renderArea = renderArea;

		vkResetCommandBuffer(commandBuffers[currentFrame], 0);
		vkBeginCommandBuffer(commandBuffers[currentFrame], &commandBufferBeginInfos[currentFrame]);
		vkCmdBeginRenderPass(commandBuffers[currentFrame], &renderPassBeginInfos[currentFrame], VK_SUBPASS_CONTENTS_INLINE);

		/* First subpass */

		VkRect2D scissor = {
			.offset = {
				.x = chessBoardViewport.x,
				.y = chessBoardViewport.y
			},
			.extent = (VkExtent2D) {
				.width = chessBoardViewport.width,
				.height = chessBoardViewport.height
			}
		};
		vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &chessBoardViewport);
		vkCmdSetScissor(commandBuffers[currentFrame], 0, 1, &scissor);
		if (!drawChessBoard(chessBoard, commandBuffers[currentFrame], error)) {
			return false;
		}

#ifdef ENABLE_IMGUI
		vkCmdNextSubpass(commandBuffers[currentFrame], VK_SUBPASS_CONTENTS_INLINE);

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
		ImGui_PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImVec2 windowPadding = {
			.x = 20.0f * windowDimensions->scale,
			.y = 20.0f * windowDimensions->scale
		};
		ImGui_PushStyleVarImVec2(ImGuiStyleVar_WindowPadding, windowPadding);
		ImVec2 framePadding = {
			.x = 5.0f * windowDimensions->scale,
			.y = 5.0f * windowDimensions->scale
		};
		ImGui_PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
		ImGui_PushStyleVarImVec2(ImGuiStyleVar_FramePadding, framePadding);
		ImVec2 itemSpacing = {
			.x = 10.0f * windowDimensions->scale,
			.y = 10.0f * windowDimensions->scale
		};
		ImGui_PushStyleVarImVec2(ImGuiStyleVar_ItemSpacing, itemSpacing);
		float optionsWindowWidth = 200;
		ImVec2 imguiWindowPos;
		ImVec2 imguiWindowSize;
		switch (windowDimensions->orientation) {
		case ROTATE_0:
			imguiWindowPos = (ImVec2) {
				.x = windowDimensions->activeArea.offset.x + windowDimensions->activeArea.extent.width - optionsWindowWidth * windowDimensions->scale,
				.y = windowDimensions->activeArea.offset.y + windowDimensions->titlebarHeight
			};
			imguiWindowSize = (ImVec2) {
				.x = optionsWindowWidth * windowDimensions->scale,
				.y = windowDimensions->activeArea.extent.height - windowDimensions->titlebarHeight
			};
			break;
		case ROTATE_90:
			imguiWindowPos = (ImVec2) {
				.x = windowDimensions->activeArea.offset.y + windowDimensions->activeArea.extent.height - (optionsWindowWidth * windowDimensions->scale + windowDimensions->titlebarHeight),
				.y = (windowDimensions->surfaceArea.width - windowDimensions->activeArea.extent.width) - windowDimensions->activeArea.offset.x
			};
			imguiWindowSize = (ImVec2) {
				.x = optionsWindowWidth * windowDimensions->scale,
				.y = windowDimensions->activeArea.extent.width
			};
			break;
		case ROTATE_180:
			imguiWindowPos = (ImVec2) {
				.x = windowDimensions->activeArea.offset.x + windowDimensions->activeArea.extent.width - optionsWindowWidth * windowDimensions->scale,
				.y = windowDimensions->surfaceArea.height - (windowDimensions->activeArea.extent.height + windowDimensions->activeArea.offset.y + windowDimensions->titlebarHeight)
			};
			imguiWindowSize = (ImVec2) {
				.x = optionsWindowWidth * windowDimensions->scale,
				.y = windowDimensions->activeArea.extent.height - windowDimensions->titlebarHeight
			};
			break;
		case ROTATE_270:
			imguiWindowPos = (ImVec2) {
				.x = windowDimensions->surfaceArea.height - (windowDimensions->activeArea.offset.y + windowDimensions->titlebarHeight + optionsWindowWidth * windowDimensions->scale),
				.y = windowDimensions->activeArea.offset.x
			};
			imguiWindowSize = (ImVec2) {
				.x = optionsWindowWidth * windowDimensions->scale,
				.y = windowDimensions->activeArea.extent.width
			};
			break;
		}
		ImGui_SetNextWindowPos(imguiWindowPos, 0);
		ImGui_SetNextWindowSize(imguiWindowSize, 0);
		ImGui_Begin("Debug", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
		ImGui_Text("fps: %ld", 1000000000 / elapsed);
		if (ImGui_Button("Close")) {
			sendCloseSignal(platformWindow);
		}
		if (ImGui_Button("Fullscreen")) {
			sendFullscreenSignal(platformWindow);
		}
		if (ImGui_Button("Exit Fullscreen")) {
			sendExitFullscreenSignal(platformWindow);
		}
		if (ImGui_Button("Reset Board")) {
			chessEngineReset(chessEngine);
		}
		if (ImGui_Checkbox("3D", &enable3d)) {
			chessBoardSetEnable3d(chessBoard, enable3d);
			if (!updateChessBoard(chessBoard, error)) {
				return false;
			}
		}
		ImGui_BeginDisabled(!enable3d);
		if (ImGui_Checkbox("Perspective", &enablePerspectiveProjection)) {
			if (enablePerspectiveProjection) {
				chessBoardSetProjection(chessBoard, PERSPECTIVE);
			} else {
				chessBoardSetProjection(chessBoard, ORTHOGRAPHIC);
			}
		}
		ImGui_EndDisabled();
		ImGui_End();
		ImGui_PopStyleVar();
		ImGui_PopStyleVar();
		ImGui_PopStyleVar();
		ImGui_PopStyleVar();
		ImGui_PopStyleVar();
		ImGui_PopFont();
		ImGui_Render();
		ImDrawData *drawData = ImGui_GetDrawData();
		cImGui_ImplVulkan_RenderDrawData(drawData, commandBuffers[currentFrame]);
#endif /* ENABLE_IMGUI */

		vkCmdNextSubpass(commandBuffers[currentFrame], VK_SUBPASS_CONTENTS_INLINE);
		if (!windowDimensions->fullscreen) {
			VkRect2D titlebarScissor = {
				.offset = {
					.x = titlebarViewport.x,
					.y = titlebarViewport.y
				},
				.extent = (VkExtent2D) {
					.width = titlebarViewport.width,
					.height = titlebarViewport.height
				}
			};
			vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &titlebarViewport);
			vkCmdSetScissor(commandBuffers[currentFrame], 0, 1, &titlebarScissor);
			if (!drawTitlebar(titlebar, commandBuffers[currentFrame], error)) {
				return false;
			}
		}

#if DRAW_WINDOW_BORDER
		vkCmdNextSubpass(commandBuffers[currentFrame], VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[0]);
		vkCmdBindDescriptorSets(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts[0], 0, 1, descriptorSets, 0, NULL);
		VkViewport secondViewport = {
			.x = 0.0f,
			.y = 0.0f,
			.width = windowDimensions->surfaceArea.width,
			.height = windowDimensions->surfaceArea.height,
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};
		VkRect2D secondScissor = {
			.offset = {
				.x = 0,
				.y = 0
			},
			.extent = windowDimensions->surfaceArea
		};
		vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &secondViewport);
		vkCmdSetScissor(commandBuffers[currentFrame], 0, 1, &secondScissor);
		PushConstants secondPushConstants = {
			.extent = {windowDimensions->activeArea.extent.width, windowDimensions->activeArea.extent.height},
			.offset = {windowDimensions->activeArea.offset.x, windowDimensions->activeArea.offset.y},
			.cornerRadius = windowDimensions->cornerRadius
		};
		vkCmdPushConstants(commandBuffers[currentFrame], pipelineLayouts[0], VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(secondPushConstants), &secondPushConstants);
		vkCmdDraw(commandBuffers[currentFrame], 3, 1, 0, 0);
#endif /* DRAW_WINDOW_BORDER */

		vkCmdEndRenderPass(commandBuffers[currentFrame]);
		vkEndCommandBuffer(commandBuffers[currentFrame]);

		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		VkSubmitInfo submitInfo = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = NULL,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = synchronizationInfo->imageAvailableSemaphores + currentFrame,
			.pWaitDstStageMask = waitStages,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = synchronizationInfo->renderFinishedSemaphores + imageIndex,
			.commandBufferCount = 1,
			.pCommandBuffers = commandBuffers + currentFrame
		};

		if ((result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, synchronizationInfo->frameInFlightFences[currentFrame])) != VK_SUCCESS) {
			asprintf(error, "Failed to submit render queue: %s", string_VkResult(result));
			return false;
		}

		VkPresentInfoKHR presentInfo = {
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.pNext = NULL,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = synchronizationInfo->renderFinishedSemaphores + imageIndex,
			.swapchainCount = 1,
			.pSwapchains = &swapchainInfo->swapchain,
			.pImageIndices = &imageIndex,
			.pResults = NULL
		};

		result = vkQueuePresentKHR(presentationQueue, &presentInfo);
		/*
			TODO: macOS sends this on every frame
			if (result == VK_SUBOPTIMAL_KHR) { }
		*/
#ifdef __APPLE__
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
#else /* __APPLE__ */
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
#endif /* __APPLE__ */
			swapchainOutOfDate = true;
			continue;
		} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			asprintf(error, "Failed to present swapchain image: %s", string_VkResult(result));
			return false;
		}
	}

cancelMainLoop:
	vkDeviceWaitIdle(device);
	return true;
}

static void sendInputToComponent(Component *components, size_t componentCount, InputEvent *inputEvent, PointerPosition pointerPosition)
{
	InputEvent newInputEvent;

	for (size_t i = 0; i < componentCount; ++i) {
		if (isPointerOnViewport(*components[i].viewport, pointerPosition)) {
			NormalizedPointerPosition normalizedPointerPosition;

			switch(inputEvent->type) {
			case POINTER_LEAVE: case BUTTON_DOWN: case BUTTON_UP:
				newInputEvent.type = inputEvent->type;
				break;
			case POINTER_MOVE:
				normalizedPointerPosition = normalizePointerPosition(*components[i].viewport, pointerPosition);
				newInputEvent.data = &normalizedPointerPosition;
				newInputEvent.type = NORMALIZED_POINTER_MOVE;
				break;
			}

			components[i].handleInputEvent(components[i].object, &newInputEvent);
			break;
		}
	}
}

static void updateViewports(WindowDimensions *windowDimensions, VkViewport *chessBoardViewport, VkViewport *titlebarViewport)
{
	VkExtent2D contentExtent = {
		.height = windowDimensions->activeArea.extent.height - windowDimensions->titlebarHeight,
		.width = windowDimensions->activeArea.extent.width
	};

	bool isLandscape = contentExtent.width >= contentExtent.height;

	int minorDimension = isLandscape ? contentExtent.height : contentExtent.width;

	*chessBoardViewport = (VkViewport) {
		.x = (isLandscape ? (contentExtent.width - contentExtent.height) / 2 : 0) + windowDimensions->activeArea.offset.x,
		.y = (isLandscape ? 0 : (contentExtent.height - contentExtent.width) / 2) + windowDimensions->activeArea.offset.y + windowDimensions->titlebarHeight,
		.width = minorDimension,
		.height = minorDimension,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	if (windowDimensions->orientation == ROTATE_90) {
		*titlebarViewport = (VkViewport) {
			.x = windowDimensions->activeArea.offset.x,
			.y = (windowDimensions->activeArea.offset.y + windowDimensions->activeArea.extent.height) - windowDimensions->titlebarHeight,
			.width = contentExtent.width,
			.height = windowDimensions->titlebarHeight,
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};
	} else {
		*titlebarViewport = (VkViewport) {
			.x = windowDimensions->activeArea.offset.x,
			.y = windowDimensions->activeArea.offset.y,
			.width = contentExtent.width,
			.height = windowDimensions->titlebarHeight,
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};
	}
}

#ifdef ENABLE_IMGUI
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

static bool rescaleImGui(Font **fonts, size_t *fontCount, ImFont **currentFont, float scale, const char *resourcePath, char **error)
{
	if (!(*currentFont = findFontWithScale(*fonts, *fontCount, scale))) {
		ImGuiIO *io = ImGui_GetIO();
#ifndef EMBED_FONTS
		char *robotoFontPath;
		int robotoFontSize;
		char *robotoFontBytes;
		asprintf(&robotoFontPath, "%s/%s", resourcePath, "roboto.ttf");
		if ((robotoFontSize = readFileToString(robotoFontPath, &robotoFontBytes)) == -1) {
			asprintf(error, "Failed to load font file.\n");
			return false;
		}
#endif /* EMBED_FONTS */
		ImFont *font = ImFontAtlas_AddFontFromMemoryTTF(io->Fonts, robotoFontBytes, robotoFontSize, 16 * scale, NULL, NULL);
		pushFont(fonts, fontCount, font, scale);
		ImFontAtlas_Build(io->Fonts);
		*currentFont = font;
	}

	return true;
}
#endif /* ENABLE_IMGUI */
