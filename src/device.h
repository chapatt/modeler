#ifndef MODELER_DEVICE_H
#define MODELER_DEVICE_H

#include <stdbool.h>
#include <vulkan/vulkan.h>

bool createDevice(VkPhysicalDevice physicalDevice, VkDevice *device, char **error);

#endif /* MODELER_DEVICE_H */