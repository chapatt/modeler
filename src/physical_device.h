#ifndef MODELER_PHYSICAL_DEVICE_H
#define MODELER_PHYSICAL_DEVICE_H

#include <stdbool.h>
#include <vulkan/vulkan.h>

typedef enum support_result_t {
	SUPPORT_ERROR = -1,
	SUPPORT_UNSUPPORTED = 0,
	SUPPORT_SUPPORTED = 1
} SupportResult;

bool choosePhysicalDevice(VkInstance instance, VkSurfaceKHR surface, VkPhysicalDevice *physicalDevice, char **error);
SupportResult areDeviceExtensionsSupported(VkPhysicalDevice physicalDevice, const char **extensions, size_t extensionCount, char **error);

#endif /* MODELER_PHYSICAL_DEVICE_H */