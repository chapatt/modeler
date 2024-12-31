#include <stdlib.h>

#include "image.h"
#include "command_buffer.h"
#include "utils.h"
#include "vulkan_utils.h"

bool createImage(VkDevice device, VmaAllocator allocator, VkExtent2D extent, VkFormat format, VkImageUsageFlagBits usage, uint32_t mipLevels, VkImage *image, VmaAllocation *allocation, char **error)
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
		.mipLevels = mipLevels,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = NULL,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};

	VmaAllocationCreateInfo allocationCreateInfo = {
		.usage = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	};

	VkResult result;
	if ((result = vmaCreateImage(allocator, &imageCreateInfo, &allocationCreateInfo, image, allocation, NULL)) != VK_SUCCESS) {
		asprintf(error, "Failed to create image: %s", string_VkResult(result));
		return false;
	}

	return true;
}

bool transitionImageLayout(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, char **error)
{
	VkCommandBuffer commandBuffer;
	if (!beginSingleTimeCommands(device, commandPool, &commandBuffer, error)) {
		return false;
	}

	VkImageMemoryBarrier barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.oldLayout = oldLayout,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.subresourceRange.baseMipLevel = 0,
		.subresourceRange.levelCount = mipLevels,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount = 1,
		.srcAccessMask = 0,
		.dstAccessMask = 0
	};

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
 		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else {
		asprintf(error, "Unsupported layout transition.\n");
		return false;
	}

	vkCmdPipelineBarrier(
 		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, NULL,
		0, NULL,
		1, &barrier
	);

	if (!endSingleTimeCommands(device, commandPool, queue, commandBuffer, error)) {
		return false;
	}

	return true;
}

bool copyBufferToImage(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t mipLevels, char **error)
{
	VkCommandBuffer commandBuffer;
	if (!beginSingleTimeCommands(device, commandPool, &commandBuffer, error)) {
		return false;
	}

	uint32_t level0Width = height;
	uint32_t level0Height = height;
	VkBufferImageCopy *regions = malloc(sizeof(*regions) * mipLevels);

	regions[0] = (VkBufferImageCopy) {
		.bufferOffset = 0,
		.bufferRowLength = width,
		.bufferImageHeight = height,
		.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.imageSubresource.mipLevel = 0,
		.imageSubresource.baseArrayLayer = 0,
		.imageSubresource.layerCount = 1,
		.imageOffset = {0, 0, 0},
		.imageExtent = {
			level0Width,
			level0Height,
			1
		}
	};

	VkDeviceSize heightOffset = 0;
	for (uint32_t i = 1; i < mipLevels; ++i) {
		uint32_t extentWidth = level0Width >> i;
		uint32_t extentHeight = level0Height >> i;
		uint32_t bufferOffset = width * heightOffset + level0Width;
		heightOffset += extentHeight;

		regions[i] = (VkBufferImageCopy) {
			.bufferOffset = bufferOffset * 4,
			.bufferRowLength = width,
			.bufferImageHeight = height,
			.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.imageSubresource.mipLevel = i,
			.imageSubresource.baseArrayLayer = 0,
			.imageSubresource.layerCount = 1,
			.imageOffset = {0, 0, 0},
			.imageExtent = {
				extentWidth,
				extentHeight,
				1
			}
		};
	}

	vkCmdCopyBufferToImage(
		commandBuffer,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		mipLevels,
		regions
	);

	if (!endSingleTimeCommands(device, commandPool, queue, commandBuffer, error)) {
		return false;
	}

	return true;
}

void destroyImage(VmaAllocator allocator, VkImage image, VmaAllocation allocation)
{
	vmaDestroyImage(allocator, image, allocation);
}
