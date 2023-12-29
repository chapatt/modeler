#ifndef MODELER_RENDERLOOP_H
#define MODELER_RENDERLOOP_H

#include <vulkan/vulkan.h>

#include "queue.h"
#include "input_event.h"

void draw(VkDevice device, VkRenderPass renderPass, VkPipeline pipeline, VkSwapchainKHR swap, VkImageView *imageViews, uint32_t imageViewCount, VkExtent2D windowExtent, VkQueue graphicsQueue, VkQueue presentationQueue, uint32_t graphicsQueueFamilyIndex, const char *resourcePath, Queue *inputQueue, ImGui_ImplVulkan_InitInfo imVulkanInitInfo);

#endif // MODELER_RENDERLOOP_H
