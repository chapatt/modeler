#include <stdlib.h>

#include "utils.h"
#include "vulkan_utils.h"

#include "command_buffer.h"

bool createCommandBuffers(VkDevice device, SwapchainInfo swapchainInfo, VkCommandPool commandPool, VkCommandBuffer **commandBuffers, char **error)
{
	*commandBuffers = malloc(sizeof(*commandBuffers) * swapchainInfo.imageCount);
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = NULL,
		.commandPool = commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = swapchainInfo.imageCount
	};

	VkResult result;
	if ((result = vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, *commandBuffers)) != VK_SUCCESS) {
		asprintf(error, "Failed to create command buffers: %s", string_VkResult(result));
		return false;
	}

	return true;
}

bool beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkCommandBuffer *commandBuffer, char **error)
{
	VkResult result;

	VkCommandBufferAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandPool = commandPool,
		.commandBufferCount = 1,
	};

	if ((result = vkAllocateCommandBuffers(device, &allocInfo, commandBuffer)) != VK_SUCCESS) {
		asprintf(error, "Failed to allocate command buffers: %s", string_VkResult(result));
		return false;
	}

	VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};

	if ((result = vkBeginCommandBuffer(*commandBuffer, &beginInfo)) != VK_SUCCESS) {
		asprintf(error, "Failed to begin command buffer: %s", string_VkResult(result));
		return false;
	}
}

bool endSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer, char **error)
{
	VkResult result;

	if ((result = vkEndCommandBuffer(commandBuffer)) != VK_SUCCESS) {
		asprintf(error, "Failed to end command buffer: %s", string_VkResult(result));
		return false;
	}

	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer,
	};

	if ((result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE)) != VK_SUCCESS) {
		asprintf(error, "Failed to submit queue: %s", string_VkResult(result));
		return false;
	}
	if ((result = vkQueueWaitIdle(queue)) != VK_SUCCESS) {
		asprintf(error, "Failed to wait for queue to be idle: %s", string_VkResult(result));
		return false;
	}

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);

	return true;
}

void freeCommandBuffers(VkDevice device, VkCommandPool commandPool, VkCommandBuffer *commandBuffers, uint32_t count)
{
	vkFreeCommandBuffers(device, commandPool, count, commandBuffers);
}
