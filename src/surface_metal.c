#ifndef VK_USE_PLATFORM_METAL_EXT
#define VK_USE_PLATFORM_METAL_EXT
#endif

#include "surface_metal.h"

VkSurfaceKHR createSurfaceMetal(VkInstance instance, const CAMetalLayer *layer)
{
    VkMetalSurfaceCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
    createInfo.pLayer = layer;
    
    VkSurfaceKHR surface;
    VkResult result;
    result = vkCreateMetalSurfaceEXT(instance, &createInfo, NULL, &surface);
    if (result != VK_SUCCESS) {
            printf("Failed to create surface!\n");
            return NULL;
    }
    
    return surface;
}