#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <vulkan/vulkan.h>

#include "imgui/cimgui.h"
#include "imgui/cimgui_impl_vulkan.h"
#include "imgui/imgui_impl_modeler.h"

#include "modeler.h"
#include "utils.h"

#include "modeler_wayland.h"

pthread_t initVulkanWayland(struct wl_display *waylandDisplay, struct wl_surface *waylandSurface, struct xdg_surface *xdgSurface, WindowDimensions windowDimensions, Queue *inputQueue, int fd, char **error)
{
	pthread_t thread;
	struct threadArguments *threadArgs = malloc(sizeof(*threadArgs));
	WaylandWindow *window = malloc(sizeof(WaylandWindow));
	*window = (WaylandWindow) {
		.display = waylandDisplay,
		.surface = waylandSurface,
		.xdgSurface = xdgSurface,
		.fd = fd
	};
	threadArgs->platformWindow = window;
	threadArgs->inputQueue = inputQueue;
	asprintf(&threadArgs->resourcePath, ".");
	char *instanceExtensions[] = {
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME
	};
	#ifdef DEBUG
	threadArgs->instanceExtensionCount = 4;
	#else
	threadArgs->instanceExtensionCount = 3;
	#endif /* DEBUG */
	threadArgs->instanceExtensions = malloc(sizeof(*threadArgs->instanceExtensions) * threadArgs->instanceExtensionCount);
	for (size_t i = 0; i < threadArgs->instanceExtensionCount; ++i) {
		threadArgs->instanceExtensions[i] = instanceExtensions[i];
	}
	threadArgs->windowDimensions = windowDimensions;
	threadArgs->error = error;

	if (pthread_create(&thread, NULL, threadProc, (void *) threadArgs) != 0) {
		free(threadArgs->resourcePath);
		free(threadArgs);
		asprintf(error, "Failed to start Vulkan thread");
		return 0;
	}

	return thread;
}

static void sendSignal(void *platformWindow, char signal)
{
	int fd = ((WaylandWindow *) platformWindow)->fd;
	write(fd, &signal, 1);
}

void sendThreadFailureSignal(void *platformWindow)
{
	int fd = ((WaylandWindow *) platformWindow)->fd;
	sendSignal(platformWindow, 'f');
	close(fd);
	pthread_exit(NULL);
}

void sendFullscreenSignal(void *platformWindow)
{
	sendSignal(platformWindow, 'g');
}

void sendExitFullscreenSignal(void *platformWindow)
{
	sendSignal(platformWindow, 'h');
}

void ackResize(ResizeInfo *resizeInfo)
{
	WaylandWindow *window = resizeInfo->platformWindow;
	wl_surface_set_buffer_scale(window->surface, resizeInfo->windowDimensions.scale);
	xdg_surface_set_window_geometry(
		window->xdgSurface,
		resizeInfo->windowDimensions.activeArea.offset.x,
		resizeInfo->windowDimensions.activeArea.offset.y,
		resizeInfo->windowDimensions.activeArea.extent.width,
		resizeInfo->windowDimensions.activeArea.extent.height
	);
}
