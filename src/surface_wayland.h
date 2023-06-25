#ifndef MODELER_SURFACE_WAYLAND_H
#define MODELER_SURFACE_WAYLAND_H

#ifndef VK_USE_PLATFORM_WAYLAND_KHR
#define VK_USE_PLATFORM_WAYLAND_KHR
#endif

#include <vulkan/vulkan.h>

VkSurfaceKHR createSurfaceWayland(VkInstance instance, struct wl_display *waylandDisplay, struct wl_surface *waylandSurface);

#endif /* MODELER_SURFACE_WAYLAND_H */
