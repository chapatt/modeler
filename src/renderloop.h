#ifndef MODELER_RENDERLOOP_H
#define MODELER_RENDERLOOP_H

#include <vulkan/vulkan.h>

#include "queue.h"
#include "inputEvent.h"

void draw(VkDevice dev, VkSwapchainKHR swap, VkImageView *imageViews, uint32_t imageViewCount, VkExtent2D windowExtent, VkQueue graphicsQueue, VkQueue presentationQueue, uint32_t graphicsQueueFamilyIndex, const char *resourcePath, Queue *inputQueue);

#endif // MODELER_RENDERLOOP_H
