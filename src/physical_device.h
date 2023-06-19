#ifndef MODELER_PHYSICAL_DEVICE_H
#define MODELER_PHYSICAL_DEVICE_H

#include <vulkan/vulkan.h>

VkPhysicalDevice choosePhysicalDevice(VkInstance instance, VkSurfaceKHR surface);

#endif /* MODELER_PHYSICAL_DEVICE_H */