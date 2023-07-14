#ifndef MODELER_RENDERLOOP_H
#define MODELER_RENDERLOOP_H

#include <vulkan/vulkan.h>

void draw(VkDevice dev, VkSwapchainKHR swap, VkExtent2D windowExtent, VkQueue graphicsQueue, VkQueue presentationQueue, uint32_t graphicsQueueFamilyIndex, const char *resourcePath);

#endif // MODELER_RENDERLOOP_H
