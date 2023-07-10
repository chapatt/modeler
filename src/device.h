#ifndef MODELER_DEVICE_H
#define MODELER_DEVICE_H

#include <stdbool.h>
#include <vulkan/vulkan.h>

#include "physical_device.h"

bool createDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
	PhysicalDeviceCharacteristics characteristics, PhysicalDeviceSurfaceCharacteristics surfaceCharacteristics,
	VkDevice *device, VkQueue *graphicsQueue, VkQueue *presentationQueue,
	uint32_t *graphicsQueueFamilyIndex, uint32_t *presentationQueueFamilyIndex, char **error);

#endif /* MODELER_DEVICE_H */