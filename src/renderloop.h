#ifndef MODELER_RENDERLOOP_H
#define MODELER_RENDERLOOP_H

#include <vulkan/vulkan.h>

#include "imgui/cimgui_impl_vulkan.h"

#include "swapchain.h"
#include "synchronization.h"
#include "queue.h"
#include "input_event.h"

bool draw(VkDevice device, VkRenderPass renderPass, VkPipeline pipeline, VkFramebuffer **framebuffers, VkCommandBuffer **commandBuffers, SynchronizationInfo synchronizationInfo, SwapchainInfo *swapchainInfo, VkImageView *imageViews, VkQueue graphicsQueue, VkQueue presentationQueue, uint32_t graphicsQueueFamilyIndex, const char *resourcePath, Queue *inputQueue, ImGui_ImplVulkan_InitInfo imVulkanInitInfo, SwapchainCreateInfo swapchainCreateInfo, char **error);

#endif /* MODELER_RENDERLOOP_H */
