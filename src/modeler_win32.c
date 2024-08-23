#include <stdlib.h>
#include <vulkan/vulkan.h>

#include "imgui/cimgui.h"
#include "imgui/cimgui_impl_vulkan.h"
#include "imgui/imgui_impl_modeler.h"

#include "modeler.h"
#include "utils.h"

#include "modeler_win32.h"

pthread_t initVulkanWin32(HINSTANCE hInstance, HWND hWnd, Queue *inputQueue, char **error)
{
	pthread_t thread;
	struct threadArguments *threadArgs = malloc(sizeof(*threadArgs));
	Win32Window *window = malloc(sizeof(Win32Window));
	window->hInstance = hInstance;
	window->hWnd = hWnd;
	threadArgs->platformWindow = window;
	threadArgs->inputQueue = inputQueue;
	asprintf(&threadArgs->resourcePath, ".");
	char *instanceExtensions[] = {
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
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

	VkExtent2D extent = getWindowExtent(window);
	float scale = getWindowScale(window);
	if (extent.width == 0 || extent.height == 0) {
		asprintf(error, "Failed to get window extent");
		return 0;
	}
	threadArgs->windowDimensions = (WindowDimensions) {
		.surfaceArea = {
			.width = extent.width,
			.height = extent.height
		},
		.activeArea = {
			.extent = {
				.width = extent.width,
				.height = extent.height
			},
			.offset = {0, 0}
		},
		.cornerRadius = 0,
		.scale = scale
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

void sendThreadFailureSignal(void *platformWindow)
{
	HWND hWnd = ((Win32Window *) platformWindow)->hWnd;
	PostMessageW(hWnd, THREAD_FAILURE_NOTIFICATION_MESSAGE, 0, 0);
	pthread_exit(NULL);
}

void ackResize(ResizeInfo *resizeInfo)
{
	/* Unnecessary on win32 */
}
