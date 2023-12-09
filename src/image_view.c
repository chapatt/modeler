#include <stdlib.h>

#include "image_view.h"
#include "utils.h"
#include "vulkan_utils.h"

bool createImageViews(VkDevice device, SwapchainInfo *swapchainInfo, VkImageView **imageViews, char **error) {
	*imageViews = malloc(sizeof(*imageViews) * swapchainInfo->imageCount);
	VkImageViewCreateInfo createInfoTemplate = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = swapchainInfo->surfaceFormat.format,
		.components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
		.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.subresourceRange.baseMipLevel = 0,
		.subresourceRange.levelCount = 1,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount = 1
	};

	for (uint32_t i = 0; i < swapchainInfo->imageCount; ++i) {
		VkImageViewCreateInfo createInfo = createInfoTemplate;
		createInfo.image = swapchainInfo->images[i];

		VkResult result;
		if ((result = vkCreateImageView(device, &createInfo, NULL, *imageViews + i)) != VK_SUCCESS) {
			asprintf(error, "Failed to create image views: %s", string_VkResult(result));
			return false;
		}
	}

	return true;
}

void destroyImageViews(VkDevice device, VkImageView *imageViews, uint32_t count) {
	for (uint32_t i = 0; i < count; ++i) {
		vkDestroyImageView(device, imageViews[i], NULL);
	}
	free(imageViews);
}