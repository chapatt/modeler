#include "allocator.h"
#include "utils.h"
#include "vulkan_utils.h"

bool createAllocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator *allocator, char **error)
{
	VmaAllocatorCreateInfo allocatorCreateInfo = {
		.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT,
		.vulkanApiVersion = VK_API_VERSION_1_0,
		.physicalDevice = physicalDevice,
		.device = device,
		.instance = instance,
		.pVulkanFunctions = NULL
	};
	VkResult result;
	if ((result = vmaCreateAllocator(&allocatorCreateInfo, allocator)) != VK_SUCCESS) {
		asprintf(error, "Failed to create allocator: %s", string_VkResult(result));
		return false;
	}

	return true;
}

void destroyAllocator(VmaAllocator allocator)
{
	vmaDestroyAllocator(allocator);
}