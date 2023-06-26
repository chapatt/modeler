#ifndef MODELER_SURFACE_WAYLAND_H
#define MODELER_SURFACE_WAYLAND_H

#ifndef VK_USE_PLATFORM_WAYLAND_KHR
#define VK_USE_PLATFORM_WAYLAND_KHR
#endif

#include <stdbool.h>
#include <vulkan/vulkan.h>

bool createSurfaceWayland(VkInstance instance, struct wl_display *waylandDisplay, struct wl_surface *waylandSurface, VkSurfaceKHR *surface, char **error);

#endif /* MODELER_SURFACE_WAYLAND_H */
