#ifndef MODELER_MODELER_WAYLAND_H
#define MODELER_MODELER_WAYLAND_H

#ifndef VK_USE_PLATFORM_WAYLAND_KHR
#define VK_USE_PLATFORM_WAYLAND_KHR
#endif

#include <vulkan/vulkan.h>

#include <stdbool.h>

bool initVulkanWayland(struct wl_display *waylandDisplay, struct wl_surface *waylandSurface, char **error);

#endif /* MODELER_MODELER_WAYLAND_H */
