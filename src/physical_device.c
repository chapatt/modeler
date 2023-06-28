#include <stdio.h>
#include <stdlib.h>

#include "physical_device.h"
#include "utils.h"
#include "vulkan_utils.h"

typedef enum suitability_result_t {
	SUITABILITY_ERROR = -1,
	SUITABILITY_UNSUITABLE = 0,
	SUITABILITY_SUITABLE = 1
} SuitabilityResult;

SuitabilityResult isPhysicalDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, char **error);

bool choosePhysicalDevice(VkInstance instance, VkSurfaceKHR surface, VkPhysicalDevice *physicalDevice, char **error)
{
	VkResult result;

	uint32_t deviceCount = 0;
	if ((result = vkEnumeratePhysicalDevices(instance, &deviceCount, NULL)) != VK_SUCCESS) {
		asprintf(error, "Failed to get physical device count: %s", string_VkResult(result));
		return false;
	}
    
	if (deviceCount == 0) {
		asprintf(error, "Failed to find a Physical Device");
		return false;
	}
    
	VkPhysicalDevice *devices = (VkPhysicalDevice *) malloc(sizeof(VkPhysicalDevice) * deviceCount);
	if ((result = vkEnumeratePhysicalDevices(instance, &deviceCount, devices)) != VK_SUCCESS) {
		asprintf(error, "Failed to get physical devices: %s", string_VkResult(result));
		return false;
	}

	bool matchFound = false;
	for (uint32_t i = 0; !matchFound && i < deviceCount; ++i) {
		switch (isPhysicalDeviceSuitable(devices[i], surface, error)) {
		case SUITABILITY_ERROR:
			return false;
		case SUITABILITY_SUITABLE:
			*physicalDevice = devices[i];
			matchFound = true;
		}
	}

	free(devices);

	if (*physicalDevice == VK_NULL_HANDLE) {
		asprintf(error, "Failed to find a suitable GPU!");
		return false;
	}
    
	return true;
}

SuitabilityResult isPhysicalDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, char **error)
{
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
	bool hasProperties = deviceProperties.limits.maxMemoryAllocationCount >= 1;
	if (!hasProperties) {
		return SUITABILITY_UNSUITABLE;
	}

	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
	bool hasFeatures = deviceFeatures.robustBufferAccess;
	if (!hasFeatures) {
		return SUITABILITY_UNSUITABLE;
	}

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);
    
	VkQueueFamilyProperties *queueFamilies = (VkQueueFamilyProperties *) malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies);
    
	VkQueueFlags compiledFlags = 0;
	bool hasPresent = false;
	for (uint32_t i = 0; i < queueFamilyCount; ++i) {
		compiledFlags |= queueFamilies[i].queueFlags;

		VkBool32 familyHasPresent = VK_FALSE;
		VkResult result;
		if ((result = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &familyHasPresent)) != VK_SUCCESS) {
			asprintf(error, "Failed to check for GPU surface support: %s", string_VkResult(result));
			return SUITABILITY_ERROR;
		}
		hasPresent = hasPresent || familyHasPresent == VK_TRUE;
	}
	free(queueFamilies);
	bool hasQueues = compiledFlags & VK_QUEUE_GRAPHICS_BIT;
	if (!hasPresent || !hasQueues) {
		return SUITABILITY_UNSUITABLE;
	}

	bool hasExtensions = false;
	const char* deviceExtension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
	switch (areDeviceExtensionsSupported(physicalDevice, &deviceExtension, 1, error)) {
	case SUPPORT_ERROR:
		return SUITABILITY_ERROR;
	case SUPPORT_SUPPORTED:
		hasExtensions = true;
	}
	if (!hasExtensions) {
		return SUITABILITY_UNSUITABLE;
	}

	VkResult result;

	VkSurfaceCapabilitiesKHR capabilities;
	if ((result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities)) != VK_SUCCESS) {
		asprintf(error, "Failed to get physical device capabilities: %s", string_VkResult(result));
		return SUPPORT_ERROR;
	}

	uint32_t formatCount = 0;
	if ((result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL)) != VK_SUCCESS) {
		asprintf(error, "Failed to get physical device surface format count: %s", string_VkResult(result));
		return SUPPORT_ERROR;
	}

	VkSurfaceFormatKHR *formats = (VkSurfaceFormatKHR *) malloc(sizeof(VkSurfaceFormatKHR) * formatCount);
	if ((result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats)) != VK_SUCCESS) {
		asprintf(error, "Failed to get physical device surface formats: %s", string_VkResult(result));
		return SUPPORT_ERROR;
	}

	uint32_t presentModeCount = 0;
	if ((result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, NULL)) != VK_SUCCESS) {
		asprintf(error, "Failed to get physical device surface present mode count: %s", string_VkResult(result));
		return SUPPORT_ERROR;
	}

	VkPresentModeKHR *presentModes = (VkPresentModeKHR *) malloc(sizeof(VkPresentModeKHR) * formatCount);
	if ((result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes)) != VK_SUCCESS) {
		asprintf(error, "Failed to get physical device surface present modes: %s", string_VkResult(result));
		return SUPPORT_ERROR;
	}

	bool hasSwapchain = formatCount && presentModeCount;
	if (!hasSwapchain) {
		return SUITABILITY_UNSUITABLE;
	}

	return SUITABILITY_SUITABLE;
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