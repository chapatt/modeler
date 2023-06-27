#include <stdio.h>
#include <stdlib.h>

#include "surface_wayland.h"
#include "utils.h"
#include "vulkan_utils.h"

bool createSurfaceWayland(VkInstance instance, struct wl_display *waylandDisplay, struct wl_surface *waylandSurface, VkSurfaceKHR *surface, char **error)
{
	VkWaylandSurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
	createInfo.display = waylandDisplay;
	createInfo.surface = waylandSurface;

	VkResult result;
	if ((result = vkCreateWaylandSurfaceKHR(instance, &createInfo, NULL, surface)) != VK_SUCCESS) {
		asprintf(error, "Failed to create surface: %s", string_VkResult(result));
		return false;
	}

	return surface;
}
