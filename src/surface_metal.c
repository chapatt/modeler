#ifndef VK_USE_PLATFORM_METAL_EXT
#define VK_USE_PLATFORM_METAL_EXT
#endif

#include <stdio.h>

#include "surface_metal.h"

bool createSurfaceMetal(VkInstance instance, const CAMetalLayer *layer, VkSurfaceKHR *surface, char **error)
{
	VkMetalSurfaceCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
	createInfo.pLayer = layer;

	VkResult result;
	if ((result = vkCreateMetalSurfaceEXT(instance, &createInfo, NULL, &surface)) != VK_SUCCESS) {
		asprintf(error, "Failed to create surface: %s", string_VkResult(result));
		return false;
	}

	return surface;
}
