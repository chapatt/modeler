#ifndef MODELER_PHYSICAL_DEVICE_H
#define MODELER_PHYSICAL_DEVICE_H

#include <stdbool.h>
#include <vulkan/vulkan.h>

typedef enum suitability_result_t {
	SUITABILITY_ERROR = -1,
	SUITABILITY_UNSUITABLE = 0,
	SUITABILITY_SUITABLE = 1
} SuitabilityResult;

typedef struct physical_device_characteristics_t {
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;
	uint32_t queueFamilyCount;
	VkQueueFamilyProperties *queueFamilies;
	uint32_t extensionCount;
	VkExtensionProperties *extensions;
} PhysicalDeviceCharacteristics;

typedef struct physical_device_surface_characteristics_t {
	VkSurfaceCapabilitiesKHR capabilities;
	uint32_t formatCount;
	VkSurfaceFormatKHR *formats;
	uint32_t presentModeCount;
	VkPresentModeKHR *presentModes;
} PhysicalDeviceSurfaceCharacteristics;

bool findQueueFamilyWithFlags(VkQueueFamilyProperties *queueFamilies, uint32_t queueFamilyCount, VkQueueFlags queueFlags, uint32_t *queueFamilyIndex);
VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, VkFormat *formats, size_t formatCount, VkImageTiling tiling, VkFormatFeatureFlags features);
SuitabilityResult findQueueFamilyWithSurfaceSupport(uint32_t queueFamilyCount, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t *queueFamilyIndex, char **error);
bool choosePhysicalDevice(VkInstance instance, VkSurfaceKHR surface,
	VkPhysicalDevice *physicalDevice, PhysicalDeviceCharacteristics *characteristics,
	PhysicalDeviceSurfaceCharacteristics *surfaceCharacteristics, char **error);
bool getPhysicalDeviceSurfaceCharacteristics(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, PhysicalDeviceSurfaceCharacteristics *characteristics, char **error);
VkSampleCountFlagBits getMaxSampleCount(VkPhysicalDeviceProperties physicalDeviceProperties);
void freePhysicalDeviceCharacteristics(PhysicalDeviceCharacteristics *characteristics);
void freePhysicalDeviceSurfaceCharacteristics(PhysicalDeviceSurfaceCharacteristics *characteristics);

#endif /* MODELER_PHYSICAL_DEVICE_H */