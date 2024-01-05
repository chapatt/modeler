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
