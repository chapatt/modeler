#ifndef MODELER_PHYSICAL_DEVICE_H
#define MODELER_PHYSICAL_DEVICE_H

#include <stdbool.h>
#include <vulkan/vulkan.h>

bool choosePhysicalDevice(VkInstance instance, VkSurfaceKHR surface, VkPhysicalDevice *physicalDevice, char **error);

#endif /* MODELER_PHYSICAL_DEVICE_H */