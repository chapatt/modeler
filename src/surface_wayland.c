#include <stdio.h>
#include <stdlib.h>

#include "modeler_wayland.h"
#include "surface.h"
#include "utils.h"
#include "vulkan_utils.h"

bool createSurface(VkInstance instance, void *platformWindow, VkSurfaceKHR *surface, char **error)
{
	WaylandWindow *window = (WaylandWindow *) platformWindow;

	VkWaylandSurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
	createInfo.display = window->display;
	createInfo.surface =window->surface;

	VkResult result;
	if ((result = vkCreateWaylandSurfaceKHR(instance, &createInfo, NULL, surface)) != VK_SUCCESS) {
		asprintf(error, "Failed to create surface: %s", string_VkResult(result));
		return false;
	}

	return surface;
}
