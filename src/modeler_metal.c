#ifndef VK_USE_PLATFORM_METAL_EXT
#define VK_USE_PLATFORM_METAL_EXT
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_metal.h>

#include "modeler_metal.h"
#include "instance.h"

VkSurfaceKHR createSurfaceMetal(VkInstance instance, const CAMetalLayer *layer);

void initVulkanMetal(void *surfaceLayer)
{
        printf("metal layer pointer: %ld\n", (long) surfaceLayer);
    
        VkInstance instance = createInstance(&VK_EXT_METAL_SURFACE_EXTENSION_NAME, 1);
        VkSurfaceKHR surface = createSurface(instance, surfaceLayer);
        VkPhysicalDevice physicalDevice = choosePhysicalDevice(instance, surface);
        VkDevice device = createLogicalDevice(physicalDevice);
}
