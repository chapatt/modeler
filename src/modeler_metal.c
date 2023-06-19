#ifndef VK_USE_PLATFORM_METAL_EXT
#define VK_USE_PLATFORM_METAL_EXT
#endif

#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_metal.h>

#include "modeler_metal.h"
#include "instance.h"
#include "surface_metal.h"
#include "physical_device.h"
#include "device.h"

bool initVulkanMetal(void *surfaceLayer, char **error)
{
        const char *instanceExtensions[] = {
                VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
                VK_KHR_SURFACE_EXTENSION_NAME,
                VK_EXT_METAL_SURFACE_EXTENSION_NAME
        };
        VkInstance instance;
        if (!createInstance(instanceExtensions, 3, &instance, error)) {
                return false;
        }
        VkSurfaceKHR surface = createSurfaceMetal(instance, surfaceLayer);
        VkPhysicalDevice physicalDevice = choosePhysicalDevice(instance, surface);
        VkDevice device = createDevice(physicalDevice);

        return true;
}
