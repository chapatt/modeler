#ifndef MODELER_SWAPCHAIN_H
#define MODELER_SWAPCHAIN_H

#include <stdbool.h>
#include <vulkan/vulkan.h>

#include "physical_device.h"

typedef struct swapchain_info_t {
	VkSwapchainKHR swapchain;
	uint32_t imageCount;
	VkImage *images;
	VkSurfaceFormatKHR surfaceFormat;
	VkExtent2D extent;
} SwapchainInfo;

bool createSwapchain(VkDevice device, VkSurfaceKHR surface, PhysicalDeviceSurfaceCharacteristics surfaceCharacteristics, uint32_t graphicsQueueFamilyIndex, uint32_t presentationQueueFamilyIndex, VkExtent2D windowExtent, SwapchainInfo *swapchainInfo, char **error);

void destroySwapchain(VkDevice device, VkSwapchainKHR swapchain);

#endif // MODELER_SWAPCHAIN_H
