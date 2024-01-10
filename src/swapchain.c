#include <stdint.h>
#include <stdlib.h>

#include "swapchain.h"
#include "utils.h"
#include "vulkan_utils.h"

const VkExtent2D NULL_EXTENT = {};

VkSurfaceFormatKHR chooseSwapchainSurfaceFormat(VkSurfaceFormatKHR *surfaceFormats, uint32_t surfaceFormatCount);
VkPresentModeKHR chooseSwapchainPresentMode(VkPresentModeKHR *presentModes, uint32_t presentModeCount);
VkExtent2D chooseSwapchainExtent(VkSurfaceCapabilitiesKHR capabilities, VkExtent2D windowExtent, char **error);

bool createSwapchain(VkDevice device, VkSurfaceKHR surface, PhysicalDeviceSurfaceCharacteristics surfaceCharacteristics, uint32_t graphicsQueueFamilyIndex, uint32_t presentationQueueFamilyIndex, VkExtent2D windowExtent, VkSwapchainKHR oldSwapchain, SwapchainInfo *swapchainInfo, char **error)
{
	swapchainInfo->surfaceFormat = chooseSwapchainSurfaceFormat(surfaceCharacteristics.formats, surfaceCharacteristics.formatCount);
#ifdef ENABLE_VSYNC
	swapchainInfo->presentMode = VK_PRESENT_MODE_FIFO_KHR;
#else
	swapchainInfo->presentMode = chooseSwapchainPresentMode(surfaceCharacteristics.presentModes, surfaceCharacteristics.presentModeCount);
#endif /* ENABLE_VSYNC */
	swapchainInfo->extent = chooseSwapchainExtent(surfaceCharacteristics.capabilities, windowExtent, error);
	if (swapchainInfo->extent.width == 0 || swapchainInfo->extent.height == 0) {
		return false;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.imageFormat = swapchainInfo->surfaceFormat.format;
	createInfo.imageColorSpace = swapchainInfo->surfaceFormat.colorSpace;
	createInfo.imageExtent = swapchainInfo->extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.preTransform = surfaceCharacteristics.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = swapchainInfo->presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = oldSwapchain;

	swapchainInfo->imageCount = surfaceCharacteristics.capabilities.minImageCount + 1;
	if (surfaceCharacteristics.capabilities.maxImageCount > 0 && swapchainInfo->imageCount > surfaceCharacteristics.capabilities.maxImageCount) {
		swapchainInfo->imageCount = surfaceCharacteristics.capabilities.maxImageCount;
	}
	createInfo.minImageCount = swapchainInfo->imageCount;

	uint32_t queueFamilyIndexes[] = {graphicsQueueFamilyIndex, presentationQueueFamilyIndex};

	if (graphicsQueueFamilyIndex != presentationQueueFamilyIndex) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndexes;
	} else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	VkResult result;
	if ((result = vkCreateSwapchainKHR(device, &createInfo, NULL, &swapchainInfo->swapchain)) != VK_SUCCESS) {
		asprintf(error, "Failed to create swapchain: %s", string_VkResult(result));
		return false;
	}

	if (oldSwapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(device, oldSwapchain, NULL);
	}

	vkGetSwapchainImagesKHR(device, swapchainInfo->swapchain, &swapchainInfo->imageCount, NULL);
	swapchainInfo->images = malloc(sizeof(*swapchainInfo->images) * swapchainInfo->imageCount);
	vkGetSwapchainImagesKHR(device, swapchainInfo->swapchain, &swapchainInfo->imageCount, swapchainInfo->images);

	return true;
}

void destroySwapchain(VkDevice device, VkSwapchainKHR swapchain)
{
	vkDestroySwapchainKHR(device, swapchain, NULL);
}

VkSurfaceFormatKHR chooseSwapchainSurfaceFormat(VkSurfaceFormatKHR *surfaceFormats, uint32_t surfaceFormatCount) {
	for (uint32_t i = 0; i < surfaceFormatCount; ++i) {
		if (surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return surfaceFormats[i];
		}
	}

	return surfaceFormats[0];
}

VkPresentModeKHR chooseSwapchainPresentMode(VkPresentModeKHR *presentModes, uint32_t presentModeCount) {
	for (uint32_t i = 0; i < presentModeCount; ++i) {
		if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			return presentModes[i];
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapchainExtent(VkSurfaceCapabilitiesKHR capabilities, VkExtent2D windowExtent, char **error) {
	if (capabilities.currentExtent.width != UINT32_MAX) {
		return capabilities.currentExtent;
	} else {
		if (windowExtent.width < capabilities.minImageExtent.width ||
			windowExtent.width > capabilities.maxImageExtent.width ||
			windowExtent.height < capabilities.minImageExtent.height ||
			windowExtent.height > capabilities.maxImageExtent.height)
		{
			asprintf(error, "Failed to select valid swap extent");
			return NULL_EXTENT;
		}

		return windowExtent;
	}
}
