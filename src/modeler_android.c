#include <stdlib.h>
#include <unistd.h>

#include "modeler_android.h"
#include "modeler.h"
#include "utils.h"

pthread_t initVulkanAndroid(struct ANativeWindow *nativeWindow, Queue *inputQueue, int fd, char **error)
{
	pthread_t thread;
	struct threadArguments *threadArgs = malloc(sizeof(*threadArgs));
	AndroidWindow *window = malloc(sizeof(*window));
	window->nativeWindow = nativeWindow;
	window->fd = fd;
	threadArgs->platformWindow = window;
	threadArgs->inputQueue = inputQueue;
	asprintf(&threadArgs->resourcePath, ".");
	char *instanceExtensions[] = {
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
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

	threadArgs->windowDimensions = (WindowDimensions) {
		.surfaceArea = {
			.width = 0,
			.height = 0
		},
		.activeArea = {
			.extent = {
				.width = 0,
				.height = 0
			},
			.offset = {0, 0}
		},
		.cornerRadius = 0,
		.scale = 1,
		.fullscreen = false
	};

	if (pthread_create(&thread, NULL, threadProc, (void *) threadArgs) != 0) {
		free(threadArgs->resourcePath);
		free(threadArgs);
		asprintf(error, "Failed to start Vulkan thread");
		return 0;
	}

	threadArgs->error = error;

	return thread;
}

static void sendSignal(void *platformWindow, char signal)
{
	int fd = ((AndroidWindow *) platformWindow)->fd;
	write(fd, &signal, 1);
	close(fd);
	pthread_exit(NULL);
}

void sendThreadFailureSignal(void *platformWindow)
{
	sendSignal(platformWindow, 'f');
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
}
