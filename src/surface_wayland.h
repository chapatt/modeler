#ifndef MODELER_SURFACE_WAYLAND_H
#define MODELER_SURFACE_WAYLAND_H

#include <stdbool.h>
#include <vulkan/vulkan.h>

bool createSurfaceWayland(VkInstance instance, struct wl_display *waylandDisplay, struct wl_surface *waylandSurface, VkSurfaceKHR *surface, char **error);

#endif /* MODELER_SURFACE_WAYLAND_H */
