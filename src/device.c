#include <stdio.h>
#include <stdlib.h>

#include "device.h"
#include "utils.h"
#include "vulkan_utils.h"

bool createDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
	PhysicalDeviceCharacteristics characteristics,
	VkDevice *device, QueueInfo *queueInfo, char **error)
{
	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.enabledLayerCount = 0;

	if (!findQueueFamilyWithFlags(characteristics.queueFamilies, characteristics.queueFamilyCount, VK_QUEUE_GRAPHICS_BIT, &queueInfo->graphicsQueueFamilyIndex)) {
		asprintf(error, "Selected device does not have graphics queue support.");
		return false;
	}

	switch (findQueueFamilyWithSurfaceSupport(characteristics.queueFamilyCount, physicalDevice, surface, &queueInfo->presentationQueueFamilyIndex, error)) {
	case SUITABILITY_UNSUITABLE:
		asprintf(error, "Selected device does not have presentation queue support.");
	case SUITABILITY_ERROR:
		return false;
	}

	VkDeviceQueueCreateInfo graphicsQueueCreateInfo = {};
	graphicsQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	graphicsQueueCreateInfo.queueFamilyIndex = queueInfo->graphicsQueueFamilyIndex;
	graphicsQueueCreateInfo.queueCount = 1;
	float graphicsQueuePriority = 1.0f;
	graphicsQueueCreateInfo.pQueuePriorities = &graphicsQueuePriority;

	VkDeviceQueueCreateInfo presentationQueueCreateInfo = {};
	presentationQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	presentationQueueCreateInfo.queueFamilyIndex = queueInfo->presentationQueueFamilyIndex;
	presentationQueueCreateInfo.queueCount = 1;
	float presentationQueuePriority = 1.0f;
	presentationQueueCreateInfo.pQueuePriorities = &presentationQueuePriority;

	VkDeviceQueueCreateInfo queueCreateInfos[2] = {graphicsQueueCreateInfo, presentationQueueCreateInfo};

	if (queueInfo->graphicsQueueFamilyIndex == queueInfo->presentationQueueFamilyIndex) {
		createInfo.queueCreateInfoCount = 1;
		createInfo.pQueueCreateInfos = &graphicsQueueCreateInfo;
	} else {
		createInfo.queueCreateInfoCount = 2;
		createInfo.pQueueCreateInfos = queueCreateInfos;
	}

	VkPhysicalDeviceFeatures deviceFeatures = {
		.samplerAnisotropy = VK_TRUE
	};
	createInfo.pEnabledFeatures = &deviceFeatures;

	const char* requiredExtensions[3] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};
	createInfo.enabledExtensionCount = 1;
	const char* optionalExtension = "VK_KHR_portability_subset";
	if (compareExtensions(&optionalExtension, 1, characteristics.extensions, characteristics.extensionCount)) {
		requiredExtensions[createInfo.enabledExtensionCount] = optionalExtension;
		createInfo.enabledExtensionCount++;
	}
	createInfo.ppEnabledExtensionNames = requiredExtensions;

	VkResult result;
	if ((result = vkCreateDevice(physicalDevice, &createInfo, NULL, device)) != VK_SUCCESS) {
		asprintf(error, "Failed to create logical device: %s", string_VkResult(result));
		return false;
	}

	vkGetDeviceQueue(*device, queueInfo->graphicsQueueFamilyIndex, 0, &queueInfo->graphicsQueue);
	vkGetDeviceQueue(*device, queueInfo->presentationQueueFamilyIndex, 0, &queueInfo->presentationQueue);

	return true;
}

void destroyDevice(VkDevice device)
{
	vkDestroyDevice(device, NULL);
}
