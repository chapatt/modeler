#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>
#include <wayland-client-protocol.h>

#include "../xdg-shell-client-protocol.h"
#include "modeler_wayland.h"

struct display {
	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_registry_listener registryListener;
	struct wl_compositor *compositor;
	struct wl_surface *surface;
	struct xdg_surface_listener xdgSurfaceListener;
	struct xdg_wm_base *xdgWmBase;
	struct xdg_surface *xdgSurface;
	struct xdg_toplevel *xdgToplevel;
};

static void globalRegistryHandler(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version);
static void globalRegistryRemover(void *data, struct wl_registry *registry, uint32_t id);
void connectDisplay(struct display *display);
void createWindow(struct display *display);
void destroyWindow(struct display *display);
void disconnectDisplay(struct display *display);
static void xdgSurfaceConfigure(void *data, struct xdg_surface *xdg_surface, uint32_t serial);
void handleFatalError(char *message);

int main(int argc, char **argv)
{
	struct display display;

	connectDisplay(&display);
	createWindow(&display);

	char *error;
	if (!initVulkanWayland(display.display, display.surface, &error)) {
		handleFatalError(error);
	}

	while (wl_display_dispatch(display.display) != -1) {
		/* This space deliberately left blank */
	}

	disconnectDisplay(&display);
	destroyWindow(&display);
}

void destroyWindow(struct display *display)
{
	wl_surface_destroy(display->surface);
	printf("Destroyed Wayland surface\n");
}

void disconnectDisplay(struct display *display)
{
	wl_registry_destroy(display->registry);
	printf("Destroyed Wayland registry\n");

	wl_display_disconnect(display->display);
	printf("Disconnected from Wayland display\n");
}

void connectDisplay(struct display *display)
{
	display->display = wl_display_connect(NULL);
	if (display->display == NULL) {
		handleFatalError("Can't connect to Wayland display\n");
	}
	printf("Connected to Wayland display\n");

	if ((display->registry = wl_display_get_registry(display->display)) == NULL) {
		handleFatalError("Can't get a Wayland registry\n");
	}
	printf("Got a Wayland registry\n");

	display->registryListener.global = globalRegistryHandler;
	display->registryListener.global_remove = globalRegistryRemover;
	if (wl_registry_add_listener(display->registry, &(display->registryListener), display))
		printf("Can't add Wayland registry listener\n");
	printf("Added Wayland registry listener\n");
	wl_display_dispatch(display->display);
	wl_display_roundtrip(display->display);
}

void createWindow(struct display *display)
{
	if (display->compositor == NULL) {
		handleFatalError("Can't find Wayland compositor\n");
	}
	printf("Found Wayland compositor\n");

	display->surface = wl_compositor_create_surface(display->compositor);
	if (display->surface == NULL) {
		handleFatalError("Can't create Wayland surface\n");
	}
	printf("Created Wayland surface\n");

	display->xdgSurface = xdg_wm_base_get_xdg_surface(display->xdgWmBase, display->surface);
	display->xdgSurfaceListener.configure = xdgSurfaceConfigure;
	xdg_surface_add_listener(display->xdgSurface, &display->xdgSurfaceListener, display);
	display->xdgToplevel = xdg_surface_get_toplevel(display->xdgSurface);
	xdg_toplevel_set_title(display->xdgToplevel, "Modeler");
	wl_surface_commit(display->surface);
}

static void globalRegistryHandler(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version)
{
	struct display *display = data;

	printf("Got a Wayland registry event for %s ID %d\n", interface, id);
	if (strcmp(interface, "wl_compositor") == 0) {
		display->compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 1);
		if (display->compositor == NULL) {
			handleFatalError("Can't bind Wayland compositor\n");
		}
		printf("Bound Wayland compositor\n");
	} else if (strcmp(interface, "xdg_wm_base") == 0) {
		display->xdgWmBase = wl_registry_bind(registry, id, &xdg_wm_base_interface, 1);
		if (display->xdgWmBase == NULL) {
			handleFatalError("Can't bind xdg wm base\n");
		}
		printf("Bound xdg wm base\n");
	}
}

static void globalRegistryRemover(void *data, struct wl_registry *registry, uint32_t id)
{
	printf("Got a Wayland registry remove event for %d\n", id);
}

static void xdgSurfaceConfigure(void *data, struct xdg_surface *xdg_surface, uint32_t serial)
{
	struct display *display = data;
	xdg_surface_ack_configure(xdg_surface, serial);
}

void handleFatalError(char *message)
{
	fprintf(stderr, "%s\n", message);
	exit(1);
}
