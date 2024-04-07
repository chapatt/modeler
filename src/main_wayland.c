#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <wayland-client.h>
#include <wayland-client-protocol.h>
#include <wayland-cursor.h>

#include "../xdg-shell-client-protocol.h"

#include "queue.h"
#include "input_event.h"
#include "modeler.h"
#include "utils.h"

#include "modeler_wayland.h"

struct display {
	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_registry_listener registryListener;
	struct wl_compositor *compositor;
	struct wl_surface *surface;
	struct wl_region *inputRegion;
	struct wl_region *opaqueRegion;
	struct wl_seat *seat;
	struct wl_shm *shm;
	struct wl_pointer *pointer;
	struct xdg_wm_base_listener xdgWmBaseListener;
	struct xdg_surface_listener xdgSurfaceListener;
	struct xdg_toplevel_listener xdgToplevelListener;
	struct xdg_wm_base *xdgWmBase;
	struct xdg_surface *xdgSurface;
	struct xdg_toplevel *xdgToplevel;
	struct wl_surface *cursorSurface;
	struct wl_cursor_image *cursorImage;
	WindowDimensions windowDimensions;
};

static void globalRegistryHandler(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version);
static void globalRegistryRemover(void *data, struct wl_registry *registry, uint32_t id);
void connectDisplay(struct display *display);
void configureWmBase(struct display *display);
void createWindow(struct display *display);
void createRegions(struct display *display);
void destroyRegions(struct display *display);
void destroyWindow(struct display *display);
void disconnectDisplay(struct display *display);
static void xdgWmBasePingHandler(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial);
static void xdgSurfaceConfigureHandler(void *data, struct xdg_surface *xdg_surface, uint32_t serial);
static void xdgToplevelConfigureHandler(void *data, struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height, struct wl_array *states);
void configurePointer(struct display *display);
void pointerEnterHandler(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y);
void pointerLeaveHandler(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface);
void pointerMotionHandler(void *data, struct wl_pointer *pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y);
void pointerButtonHandler(void *data, struct wl_pointer *pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
void pointerAxisHandler(void *data, struct wl_pointer *pointer, uint32_t time, uint32_t axis, wl_fixed_t value);
void handleFatalError(char *message);

const struct wl_pointer_listener pointerListener = {
	.enter = pointerEnterHandler,
	.leave = pointerLeaveHandler,
	.motion = pointerMotionHandler,
	.button = pointerButtonHandler,
	.axis = pointerAxisHandler
};

int main(int argc, char **argv)
{
	struct display display;
	Queue inputQueue;
	int threadPipe[2];
	int epollFd;
	if (pipe(threadPipe)) {
		handleFatalError("Failed to create pipe");
	}
	display.windowDimensions = (WindowDimensions) {
		.activeArea = {
			.offset.x = 40,
			.offset.y = 40,
			.extent.width = 600,
			.extent.height = 400
		},
		.cornerRadius = 10,
		.marginWidth = 10
	};

	connectDisplay(&display);
	configurePointer(&display);
	configureWmBase(&display);
	createWindow(&display);
	createRegions(&display);

	initializeQueue(&inputQueue);

	char *error;
	if (!initVulkanWayland(display.display, display.surface, &display.windowDimensions, &inputQueue, threadPipe[1], &error)) {
		handleFatalError(error);
	}

	int wlFd = wl_display_get_fd(display.display);
	if ((epollFd = epoll_create1(0)) == -1) {
		char *error;
		asprintf(&error, "Failed to create epoll instance: %s", strerror(errno));
		handleFatalError(error);
	}
	struct epoll_event epollConfigurationEvent1 = {
		.events = EPOLLIN,
		.data.fd = threadPipe[0]
	};
	epoll_ctl(epollFd, EPOLL_CTL_ADD, threadPipe[0], &epollConfigurationEvent1);
	struct epoll_event epollConfigurationEvent2 = {
		.events = EPOLLIN,
		.data.fd = wlFd
	};
	epoll_ctl(epollFd, EPOLL_CTL_ADD, wlFd, &epollConfigurationEvent2);

	struct epoll_event epollEventBuffer;
	for (;;) {
		if (epoll_wait(epollFd, &epollEventBuffer, 1, -1) == -1) {
			char *error;
			asprintf(&error, "Failed to wait for epoll: %s", strerror(errno));
			handleFatalError(error);
		}

		if (epollEventBuffer.data.fd == wlFd) {
			wl_display_dispatch(display.display);
		} else if (epollEventBuffer.data.fd == threadPipe[0]) {
			handleFatalError(error);
		}
	}

	close(epollFd);

	destroyRegions(&display);
	destroyWindow(&display);
	disconnectDisplay(&display);
}

void destroyRegions(struct display *display)
{
	wl_region_destroy(display->inputRegion);
	printf("Destroyed Wayland region\n");

	wl_region_destroy(display->opaqueRegion);
	printf("Destroyed Wayland region\n");
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
	if (wl_registry_add_listener(display->registry, &(display->registryListener), display)) {
		printf("Can't add Wayland registry listener\n");
	}
	printf("Added Wayland registry listener\n");
	wl_display_dispatch(display->display);
	wl_display_roundtrip(display->display);
}

void configureWmBase(struct display *display)
{
	if (display->xdgWmBase == NULL) {
		handleFatalError("Can't find xdg wm base\n");
	}

	display->xdgWmBaseListener.ping = xdgWmBasePingHandler;
	xdg_wm_base_add_listener(display->xdgWmBase, &display->xdgWmBaseListener, display);
}

void configurePointer(struct display *display)
{
	if (display->seat == NULL || display->shm == NULL) {
		handleFatalError("Can't find Wayland seat or shared memory\n");
	}

	struct wl_cursor_theme *cursorTheme = wl_cursor_theme_load(NULL, 24, display->shm);
	struct wl_cursor *cursor = wl_cursor_theme_get_cursor(cursorTheme, "left_ptr");
	display->cursorImage = cursor->images[0];
	struct wl_buffer *cursorBuffer = wl_cursor_image_get_buffer(display->cursorImage);

	display->cursorSurface = wl_compositor_create_surface(display->compositor);
	wl_surface_attach(display->cursorSurface, cursorBuffer, 0, 0);
	wl_surface_commit(display->cursorSurface);

	display->pointer = wl_seat_get_pointer(display->seat);
	wl_pointer_add_listener(display->pointer, &pointerListener, display);

	printf("Configured Wayland pointer\n");
}

void createWindow(struct display *display)
{
	if (display->compositor == NULL) {
		handleFatalError("Can't find Wayland compositor\n");
	}

	display->surface = wl_compositor_create_surface(display->compositor);
	if (display->surface == NULL) {
		handleFatalError("Can't create Wayland surface\n");
	}
	printf("Created Wayland surface\n");

	display->xdgSurface = xdg_wm_base_get_xdg_surface(display->xdgWmBase, display->surface);
	display->xdgSurfaceListener.configure = xdgSurfaceConfigureHandler;
	xdg_surface_add_listener(display->xdgSurface, &display->xdgSurfaceListener, display);
	display->xdgToplevel = xdg_surface_get_toplevel(display->xdgSurface);
	display->xdgToplevelListener.configure = xdgToplevelConfigureHandler;
	xdg_toplevel_add_listener(display->xdgToplevel, &display->xdgToplevelListener, display);
	xdg_toplevel_set_title(display->xdgToplevel, "Modeler");
	wl_surface_commit(display->surface);
}

void createRegions(struct display *display)
{
	if (display->compositor == NULL) {
		handleFatalError("Can't find Wayland compositor\n");
	}
	printf("Found Wayland compositor\n");

	display->inputRegion = wl_compositor_create_region(display->compositor);
	if (display->inputRegion == NULL) {
		handleFatalError("Can't create Wayland region\n");
	}
	printf("Created Wayland region\n");
	wl_region_add(display->inputRegion, 0, 0, 100, 100);

	display->opaqueRegion = wl_compositor_create_region(display->compositor);
	if (display->opaqueRegion == NULL) {
		handleFatalError("Can't create Wayland region\n");
	}
	printf("Created Wayland region\n");
	wl_region_add(
		display->opaqueRegion,
		display->windowDimensions.activeArea.offset.x + display->windowDimensions.cornerRadius,
		display->windowDimensions.activeArea.offset.y,
		display->windowDimensions.activeArea.extent.width - (2 * display->windowDimensions.cornerRadius),
		display->windowDimensions.activeArea.extent.height
	);
	wl_region_add(
		display->opaqueRegion,
		display->windowDimensions.activeArea.offset.x,
		display->windowDimensions.activeArea.offset.y + display->windowDimensions.cornerRadius,
		display->windowDimensions.activeArea.extent.width,
		display->windowDimensions.activeArea.extent.height - (2 * display->windowDimensions.cornerRadius)
	);

	wl_surface_set_input_region(display->surface, display->inputRegion);
	wl_surface_set_opaque_region(display->surface, display->opaqueRegion);
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
	} else if (strcmp(interface, "wl_shm") == 0) {
		display->shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
		if (display->shm == NULL) {
			handleFatalError("Can't bind Wayland shared memory\n");
		}
		printf("Bound Wayland shared memory\n");
	} else if (strcmp(interface, "wl_seat") == 0) {
		display->seat = wl_registry_bind(registry, id, &wl_seat_interface, 1);
		if (display->seat == NULL) {
			handleFatalError("Can't bind Wayland seat\n");
		}
		printf("Bound Wayland seat\n");
	}
}

static void globalRegistryRemover(void *data, struct wl_registry *registry, uint32_t id)
{
	printf("Got a Wayland registry remove event for %d\n", id);
}

static void xdgWmBasePingHandler(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial)
{
	printf("Got a xdg wm base ping event\n");
	struct display *display = data;
	xdg_wm_base_pong(display->xdgWmBase, serial);
}

static void xdgSurfaceConfigureHandler(void *data, struct xdg_surface *xdg_surface, uint32_t serial)
{
	printf("Got a xdg surface configure event\n");
	struct display *display = data;
	xdg_surface_ack_configure(xdg_surface, serial);
}

static void xdgToplevelConfigureHandler(void *data, struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height, struct wl_array *states)
{
	printf("Got a xdg toplevel configure event\n");
	struct display *display = data;
	display->windowDimensions.activeArea.extent.width = width;
	display->windowDimensions.activeArea.extent.height = height;
}

void pointerEnterHandler(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y)
{
	struct display *display = data;
	wl_pointer_set_cursor(pointer, serial, display->cursorSurface, display->cursorImage->hotspot_x, display->cursorImage->hotspot_y);
}

void pointerLeaveHandler(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface)
{

}

void pointerMotionHandler(void *data, struct wl_pointer *pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y)
{

}

void pointerButtonHandler(void *data, struct wl_pointer *pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{

}

void pointerAxisHandler(void *data, struct wl_pointer *pointer, uint32_t time, uint32_t axis, wl_fixed_t value)
{

}

void handleFatalError(char *message)
{
	fprintf(stderr, "%s\n", message);
	exit(1);
}
