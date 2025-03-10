#include <stdio.h>
#include <stdlib.h>

#include "physical_device.h"
#include "utils.h"
#include "vulkan_utils.h"

SuitabilityResult isPhysicalDeviceSuitable(PhysicalDeviceCharacteristics characteristics, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, char **error);
bool isPhysicalDeviceSurfaceSupportSuitable(PhysicalDeviceSurfaceCharacteristics characteristics);
bool getPhysicalDeviceCharacteristics(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, PhysicalDeviceCharacteristics *characteristics, char **error);

bool choosePhysicalDevice(VkInstance instance, VkSurfaceKHR surface,
	VkPhysicalDevice *physicalDevice, PhysicalDeviceCharacteristics *characteristics,
	PhysicalDeviceSurfaceCharacteristics *surfaceCharacteristics, char **error)
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

	VkPhysicalDevice *devices = malloc(sizeof(*devices) * deviceCount);
	if ((result = vkEnumeratePhysicalDevices(instance, &deviceCount, devices)) != VK_SUCCESS) {
		asprintf(error, "Failed to get physical devices: %s", string_VkResult(result));
		return false;
	}

	bool matchFound = false;
	for (uint32_t i = 0; !matchFound && i < deviceCount; ++i) {
		if (!getPhysicalDeviceCharacteristics(devices[i], surface, characteristics, error)) {
			free(devices);
			return false;
		}

		switch(isPhysicalDeviceSuitable(*characteristics, devices[i], surface, error)) {
		case SUITABILITY_ERROR:
			free(devices);
			return false;
		case SUITABILITY_SUITABLE:
			if (!getPhysicalDeviceSurfaceCharacteristics(devices[i], surface, surfaceCharacteristics, error)) {
				free(devices);
				return false;
			}

			if (isPhysicalDeviceSurfaceSupportSuitable(*surfaceCharacteristics)) {
				*physicalDevice = devices[i];
				matchFound = true;
			}
		}
	}

	free(devices);

	if (*physicalDevice == VK_NULL_HANDLE || !matchFound) {
		asprintf(error, "Failed to find a suitable GPU!");
		return false;
	}

	return true;
}

bool findQueueFamilyWithFlags(VkQueueFamilyProperties *queueFamilies, uint32_t queueFamilyCount, VkQueueFlags queueFlags, uint32_t *queueFamilyIndex)
{
	for (uint32_t i = 0; i < queueFamilyCount; ++i) {
		if (queueFamilies[i].queueFlags & queueFlags) {
			*queueFamilyIndex = i;
			return true;
		}
	}

	return false;
}

SuitabilityResult findQueueFamilyWithSurfaceSupport(uint32_t queueFamilyCount, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t *queueFamilyIndex, char **error)
{
	for (uint32_t i = 0; i < queueFamilyCount; ++i) {
		VkBool32 familyHasPresent = VK_FALSE;
		VkResult result;
		if ((result = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &familyHasPresent)) != VK_SUCCESS) {
			asprintf(error, "Failed to check for GPU surface support: %s", string_VkResult(result));
			return SUITABILITY_ERROR;
		}

		if (familyHasPresent == VK_TRUE) {
			*queueFamilyIndex = i;
			return SUITABILITY_SUITABLE;
		}
	}

	return SUITABILITY_UNSUITABLE;
}

bool isPhysicalDeviceSurfaceSupportSuitable(PhysicalDeviceSurfaceCharacteristics characteristics)
{
	return characteristics.formatCount && characteristics.presentModeCount &&
#if DRAW_WINDOW_BORDER
		(characteristics.capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR);
#elif defined(ANDROID)
		(characteristics.capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR);
#else
		(characteristics.capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR);
#endif /* DRAW_WINDOW_BORDER */
}

SuitabilityResult isPhysicalDeviceSuitable(PhysicalDeviceCharacteristics characteristics, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, char **error)
{
	bool hasProperties = characteristics.deviceProperties.limits.maxMemoryAllocationCount >= 1;
	if (!hasProperties) {
		return SUITABILITY_UNSUITABLE;
	}

	bool hasFeatures = characteristics.deviceFeatures.robustBufferAccess && characteristics.deviceFeatures.samplerAnisotropy;
	if (!hasFeatures) {
		return SUITABILITY_UNSUITABLE;
	}

	uint32_t queueFamilyIndex;
	if (!findQueueFamilyWithFlags(characteristics.queueFamilies, characteristics.queueFamilyCount, VK_QUEUE_GRAPHICS_BIT, &queueFamilyIndex)) {
		return SUITABILITY_UNSUITABLE;
	}

	switch (findQueueFamilyWithSurfaceSupport(characteristics.queueFamilyCount, physicalDevice, surface, &queueFamilyIndex, error)) {
	case SUITABILITY_ERROR:
		return SUITABILITY_ERROR;
	case SUITABILITY_UNSUITABLE:
		return SUITABILITY_UNSUITABLE;
	}

	const char* deviceExtension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
	if (!compareExtensions(&deviceExtension, 1, characteristics.extensions, characteristics.extensionCount)) {
		return SUITABILITY_UNSUITABLE;
	}

	return SUITABILITY_SUITABLE;
}

bool getPhysicalDeviceCharacteristics(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, PhysicalDeviceCharacteristics *characteristics, char **error)
{
	vkGetPhysicalDeviceProperties(physicalDevice, &characteristics->deviceProperties);

	vkGetPhysicalDeviceFeatures(physicalDevice, &characteristics->deviceFeatures);

	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &characteristics->queueFamilyCount, NULL);

	characteristics->queueFamilies = malloc(sizeof(*characteristics->queueFamilies) * characteristics->queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &characteristics->queueFamilyCount, characteristics->queueFamilies);

	VkResult result;

	if ((result = vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &characteristics->extensionCount, NULL)) != VK_SUCCESS) {
		asprintf(error, "Failed to get available device extension count: %s", string_VkResult(result));
		return false;
	}

	characteristics->extensions = malloc(sizeof(*characteristics->extensions) * characteristics->extensionCount);
	if ((result = vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &characteristics->extensionCount, characteristics->extensions)) != VK_SUCCESS) {
		asprintf(error, "Failed to get available device extensions: %s", string_VkResult(result));
		return false;
	}

	return true;
}

VkSampleCountFlagBits getMaxSampleCount(VkPhysicalDeviceProperties physicalDeviceProperties)
{
	VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
	if (counts & VK_SAMPLE_COUNT_64_BIT) {
		return VK_SAMPLE_COUNT_64_BIT;
	} else if (counts & VK_SAMPLE_COUNT_32_BIT) {
		return VK_SAMPLE_COUNT_32_BIT;
	} else if (counts & VK_SAMPLE_COUNT_16_BIT) {
		return VK_SAMPLE_COUNT_16_BIT;
	} else if (counts & VK_SAMPLE_COUNT_8_BIT) {
		return VK_SAMPLE_COUNT_8_BIT;
	} else if (counts & VK_SAMPLE_COUNT_4_BIT) {
		return VK_SAMPLE_COUNT_4_BIT;
	} else if (counts & VK_SAMPLE_COUNT_2_BIT) {
		return VK_SAMPLE_COUNT_2_BIT;
	} else {
		return VK_SAMPLE_COUNT_1_BIT;
	}
}

VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, VkFormat *formats, size_t formatCount, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (size_t i = 0; i < formatCount; ++i) {
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, formats[i], &formatProperties);

		VkFormatFeatureFlags featuresForTiling;
		switch (tiling) {
		case VK_IMAGE_TILING_OPTIMAL:
			featuresForTiling = formatProperties.optimalTilingFeatures;
			break;
		case VK_IMAGE_TILING_LINEAR:
			featuresForTiling = formatProperties.linearTilingFeatures;
			break;
		default:
			return VK_FORMAT_UNDEFINED;
		}

		if ((featuresForTiling & features) == features) {
			return formats[i];
		}
	}
}

void freePhysicalDeviceCharacteristics(PhysicalDeviceCharacteristics *characteristics)
{
	if (characteristics->queueFamilies) {
		free(characteristics->queueFamilies);
	}
	if (characteristics->extensions) {
		free(characteristics->extensions);
	}
}

bool getPhysicalDeviceSurfaceCharacteristics(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, PhysicalDeviceSurfaceCharacteristics *characteristics, char **error)
{
	VkResult result;

	if ((result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &characteristics->capabilities)) != VK_SUCCESS) {
		asprintf(error, "Failed to get physical device capabilities: %s", string_VkResult(result));
		return false;
	}

	if ((result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &characteristics->formatCount, NULL)) != VK_SUCCESS) {
		asprintf(error, "Failed to get physical device surface format count: %s", string_VkResult(result));
		return false;
	}

	characteristics->formats = malloc(sizeof(*characteristics->formats) * characteristics->formatCount);
	if ((result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &characteristics->formatCount, characteristics->formats)) != VK_SUCCESS) {
		asprintf(error, "Failed to get physical device surface formats: %s", string_VkResult(result));
		return false;
	}

	if ((result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &characteristics->presentModeCount, NULL)) != VK_SUCCESS) {
		asprintf(error, "Failed to get physical device surface present mode count: %s", string_VkResult(result));
		return false;
	}

	characteristics->presentModes = malloc(sizeof(*characteristics->presentModes) * characteristics->presentModeCount);
	if ((result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &characteristics->presentModeCount, characteristics->presentModes)) != VK_SUCCESS) {
		asprintf(error, "Failed to get physical device surface present modes: %s", string_VkResult(result));
		return false;
	}

	return true;
}

void freePhysicalDeviceSurfaceCharacteristics(PhysicalDeviceSurfaceCharacteristics *characteristics)
{
	if (characteristics->formats) {
		free(characteristics->formats);
	}
	if (characteristics->presentModes) {
		free(characteristics->presentModes);
	}
}
