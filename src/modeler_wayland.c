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

static void imVkCheck(VkResult result);

pthread_t initVulkanWayland(struct wl_display *waylandDisplay, struct wl_surface *waylandSurface, Queue *inputQueue, int fd, char **error)
{
	pthread_t thread;
	struct threadArguments *threadArgs = malloc(sizeof(*threadArgs));
	WaylandWindow *window = malloc(sizeof(WaylandWindow));
	window->display = waylandDisplay;
	window->surface = waylandSurface;
	window->fd = fd;
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
	threadArgs->initialExtent = (VkExtent2D) {
		.width = 600,
		.height = 400
        };
	if (threadArgs->initialExtent.width == 0 || threadArgs->initialExtent.height == 0) {
		asprintf(error, "Failed to get window extent");
		return 0;
	}
	threadArgs->error = error;

	if (pthread_create(&thread, NULL, threadProc, (void *) threadArgs) != 0) {
		free(threadArgs->resourcePath);
		free(threadArgs);
		asprintf(error, "Failed to start Vulkan thread");
		return 0;
	}

	return thread;
}

void sendThreadFailureSignal(void *platformWindow)
{
	int fd = ((WaylandWindow *) platformWindow)->fd;
	char c = 'f';
	write(fd, &c, 1);
	close(fd);
	pthread_exit(NULL);
}
