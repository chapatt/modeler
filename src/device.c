#include <stdio.h>
#include <stdlib.h>

#include "device.h"
#include "utils.h"
#include "vulkan_utils.h"

bool createDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
	PhysicalDeviceCharacteristics characteristics, PhysicalDeviceSurfaceCharacteristics surfaceCharacteristics,
	VkDevice *device, VkQueue *graphicsQueue, VkQueue *presentationQueue, char **error)
{
	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.enabledLayerCount = 0;

	uint32_t graphicsQueueFamilyIndex;
	if (!findQueueFamilyWithFlags(characteristics.queueFamilies, characteristics.queueFamilyCount, VK_QUEUE_GRAPHICS_BIT, &graphicsQueueFamilyIndex)) {
		asprintf(error, "Selected device does not have graphics queue support.");
		return false;
	}

	uint32_t presentationQueueFamilyIndex;
	switch (findQueueFamilyWithSurfaceSupport(characteristics.queueFamilyCount, physicalDevice, surface, &presentationQueueFamilyIndex, error)) {
	case SUITABILITY_UNSUITABLE:
		asprintf(error, "Selected device does not have presentation queue support.");
	case SUITABILITY_ERROR:
		return false;
	}

	VkDeviceQueueCreateInfo graphicsQueueCreateInfo = {};
	graphicsQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	graphicsQueueCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
	graphicsQueueCreateInfo.queueCount = 1;
	float graphicsQueuePriority = 1.0f;
	graphicsQueueCreateInfo.pQueuePriorities = &graphicsQueuePriority;

	VkDeviceQueueCreateInfo presentationQueueCreateInfo = {};
	presentationQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	presentationQueueCreateInfo.queueFamilyIndex = presentationQueueFamilyIndex;
	presentationQueueCreateInfo.queueCount = 1;
	float presentationQueuePriority = 1.0f;
	presentationQueueCreateInfo.pQueuePriorities = &presentationQueuePriority;

	VkDeviceQueueCreateInfo queueCreateInfos[2] = {graphicsQueueCreateInfo, presentationQueueCreateInfo};

	if (graphicsQueueFamilyIndex == presentationQueueFamilyIndex) {
		createInfo.queueCreateInfoCount = 1;
		createInfo.pQueueCreateInfos = &graphicsQueueCreateInfo;
	} else {
		createInfo.queueCreateInfoCount = 2;
		createInfo.pQueueCreateInfos = queueCreateInfos;
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};
	createInfo.pEnabledFeatures = &deviceFeatures;

	const char* requiredExtensions[2] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
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
    
	vkGetDeviceQueue(*device, graphicsQueueFamilyIndex, 0, &graphicsQueue);
	vkGetDeviceQueue(*device, presentationQueueFamilyIndex, 0, &presentationQueue);

	return true;
}

void destroyDevice(VkDevice device)
{
	vkDestroyDevice(device, NULL);
}