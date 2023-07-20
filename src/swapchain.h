#ifndef MODELER_SWAPCHAIN_H
#define MODELER_SWAPCHAIN_H

#include <stdbool.h>
#include <vulkan/vulkan.h>

#include "physical_device.h"

bool createSwapchain(VkDevice device, VkSurfaceKHR surface, PhysicalDeviceSurfaceCharacteristics surfaceCharacteristics, uint32_t graphicsQueueFamilyIndex, uint32_t presentationQueueFamilyIndex, VkExtent2D windowExtent, VkSwapchainKHR *swapchain, char **error);

void destroySwapchain(VkDevice device, VkSwapchainKHR swapchain);

#endif // MODELER_SWAPCHAIN_H
