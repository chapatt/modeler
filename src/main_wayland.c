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

#define CHROME_HEIGHT 20
#define RESIZE_BORDER 10
#define MARGIN 25
#define DEFAULT_SURFACE_WIDTH 600 /* DEFAULT_ACTIVE_WIDTH + MARGIN * 2 */
#define DEFAULT_SURFACE_HEIGHT 400 /* DEFAULT_ACTIVE_HEIGHT + MARGIN * 2 */
#define DEFAULT_ACTIVE_WIDTH  550
#define DEFAULT_ACTIVE_HEIGHT 350
#define OFFSET_X 25
#define OFFSET_Y 15
#define CORNER_RADIUS 10

typedef enum window_region_t {
	UNKNOWN_REGION,
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

typedef struct output_info_t {
	uint32_t id;
	struct wl_output *output;
	uint32_t scale;
} OutputInfo;

struct display {
	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_registry_listener registryListener;
	struct wl_compositor *compositor;
	struct wl_surface *surface;
	struct wl_seat *seat;
	struct wl_shm *shm;
	struct wl_pointer *pointer;
	OutputInfo **outputInfos;
	size_t outputInfoCount;
	OutputInfo **activeOutputInfos;
	size_t activeOutputInfoCount;
	struct xdg_wm_base *xdgWmBase;
	struct xdg_surface *xdgSurface;
	struct xdg_toplevel *xdgToplevel;
	struct wl_surface *cursorSurface;
	struct wl_cursor_theme *cursorTheme;
	struct wl_output_listener outputListener;
	struct wl_surface_listener surfaceListener;
	struct xdg_wm_base_listener xdgWmBaseListener;
	struct xdg_surface_listener xdgSurfaceListener;
	struct xdg_toplevel_listener xdgToplevelListener;
	struct wl_pointer_listener pointerListener;
	uint32_t pointerSerial;
	WindowDimensions windowDimensions;
	WindowRegion pointerRegion;
	VkOffset2D pointerPosition;
	Queue inputQueue;
	int threadPipe[2];
	bool vulkanInitialized;
};

static void registryGlobalHandler(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version);
static void registryGlobalRemoveHandler(void *data, struct wl_registry *registry, uint32_t id);
static void connectDisplay(struct display *display);
static void configureWmBase(struct display *display);
static void configureOutput(struct display *display, struct wl_output *output, uint32_t id);
static void createWindow(struct display *display);
static void setUpRegions(struct display *display);
static void destroyCursor(struct display *display);
static void destroyWindow(struct display *display);
static void disconnectDisplay(struct display *display);
static void configurePointer(struct display *display);
static void loadCursorTheme(struct display *display);
static void setCursor(struct display *display, char *name);
static void setCursorFromWindowRegion(struct display *display, WindowRegion region);
static void surfaceEnterHandler(void *data, struct wl_surface *surface, struct wl_output *output);
static void surfaceLeaveHandler(void *data, struct wl_surface *surface, struct wl_output *output);
static void xdgWmBasePingHandler(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial);
static void xdgSurfaceConfigureHandler(void *data, struct xdg_surface *xdg_surface, uint32_t serial);
static void xdgToplevelConfigureHandler(void *data, struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height, struct wl_array *states);
static void outputGeometryHandler(void *data, struct wl_output *output, int32_t x, int32_t y, int32_t physical_width, int32_t physical_height, enum wl_output_subpixel subpixel, const char *make, const char *model, enum wl_output_transform transform);
static void outputModeHandler(void *data, struct wl_output *output, enum wl_output_mode mode, int32_t width, int32_t height, int32_t refresh);
static void outputDoneHandler(void *data, struct wl_output *output);
static void outputScaleHandler(void *data, struct wl_output *output, int32_t factor);
static void outputNameHandler(void *data, struct wl_output *output, const char *name);
static void outputDescriptionHandler(void *data, struct wl_output *output, const char *description);
static void pointerEnterHandler(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y);
static void pointerLeaveHandler(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface);
static void pointerMotionHandler(void *data, struct wl_pointer *pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y);
static void pointerButtonHandler(void *data, struct wl_pointer *pointer, uint32_t serial, uint32_t time, uint32_t button, enum wl_pointer_button_state state);
static void pointerAxisHandler(void *data, struct wl_pointer *pointer, uint32_t time, uint32_t axis, wl_fixed_t value);
static void scaleWindowDimensions(WindowDimensions *windowDimensions, uint scale);
static void setSurfaceScale(struct display *display);
static WindowRegion hitTest(struct display *display, int x, int y);
static void hitTestAndSetCursor(struct display *display, int x, int y, bool debounce);
static int findIndexOfOutputInfoWithGlobalId(struct display *display, uint32_t id);
static size_t findIndexOfOutputInfoWithOutput(struct display *display, struct wl_output *output);
static size_t findIndexOfActiveOutputInfoWithOutput(struct display *display, struct wl_output *output);
static void enqueueResizeEvent(struct display *display);
static void handleFatalError(char *message);

struct wl_output_listener outputListener = {
	.geometry = outputGeometryHandler,
	.mode = outputModeHandler,
	.done = outputDoneHandler,
	.scale = outputScaleHandler,
	.name = outputNameHandler,
	.description = outputDescriptionHandler
};

int main(int argc, char **argv)
{
	struct display display = {};
	int epollFd;
	if (pipe(display.threadPipe)) {
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
		.cornerRadius = CORNER_RADIUS,
		.scale = 1.0f
	};

	initializeQueue(&display.inputQueue);

	connectDisplay(&display);
	configurePointer(&display);
	configureWmBase(&display);
	createWindow(&display);
	setUpRegions(&display);

	char *error;

	int wlFd = wl_display_get_fd(display.display);
	if ((epollFd = epoll_create1(0)) == -1) {
		char *error;
		asprintf(&error, "Failed to create epoll instance: %s", strerror(errno));
		handleFatalError(error);
	}
	struct epoll_event epollConfigurationEvent1 = {
		.events = EPOLLIN,
		.data.fd = display.threadPipe[0]
	};
	epoll_ctl(epollFd, EPOLL_CTL_ADD, display.threadPipe[0], &epollConfigurationEvent1);
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
		} else if (epollEventBuffer.data.fd == display.threadPipe[0]) {
			handleFatalError(error);
		}
	}

	close(epollFd);

	destroyCursor(&display);
	destroyWindow(&display);
	disconnectDisplay(&display);
}

static void destroyCursor(struct display *display)
{
	wl_cursor_theme_destroy(display->cursorTheme);
}

static void destroyWindow(struct display *display)
{
	wl_surface_destroy(display->surface);
	printf("Destroyed Wayland surface\n");
}

static void disconnectDisplay(struct display *display)
{
	wl_registry_destroy(display->registry);
	printf("Destroyed Wayland registry\n");

	wl_display_disconnect(display->display);
	printf("Disconnected from Wayland display\n");
}

static void connectDisplay(struct display *display)
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

	display->registryListener.global = registryGlobalHandler;
	display->registryListener.global_remove = registryGlobalRemoveHandler;
	if (wl_registry_add_listener(display->registry, &(display->registryListener), display)) {
		printf("Can't add Wayland registry listener\n");
	}
	printf("Added Wayland registry listener\n");
	wl_display_dispatch(display->display);
	wl_display_roundtrip(display->display);
}

static void configureWmBase(struct display *display)
{
	if (display->xdgWmBase == NULL) {
		handleFatalError("Can't find xdg wm base\n");
	}

	display->xdgWmBaseListener.ping = xdgWmBasePingHandler;
	xdg_wm_base_add_listener(display->xdgWmBase, &display->xdgWmBaseListener, display);
}

static void configurePointer(struct display *display)
{
	if (display->seat == NULL || display->shm == NULL) {
		handleFatalError("Can't find Wayland seat or shared memory\n");
	}

	display->cursorSurface = wl_compositor_create_surface(display->compositor);
	loadCursorTheme(display);

	display->pointer = wl_seat_get_pointer(display->seat);
	display->pointerListener = (struct wl_pointer_listener) {
		.enter = pointerEnterHandler,
		.leave = pointerLeaveHandler,
		.motion = pointerMotionHandler,
		.button = pointerButtonHandler,
		.axis = pointerAxisHandler
	};
	wl_pointer_add_listener(display->pointer, &display->pointerListener, display);

	printf("Configured Wayland pointer\n");
}

static void loadCursorTheme(struct display *display)
{
	if (display->cursorTheme) {
		wl_cursor_theme_destroy(display->cursorTheme);
	}
	display->cursorTheme = wl_cursor_theme_load(NULL, 24 * display->windowDimensions.scale, display->shm);
}

static void setCursor(struct display *display, char *name)
{
	struct wl_cursor *cursor = wl_cursor_theme_get_cursor(display->cursorTheme, name);
	struct wl_cursor_image *cursorImage = cursor->images[0];
	struct wl_buffer *cursorBuffer = wl_cursor_image_get_buffer(cursorImage);

	wl_surface_attach(display->cursorSurface, cursorBuffer, 0, 0);
	wl_surface_set_buffer_scale(display->cursorSurface, display->windowDimensions.scale);
	wl_surface_damage(display->cursorSurface, 0, 0, INT32_MAX, INT32_MAX);
	wl_surface_commit(display->cursorSurface);
	wl_pointer_set_cursor(display->pointer, display->pointerSerial, display->cursorSurface, cursorImage->hotspot_x / display->windowDimensions.scale, cursorImage->hotspot_y / display->windowDimensions.scale);
}

static void createWindow(struct display *display)
{
	if (display->compositor == NULL) {
		handleFatalError("Can't find Wayland compositor\n");
	}

	display->surface = wl_compositor_create_surface(display->compositor);
	if (display->surface == NULL) {
		handleFatalError("Can't create Wayland surface\n");
	}
	printf("Created Wayland surface\n");
	display->surfaceListener = (struct wl_surface_listener) {
		.enter = surfaceEnterHandler,
		.leave = surfaceLeaveHandler
	};
	wl_surface_add_listener(display->surface, &display->surfaceListener, display);

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

static void setUpRegions(struct display *display)
{
	if (display->compositor == NULL) {
		handleFatalError("Can't find Wayland compositor\n");
	}
	printf("Found Wayland compositor\n");

	struct wl_region *inputRegion = wl_compositor_create_region(display->compositor);
	if (inputRegion == NULL) {
		handleFatalError("Can't create Wayland region\n");
	}
	printf("Created Wayland region\n");
	wl_region_add(
		inputRegion,
		display->windowDimensions.activeArea.offset.x - RESIZE_BORDER,
		display->windowDimensions.activeArea.offset.y - RESIZE_BORDER,
		display->windowDimensions.activeArea.extent.width + RESIZE_BORDER * 2,
		display->windowDimensions.activeArea.extent.height + RESIZE_BORDER * 2
	);

	struct wl_region *opaqueRegion = wl_compositor_create_region(display->compositor);
	if (opaqueRegion == NULL) {
		handleFatalError("Can't create Wayland region\n");
	}
	printf("Created Wayland region\n");
	wl_region_add(
		opaqueRegion,
		display->windowDimensions.activeArea.offset.x + display->windowDimensions.cornerRadius,
		display->windowDimensions.activeArea.offset.y,
		display->windowDimensions.activeArea.extent.width - (2 * display->windowDimensions.cornerRadius),
		display->windowDimensions.activeArea.extent.height
	);
	wl_region_add(
		opaqueRegion,
		display->windowDimensions.activeArea.offset.x,
		display->windowDimensions.activeArea.offset.y + display->windowDimensions.cornerRadius,
		display->windowDimensions.activeArea.extent.width,
		display->windowDimensions.activeArea.extent.height - (2 * display->windowDimensions.cornerRadius)
	);

	wl_surface_set_input_region(display->surface, inputRegion);
	wl_surface_set_opaque_region(display->surface, opaqueRegion);

	wl_region_destroy(inputRegion);
	wl_region_destroy(opaqueRegion);
}

static void registryGlobalHandler(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version)
{
	struct display *display = data;

	printf("Got a Wayland registry event for %s ID %d\n", interface, id);
	if (strcmp(interface, "wl_compositor") == 0) {
		display->compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 3);
		if (display->compositor == NULL) {
			handleFatalError("Can't bind Wayland compositor\n");
		}
		printf("Bound Wayland compositor\n");
	} else if (strcmp(interface, "xdg_wm_base") == 0) {
		display->xdgWmBase = wl_registry_bind(registry, id, &xdg_wm_base_interface, 2);
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
	} else if (strcmp(interface, "wl_output") == 0) {
		struct wl_output *output = wl_registry_bind(registry, id, &wl_output_interface, 2);
		if (output == NULL) {
			handleFatalError("Can't bind Wayland output\n");
		}
		printf("Bound Wayland output\n");
		configureOutput(display, output, id);
	}
}

static void configureOutput(struct display *display, struct wl_output *output, uint32_t id)
{
	display->outputInfos = realloc(display->outputInfos, sizeof(*display->outputInfos) * ++display->outputInfoCount);
	OutputInfo *outputInfo = malloc(sizeof(*outputInfo));
	*outputInfo = (OutputInfo) {
		.output = output,
		.id = id
	};
	display->outputInfos[display->outputInfoCount - 1] = outputInfo;
	wl_output_add_listener(display->outputInfos[display->outputInfoCount - 1]->output, &outputListener, display->outputInfos[display->outputInfoCount - 1]);
}

static int findIndexOfOutputInfoWithGlobalId(struct display *display, uint32_t id)
{
	for (size_t i = 0; i < display->outputInfoCount; ++i) {
		if (display->outputInfos[i]->id == id) {
			return i;
		}
	}

	return -1;
}

static size_t findIndexOfOutputInfoWithOutput(struct display *display, struct wl_output *output)
{
	for (size_t i = 0; i < display->outputInfoCount; ++i) {
		if (display->outputInfos[i]->output == output) {
			return i;
		}
	}
}

static size_t findIndexOfActiveOutputInfoWithOutput(struct display *display, struct wl_output *output)
{
	for (size_t i = 0; i < display->activeOutputInfoCount; ++i) {
		if (display->activeOutputInfos[i]->output == output) {
			return i;
		}
	}
}

static void registryGlobalRemoveHandler(void *data, struct wl_registry *registry, uint32_t id)
{
	struct display *display = data;

	printf("Got a Wayland registry remove event for %d\n", id);
	int outputIndex = findIndexOfOutputInfoWithGlobalId(display, id);
	if (outputIndex < 0) {
		return;
	}
	free(display->outputInfos[outputIndex]);
	for (size_t i = outputIndex; i < display->outputInfoCount - 1; ++i) {
		display->outputInfos[i] = display->outputInfos[i + 1];
	}
	--display->outputInfoCount;
}

void scaleWindowDimensions(WindowDimensions *windowDimensions, uint scale)
{
	windowDimensions->surfaceArea = (VkExtent2D) {
		.width = windowDimensions->surfaceArea.width * scale,
		.height = windowDimensions->surfaceArea.height * scale
	};

	windowDimensions->activeArea = (VkRect2D) {
		.offset.x = windowDimensions->activeArea.offset.x * scale,
		.offset.y = windowDimensions->activeArea.offset.y * scale,
		.extent.width = windowDimensions->activeArea.extent.width * scale,
		.extent.height = windowDimensions->activeArea.extent.height * scale
	};

	windowDimensions->cornerRadius = windowDimensions->cornerRadius * scale;
}

void setSurfaceScale(struct display *display)
{
	printf("setSurfaceScale\n");
	unsigned int scale = 1;
	for (size_t i = 0; i < display->activeOutputInfoCount; ++i) {
		if (display->activeOutputInfos[i]->scale > scale) {
			scale = display->activeOutputInfos[i]->scale;
		}
	}

	if (display->windowDimensions.scale == scale) {
		return;
	}

	display->pointerRegion = UNKNOWN_REGION;

	display->windowDimensions.scale = scale;

	loadCursorTheme(display);

	enqueueResizeEvent(display);
}

static void enqueueResizeEvent(struct display *display)
{
	WindowDimensions windowDimensions = display->windowDimensions;
	scaleWindowDimensions(&windowDimensions, display->windowDimensions.scale);
	WaylandWindow *window = malloc(sizeof(*window));
	*window = (WaylandWindow) {
		.display = display->display,
		.surface = display->surface,
		.xdgSurface = display->xdgSurface,
		.fd = display->threadPipe[0]
	};
	ResizeInfo *resizeInfo = malloc(sizeof(*resizeInfo));
	*resizeInfo = (ResizeInfo) {
		.windowDimensions = windowDimensions,
		.platformWindow = window
	};
	enqueueInputEvent(&display->inputQueue, RESIZE, resizeInfo);
}

static void surfaceEnterHandler(void *data, struct wl_surface *surface, struct wl_output *output)
{
	printf("Got a Wayland surface enter event\n");
	struct display *display = data;
	size_t outputIndex = findIndexOfOutputInfoWithOutput(display, output);
	OutputInfo *outputInfo = display->outputInfos[outputIndex];
	display->activeOutputInfos = realloc(display->activeOutputInfos, sizeof(*display->activeOutputInfos) * ++display->activeOutputInfoCount);
	display->activeOutputInfos[display->activeOutputInfoCount - 1] = outputInfo;
	setSurfaceScale(display);
}

static void surfaceLeaveHandler(void *data, struct wl_surface *surface, struct wl_output *output)
{
	printf("Got a Wayland surface leave event\n");
	struct display *display = data;
	size_t outputIndex = findIndexOfActiveOutputInfoWithOutput(display, output);
	for (size_t i = outputIndex; i < display->activeOutputInfoCount - 1; ++i) {
		display->activeOutputInfos[i] = display->activeOutputInfos[i + 1];
	}
	--display->activeOutputInfoCount;
	setSurfaceScale(display);
}

static void xdgWmBasePingHandler(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial)
{
	struct display *display = data;
	xdg_wm_base_pong(display->xdgWmBase, serial);
}

static void xdgSurfaceConfigureHandler(void *data, struct xdg_surface *xdg_surface, uint32_t serial)
{
	printf("Got a xdg surface configure event\n");
	struct display *display = data;

	xdg_surface_ack_configure(xdg_surface, serial);

	char *error;
	if (!display->vulkanInitialized) {
		if (!initVulkanWayland(display->display, display->surface, display->xdgSurface, display->windowDimensions, &display->inputQueue, display->threadPipe[1], &error)) {
			handleFatalError(error);
		}
		display->vulkanInitialized = true;
	}

	setUpRegions(display);

	enqueueResizeEvent(display);
}

static void xdgToplevelConfigureHandler(void *data, struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height, struct wl_array *states)
{
	printf("Got a xdg toplevel configure event\n");
	struct display *display = data;

	bool leftTiled = false;
	bool rightTiled = false;
	bool topTiled = false;
	bool bottomTiled = false;

	enum xdg_toplevel_state *state;
	wl_array_for_each(state, states) {
		switch (*state) {
		case XDG_TOPLEVEL_STATE_MAXIMIZED:
			printf("maximized\n");
			leftTiled = rightTiled = topTiled = bottomTiled = true;
			break;
		case XDG_TOPLEVEL_STATE_RESIZING:
			printf("resizing\n");
			break;
		case XDG_TOPLEVEL_STATE_TILED_LEFT:
			printf("tiled left\n");
			leftTiled = true;
			break;
		case XDG_TOPLEVEL_STATE_TILED_RIGHT:
			printf("tiled right\n");
			rightTiled = true;
			break;
		case XDG_TOPLEVEL_STATE_TILED_TOP:
			printf("tiled top\n");
			topTiled = true;
			break;
		case XDG_TOPLEVEL_STATE_TILED_BOTTOM:
			printf("tiled bottom\n");
			bottomTiled = true;
			break;
		default:
			printf("other state\n");
			break;
		}
	}

	int marginLeft = leftTiled ? 0 : OFFSET_X;
	int marginRight = rightTiled ? 0 : (MARGIN * 2) - OFFSET_X;
	int marginTop = topTiled ? 0 : OFFSET_Y;
	int marginBottom = bottomTiled ? 0 : (MARGIN * 2) - OFFSET_Y;

	if (width != 0 || height != 0) {
		display->windowDimensions.surfaceArea.width = width + marginLeft + marginRight;
		display->windowDimensions.surfaceArea.height = height + marginTop + marginLeft;
		display->windowDimensions.activeArea.offset.x = marginLeft;
		display->windowDimensions.activeArea.offset.y = marginTop;
		display->windowDimensions.activeArea.extent.width = width;
		display->windowDimensions.activeArea.extent.height = height;
	}
}

static void outputGeometryHandler(void *data, struct wl_output *output, int32_t x, int32_t y, int32_t physical_width, int32_t physical_height, enum wl_output_subpixel subpixel, const char *make, const char *model, enum wl_output_transform transform)
{
	printf("Got an output geometry event\n");
	OutputInfo *outputInfo = data;
}

static void outputModeHandler(void *data, struct wl_output *output, enum wl_output_mode mode, int32_t width, int32_t height, int32_t refresh)
{
	printf("Got an output mode event\n");
	OutputInfo *outputInfo = data;
}


static void outputDoneHandler(void *data, struct wl_output *output)
{
	printf("Got an output done event\n");
	OutputInfo *outputInfo = data;
}

static void outputScaleHandler(void *data, struct wl_output *output, int32_t factor)
{
	printf("Got an output scale event\n");
	OutputInfo *outputInfo = data;
	outputInfo->scale = factor;
}

static void outputNameHandler(void *data, struct wl_output *output, const char *name)
{
	printf("Got an output name event\n");
	OutputInfo *outputInfo = data;
}

static void outputDescriptionHandler(void *data, struct wl_output *output, const char *description)
{
	printf("Got an output description event\n");
	OutputInfo *outputInfo = data;
}


static void pointerEnterHandler(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y)
{
	struct display *display = data;
	int cursorX = wl_fixed_to_double(x);
	int cursorY = wl_fixed_to_double(y);

	display->pointerSerial = serial;
	hitTestAndSetCursor(display, cursorX, cursorY, false);
}

static void pointerLeaveHandler(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface)
{
	struct display *display = data;

	enqueueInputEvent(&display->inputQueue, POINTER_LEAVE, NULL);
}

static void pointerMotionHandler(void *data, struct wl_pointer *pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
	struct display *display = data;
	int cursorX = wl_fixed_to_double(x);
	int cursorY = wl_fixed_to_double(y);

	hitTestAndSetCursor(display, cursorX, cursorY, true);

	enqueueInputEventWithPosition(&display->inputQueue, POINTER_MOVE, cursorX, cursorY);
}

static void hitTestAndSetCursor(struct display *display, int x, int y, bool debounce)
{
	display->pointerPosition = (VkOffset2D) {x: x - RESIZE_BORDER, y: y - RESIZE_BORDER};
	WindowRegion region = hitTest(display, x, y);

	if (debounce && display->pointerRegion == region) {
		return;
	}

	display->pointerRegion = region;
	setCursorFromWindowRegion(display, region);
}

static void setCursorFromWindowRegion(struct display *display, WindowRegion region)
{
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

static void pointerButtonHandler(void *data, struct wl_pointer *pointer, uint32_t serial, uint32_t time, uint32_t button, enum wl_pointer_button_state state)
{
	struct display *display = data;

	switch (display->pointerRegion) {
	case CLIENT:
		if (button == BTN_LEFT) {
			if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
				enqueueInputEvent(&display->inputQueue, BUTTON_DOWN, NULL);
			} else {
				enqueueInputEvent(&display->inputQueue, BUTTON_UP, NULL);
			}
		}
		break;
	case CHROME:
		if (button == BTN_LEFT && state == WL_POINTER_BUTTON_STATE_PRESSED) {
			xdg_toplevel_move(display->xdgToplevel, display->seat, serial);
		} else if (button == BTN_RIGHT && state == WL_POINTER_BUTTON_STATE_PRESSED) {
			xdg_toplevel_show_window_menu(display->xdgToplevel, display->seat, serial, display->pointerPosition.x, display->pointerPosition.y);
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

static WindowRegion hitTest(struct display *display, int x, int y)
{
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

	if (x < offset.x) {
		result |= _LEFT;
	}

	if (x >= extent.width + offset.x) {
		result |= _RIGHT;
	}

	if (y < offset.y) {
		result |= _TOP;
	}

	if (y >= extent.height + offset.y) {
		result |= _BOTTOM;
	}

	if (!(result & (_LEFT | _RIGHT | _TOP | _BOTTOM)) && y < (offset.y + CHROME_HEIGHT)) {
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

static void pointerAxisHandler(void *data, struct wl_pointer *pointer, uint32_t time, uint32_t axis, wl_fixed_t value)
{

}

static void handleFatalError(char *message)
{
	fprintf(stderr, "%s\n", message);
	exit(1);
}
