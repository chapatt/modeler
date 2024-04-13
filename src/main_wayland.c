#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <linux/input-event-codes.h>
#include <wayland-client.h>
#include <wayland-client-protocol.h>
#include <wayland-cursor.h>

#include "../xdg-shell-client-protocol.h"

#include "queue.h"
#include "input_event.h"
#include "modeler.h"
#include "utils.h"

#include "modeler_wayland.h"

#define RESIZE_BORDER 10
#define MARGIN 25
#define DEFAULT_SURFACE_WIDTH 600
#define DEFAULT_SURFACE_HEIGHT 400
#define DEFAULT_ACTIVE_WIDTH  550 /* DEFAULT_SURFACE_WIDTH - MARGIN * 2 */
#define DEFAULT_ACTIVE_HEIGHT 350 /* DEFAULT_SURFACE_HEIGHT - MARGIN * 2 */
#define OFFSET_X 25
#define OFFSET_Y 15
#define CORNER_RADIUS 10

typedef enum window_region_t {
	CLIENT,
	CHROME,
	TOP,
	RIGHT,
	BOTTOM,
	LEFT,
	TOP_RIGHT,
	BOTTOM_RIGHT,
	BOTTOM_LEFT,
	TOP_LEFT
} WindowRegion;

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
	uint32_t pointerSerial;
	struct wl_surface *cursorSurface;
	struct wl_cursor_theme *cursorTheme;
	WindowDimensions windowDimensions;
	WindowRegion pointerRegion;
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
void setCursor(struct display *display, char *name);
void pointerEnterHandler(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y);
void pointerLeaveHandler(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface);
void pointerMotionHandler(void *data, struct wl_pointer *pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y);
void pointerButtonHandler(void *data, struct wl_pointer *pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
void pointerAxisHandler(void *data, struct wl_pointer *pointer, uint32_t time, uint32_t axis, wl_fixed_t value);
WindowRegion hitTest(struct display *display, int x, int y);
void hitTestAndSetCursor(struct display *display, wl_fixed_t x, wl_fixed_t y, bool debounce);
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
		.surfaceArea = {
			.width = DEFAULT_SURFACE_WIDTH,
			.height = DEFAULT_SURFACE_HEIGHT
		},
		.activeArea = {
			.offset.x = OFFSET_X,
			.offset.y = OFFSET_Y,
			.extent.width = DEFAULT_ACTIVE_WIDTH,
			.extent.height = DEFAULT_ACTIVE_HEIGHT
		},
		.cornerRadius = CORNER_RADIUS
	};

	connectDisplay(&display);
	configurePointer(&display);
	configureWmBase(&display);
	createWindow(&display);
	createRegions(&display);

	initializeQueue(&inputQueue);

	char *error;
	if (!initVulkanWayland(display.display, display.surface, display.windowDimensions, &inputQueue, threadPipe[1], &error)) {
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

	display->cursorTheme = wl_cursor_theme_load(NULL, 24, display->shm);
	display->cursorSurface = wl_compositor_create_surface(display->compositor);
	display->pointer = wl_seat_get_pointer(display->seat);
	wl_pointer_add_listener(display->pointer, &pointerListener, display);

	printf("Configured Wayland pointer\n");
}

void setCursor(struct display *display, char *name)
{
	struct wl_cursor *cursor = wl_cursor_theme_get_cursor(display->cursorTheme, name);
	struct wl_cursor_image *cursorImage = cursor->images[0];
	struct wl_buffer *cursorBuffer = wl_cursor_image_get_buffer(cursorImage);

	wl_surface_attach(display->cursorSurface, cursorBuffer, 0, 0);
	wl_surface_commit(display->cursorSurface);
	wl_pointer_set_cursor(display->pointer, display->pointerSerial, display->cursorSurface, cursorImage->hotspot_x, cursorImage->hotspot_y);
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
	wl_display_roundtrip(display->display);
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
	wl_region_add(
		display->inputRegion,
		display->windowDimensions.activeArea.offset.x - RESIZE_BORDER,
		display->windowDimensions.activeArea.offset.y - RESIZE_BORDER,
		display->windowDimensions.activeArea.extent.width + RESIZE_BORDER * 2,
		display->windowDimensions.activeArea.extent.height + RESIZE_BORDER * 2
	);

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

	// render here
}

static void xdgToplevelConfigureHandler(void *data, struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height, struct wl_array *states)
{
	printf("Got a xdg toplevel configure event\n");
	struct display *display = data;
	if (width == 0 && height == 0) {
		display->windowDimensions.activeArea.extent.width = DEFAULT_ACTIVE_WIDTH;
		display->windowDimensions.activeArea.extent.height = DEFAULT_ACTIVE_HEIGHT;
	} else {
		display->windowDimensions.activeArea.extent.width = width - MARGIN * 2;
		display->windowDimensions.activeArea.extent.height = height - MARGIN * 2;
	}
}

void pointerEnterHandler(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y)
{
	struct display *display = data;

	display->pointerSerial = serial;
	hitTestAndSetCursor(display, x, y, false);
}

void pointerLeaveHandler(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface)
{

}

void pointerMotionHandler(void *data, struct wl_pointer *pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
	struct display *display = data;
	hitTestAndSetCursor(display, x, y, true);
}

void hitTestAndSetCursor(struct display *display, wl_fixed_t x, wl_fixed_t y, bool debounce)
{
	WindowRegion region = hitTest(display, wl_fixed_to_double(x), wl_fixed_to_double(y));

	if (debounce && display->pointerRegion == region) {
		return;
	}

	display->pointerRegion = region;

	switch (region) {
	case CLIENT:
		setCursor(display, "left_ptr");
		break;
	case CHROME:
		setCursor(display, "left_ptr");
		break;
	case TOP:
		setCursor(display, "top_side");
		break;
	case RIGHT:
		setCursor(display, "right_side");
		break;
	case BOTTOM:
		setCursor(display, "bottom_side");
		break;
	case LEFT:
		setCursor(display, "left_side");
		break;
	case TOP_RIGHT:
		setCursor(display, "top_right_corner");
		break;
	case BOTTOM_RIGHT:
		setCursor(display, "bottom_right_corner");
		break;
	case BOTTOM_LEFT:
		setCursor(display, "bottom_left_corner");
		break;
	case TOP_LEFT:
		setCursor(display, "top_left_corner");
		break;
	}
}

void pointerButtonHandler(void *data, struct wl_pointer *pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
	struct display *display = data;

	switch (display->pointerRegion) {
	case CLIENT:
		break;
	case CHROME:
		if (button == BTN_LEFT && state == 1) {
			xdg_toplevel_move(display->xdgToplevel, display->seat, serial);
		}
		break;
	case TOP:
		xdg_toplevel_resize(display->xdgToplevel, display->seat, serial, 1);
		break;
	case RIGHT:
		xdg_toplevel_resize(display->xdgToplevel, display->seat, serial, 8);
		break;
	case BOTTOM:
		xdg_toplevel_resize(display->xdgToplevel, display->seat, serial, 2);
		break;
	case LEFT:
		xdg_toplevel_resize(display->xdgToplevel, display->seat, serial, 4);
		break;
	case TOP_RIGHT:
		xdg_toplevel_resize(display->xdgToplevel, display->seat, serial, 9);
		break;
	case BOTTOM_RIGHT:
		xdg_toplevel_resize(display->xdgToplevel, display->seat, serial, 10);
		break;
	case BOTTOM_LEFT:
		xdg_toplevel_resize(display->xdgToplevel, display->seat, serial, 6);
		break;
	case TOP_LEFT:
		xdg_toplevel_resize(display->xdgToplevel, display->seat, serial, 5);
		break;
	}
}

WindowRegion hitTest(struct display *display, int x, int y)
{
	printf("x: %d, y: %d\n", x, y);
#define CHROME_HEIGHT 20
	enum regionMask
	{
		_CLIENT = 0b00000,
		_LEFT = 0b00001,
		_RIGHT = 0b00010,
		_TOP = 0b00100,
		_BOTTOM = 0b01000,
		_CHROME = 0b10000
	};
	enum regionMask result = _CLIENT;
	VkExtent2D extent = display->windowDimensions.activeArea.extent;
	VkOffset2D offset = display->windowDimensions.activeArea.offset;
	printf("extent width: %d, extent height: %d\n", extent.width, extent.height);
	printf("offset x: %d, offset y: %d\n", offset.x, offset.y);

	if (x < offset.x) {
		printf("it's on the left\n");
		result |= _LEFT;
	}

	if (x >= extent.width + offset.x) {
		printf("it's on the right\n");
		result |= _RIGHT;
	}

	if (y < offset.y) {
		printf("it's on the top\n");
		result |= _TOP;
	}

	if (y >= extent.height + offset.y) {
		printf("it's on the bottom\n");
		result |= _BOTTOM;
	}

	if (!(result & (_LEFT | _RIGHT | _TOP | _BOTTOM)) && y < (offset.y + CHROME_HEIGHT)) {
		printf("it's in the chrome\n");
		result |= _CHROME;
	}

	if (result & _TOP)
	{
		if (result & _LEFT) return TOP_LEFT;
		if (result & _RIGHT) return TOP_RIGHT;
		return TOP;
	} else if (result & _BOTTOM) {
		if (result & _LEFT) return BOTTOM_LEFT;
		if (result & _RIGHT) return BOTTOM_RIGHT;
		return BOTTOM;
	} else if (result & _LEFT) {
		return LEFT;
	} else if (result & _RIGHT) {
		return RIGHT;
	} else if (result & _CHROME) {
		return CHROME;
	} else {
		return CLIENT;
	}
}

void pointerAxisHandler(void *data, struct wl_pointer *pointer, uint32_t time, uint32_t axis, wl_fixed_t value)
{

}

void handleFatalError(char *message)
{
	fprintf(stderr, "%s\n", message);
	exit(1);
}
