#ifndef MODELER_RENDERLOOP_H
#define MODELER_RENDERLOOP_H

#include <stdbool.h>
#include <vulkan/vulkan.h>

#include "imgui/cimgui_impl_vulkan.h"

#include "modeler.h"
#include "swapchain.h"
#include "synchronization.h"
#include "queue.h"
#include "input_event.h"
#include "chess_board.h"

bool draw(VkDevice device, void *platformWindow, WindowDimensions *windowDimensions, VkDescriptorSet **descriptorSets, VkRenderPass *renderPass, VkPipeline *pipelines, VkPipelineLayout *pipelineLayouts, VkFramebuffer **framebuffers, VkCommandBuffer *commandBuffers, SynchronizationInfo synchronizationInfo, SwapchainInfo *swapchainInfo, VkQueue graphicsQueue, VkQueue presentationQueue, uint32_t graphicsQueueFamilyIndex, const char *resourcePath, Queue *inputQueue, SwapchainCreateInfo swapchainCreateInfo, ChessBoard chessBoard, char **error);

#endif /* MODELER_RENDERLOOP_H */
