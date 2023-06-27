#include <stdio.h>
#include <stdlib.h>

#include "device.h"
#include "utils.h"
#include "vulkan_utils.h"

typedef enum support_result_t {
	SUPPORT_ERROR = -1,
	SUPPORT_UNSUPPORTED = 0,
	SUPPORT_SUPPORTED = 1
} SupportResult;

SupportResult areDeviceExtensionsSupported(VkPhysicalDevice physicalDevice, const char **extensions, size_t extensionCount, char **error);
uint32_t findFirstMatchingFamily(VkPhysicalDevice physicalDevice, VkQueueFlags flag);

bool createDevice(VkPhysicalDevice physicalDevice, VkDevice *device, char **error)
{
	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = findFirstMatchingFamily(physicalDevice, VK_QUEUE_GRAPHICS_BIT);
	queueCreateInfo.queueCount = 1;
	float queuePriority = 1.0f;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	VkPhysicalDeviceFeatures deviceFeatures = {};
    
	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = &queueCreateInfo;
	createInfo.queueCreateInfoCount = 1;
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledLayerCount = 0;

	const char* deviceExtension = "VK_KHR_portability_subset";
	switch (areDeviceExtensionsSupported(physicalDevice, &deviceExtension, 1, error)) {
	case SUPPORT_ERROR:
		return false;
	case SUPPORT_SUPPORTED:
		createInfo.ppEnabledExtensionNames = &deviceExtension;
		createInfo.enabledExtensionCount = 1;
	}

    
	VkResult result;
	if ((result = vkCreateDevice(physicalDevice, &createInfo, NULL, device)) != VK_SUCCESS) {
		asprintf(error, "Failed to create logical device: %s", string_VkResult(result));
		return false;
	}
    
	VkQueue queue;
	vkGetDeviceQueue(*device, queueCreateInfo.queueFamilyIndex, 0, &queue);

	return true;
}

void destroyDevice(VkDevice device)
{
	vkDestroyDevice(device, NULL);
}

SupportResult areDeviceExtensionsSupported(VkPhysicalDevice physicalDevice, const char **extensions, size_t extensionCount, char **error)
{
	VkResult result;

	uint32_t availableExtensionCount;
	if ((result = vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &availableExtensionCount, NULL)) != VK_SUCCESS) {
		asprintf(error, "Failed to get available device extension count: %s", string_VkResult(result));
		return SUPPORT_ERROR;
	}

	VkExtensionProperties *availableExtensions = (VkExtensionProperties *) malloc(sizeof(VkExtensionProperties) * availableExtensionCount);
	if ((result = vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &availableExtensionCount, availableExtensions)) != VK_SUCCESS) {
		asprintf(error, "Failed to get available device extensions: %s", string_VkResult(result));
		return SUPPORT_ERROR;
	}

	bool match = compareExtensions(extensions, extensionCount, availableExtensions, availableExtensionCount);
	free(availableExtensions);

	return match ? SUPPORT_SUPPORTED : SUPPORT_UNSUPPORTED;
}

/* Not guaranteed to find a match (0 returned if none are found) */
uint32_t findFirstMatchingFamily(VkPhysicalDevice physicalDevice, VkQueueFlags flags)
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);

	VkQueueFamilyProperties *queueFamilies = (VkQueueFamilyProperties *) malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies);

	VkQueueFlags match = 0;
	for (uint32_t i = 0; i < queueFamilyCount; ++i) {
		if (queueFamilies[i].queueFlags & flags) {
			match = i;
			break;
		}
	}
	free(queueFamilies);
    
	return match;
}