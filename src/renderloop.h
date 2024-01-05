#ifndef MODELER_RENDERLOOP_H
#define MODELER_RENDERLOOP_H

#include <vulkan/vulkan.h>

#include "imgui/cimgui_impl_vulkan.h"

#include "swapchain.h"
#include "synchronization.h"
#include "queue.h"
#include "input_event.h"

void draw(VkDevice device, VkRenderPass renderPass, VkPipeline pipeline, VkFramebuffer *framebuffers, VkCommandBuffer *commandBuffers, SynchronizationInfo synchronizationInfo, SwapchainInfo swapchainInfo, VkImageView *imageViews, uint32_t imageViewCount, VkQueue graphicsQueue, VkQueue presentationQueue, uint32_t graphicsQueueFamilyIndex, const char *resourcePath, Queue *inputQueue, ImGui_ImplVulkan_InitInfo imVulkanInitInfo);

#endif /* MODELER_RENDERLOOP_H */
