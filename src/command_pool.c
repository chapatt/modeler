#include "utils.h"
#include "vulkan_utils.h"

#include "command_pool.h"

bool createCommandPool(VkDevice device, QueueInfo queueInfo, VkCommandPool *commandPool, char **error)
{
	VkCommandPoolCreateInfo commandPoolCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = NULL,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = queueInfo.graphicsQueueFamilyIndex
	};

	VkResult result;
	if ((result = vkCreateCommandPool(device, &commandPoolCreateInfo, NULL, commandPool)) != VK_SUCCESS) {
		asprintf(error, "Failed to create command pool: %s", string_VkResult(result));
		return false;
	}

	return true;
}

void destroyCommandPool(VkDevice device, VkCommandPool commandPool)
{
	vkDestroyCommandPool(device, commandPool, NULL);
}