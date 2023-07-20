#ifndef MODELER_DEVICE_H
#define MODELER_DEVICE_H

#include <stdbool.h>
#include <vulkan/vulkan.h>

#include "physical_device.h"

typedef struct queue_info_t {
	VkQueue graphicsQueue;
	VkQueue presentationQueue;
	uint32_t graphicsQueueFamilyIndex;
	uint32_t presentationQueueFamilyIndex;
} QueueInfo;

bool createDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
	PhysicalDeviceCharacteristics characteristics, PhysicalDeviceSurfaceCharacteristics surfaceCharacteristics,
	VkDevice *device, QueueInfo *queueInfo, char **error);

void destroyDevice(VkDevice device);

#endif /* MODELER_DEVICE_H */