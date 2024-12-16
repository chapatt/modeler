#include "image.h"
#include "utils.h"
#include "vulkan_utils.h"

bool createImage(VkDevice device, VmaAllocator allocator, VkExtent2D extent, VkFormat format, VkImage *image, VmaAllocation *allocation, char **error)
{
	VkImageCreateInfo imageCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = format,
		.extent.width = extent.width,
		.extent.height = extent.height,
		.extent.depth = 1,
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
		.sharingMode = 0,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = NULL,
		.initialLayout = 0
	};

	VmaAllocationCreateInfo allocationCreateInfo = {
		.usage = VMA_MEMORY_USAGE_GPU_ONLY
	};

	VkResult result;
	if ((result = vmaCreateImage(allocator, &imageCreateInfo, &allocationCreateInfo, image, allocation, NULL)) != VK_SUCCESS) {
		asprintf(error, "Failed to create image: %s", string_VkResult(result));
		return false;
	}

	return true;
}

void destroyImage(VmaAllocator allocator, VkImage image, VmaAllocation allocation)
{
	vmaDestroyImage(allocator, image, allocation);
}