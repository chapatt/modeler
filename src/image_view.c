#include <stdlib.h>

#include "image_view.h"
#include "utils.h"
#include "vulkan_utils.h"

bool createImageView(VkDevice device, VkImage image, VkFormat format, VkImageView *imageView, char **error)
{
	VkImageViewCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = format,
		.components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
		.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.subresourceRange.baseMipLevel = 0,
		.subresourceRange.levelCount = 1,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount = 1,
		.image = image
	};

	VkResult result;
	if ((result = vkCreateImageView(device, &createInfo, NULL, imageView)) != VK_SUCCESS) {
		asprintf(error, "Failed to create image views: %s", string_VkResult(result));
		return false;
	}

	return true;
}

bool createImageViews(VkDevice device, VkImage *images, uint32_t imageCount, VkFormat format, VkImageView *imageViews, char **error)
{
	for (uint32_t i = 0; i < imageCount; ++i) {
		if (!createImageView(device, images[i], format, imageViews + i, error)) {
			return false;
		}
	}

	return true;
}

void destroyImageView(VkDevice device, VkImageView imageView)
{
	vkDestroyImageView(device, imageView, NULL);
}


void destroyImageViews(VkDevice device, VkImageView *imageViews, uint32_t count)
{
	for (uint32_t i = 0; i < count; ++i) {
		destroyImageView(device, imageViews[i]);
	}
}
