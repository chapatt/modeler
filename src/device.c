#include <stdio.h>
#include <stdlib.h>

#include "device.h"
#include "utils.h"

bool areDeviceExtensionsSupported(VkPhysicalDevice physicalDevice, const char **extensions, size_t extensionCount);
uint32_t findFirstMatchingFamily(VkPhysicalDevice physicalDevice, VkQueueFlags flag);

VkDevice createDevice(VkPhysicalDevice physicalDevice)
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
	if (areDeviceExtensionsSupported(physicalDevice, &deviceExtension, 1)) {
		createInfo.ppEnabledExtensionNames = &deviceExtension;
		createInfo.enabledExtensionCount = 1;
	}

    
	VkDevice device;
	if (vkCreateDevice(physicalDevice, &createInfo, NULL, &device) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create logical device!\n");
		exit(EXIT_FAILURE);
	}
    
	VkQueue queue;
	vkGetDeviceQueue(device, queueCreateInfo.queueFamilyIndex, 0, &queue);

	return device;
}

void destroyDevice(VkDevice device)
{
	vkDestroyDevice(device, NULL);
}

bool areDeviceExtensionsSupported(VkPhysicalDevice physicalDevice, const char **extensions, size_t extensionCount)
{
	uint32_t availableExtensionCount;
	if (vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &availableExtensionCount, NULL) != VK_SUCCESS) {
		fprintf(stderr, "Failed to get available device extension count!\n");
		exit(EXIT_FAILURE);
	}

	VkExtensionProperties *availableExtensions = (VkExtensionProperties *) malloc(sizeof(VkExtensionProperties) * availableExtensionCount);
	if (vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &availableExtensionCount, availableExtensions) != VK_SUCCESS) {
		fprintf(stderr, "Failed to get available device extensions!\n");
		exit(EXIT_FAILURE);
	}

	bool match = compareExtensions(extensions, extensionCount, availableExtensions, availableExtensionCount);
	free(availableExtensions);

	return match;
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