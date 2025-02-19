#include <stdio.h>

#include "modeler_metal.h"
#include "surface.h"
#include "vulkan_utils.h"

bool createSurface(VkInstance instance, void *platformWindow, VkSurfaceKHR *surface, char **error)
{
	MetalWindow *window = (MetalWindow *) platformWindow;

	VkMetalSurfaceCreateInfoEXT createInfo = {
		.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT,
		.pLayer = window->surfaceLayer
	};

	VkResult result;
	if ((result = vkCreateMetalSurfaceEXT(instance, &createInfo, NULL, surface)) != VK_SUCCESS) {
		asprintf(error, "Failed to create surface: %s", string_VkResult(result));
		return false;
	}

	return surface;
}
