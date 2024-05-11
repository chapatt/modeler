#ifndef MODELER_MODELER_WAYLAND_H
#define MODELER_MODELER_WAYLAND_H

#include <pthread.h>

#include <wayland-client.h>
#include <wayland-client-protocol.h>

#include "../xdg-shell-client-protocol.h"

#include "modeler.h"
#include "queue.h"

typedef struct wayland_window_t {
	struct wl_display *display;
	struct wl_surface *surface;
	struct xdg_surface *xdgSurface;
	int fd;
} WaylandWindow;

pthread_t initVulkanWayland(struct wl_display *waylandDisplay, struct wl_surface *waylandSurface, struct xdg_surface *xdgSurface, WindowDimensions windowDimensions, Queue *inputQueue, int fd, char **error);

#endif /* MODELER_MODELER_WAYLAND_H */
