#include <stdio.h>
#include <stdlib.h>

#include "surface_wayland.h"

VkSurfaceKHR createSurfaceWayland(VkInstance instance, struct wl_display *waylandDisplay, struct wl_surface *waylandSurface)
{
	VkWaylandSurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
	createInfo.display = waylandDisplay;
	createInfo.surface = waylandSurface;

	VkSurfaceKHR surface;
	VkResult result;

	if (vkCreateWaylandSurfaceKHR(instance, &createInfo, NULL, &surface) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create surface!\n");
		exit(EXIT_FAILURE);
	}

	return surface;
}
