#ifndef MODELER_MODELER_WAYLAND_H
#define MODELER_MODELER_WAYLAND_H

#ifndef VK_USE_PLATFORM_WAYLAND_KHR
#define VK_USE_PLATFORM_WAYLAND_KHR
#endif

#include <pthread.h>

#include "queue.h"

typedef struct wayland_window_t {
	struct wl_display *display;
	struct wl_surface *surface;
} WaylandWindow;

pthread_t initVulkanWayland(struct wl_display *waylandDisplay, struct wl_surface *waylandSurface, Queue *inputQueue, char **error);

#endif /* MODELER_MODELER_WAYLAND_H */
