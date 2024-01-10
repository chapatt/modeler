#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <vulkan/vulkan.h>

#include "imgui/cimgui.h"
#include "imgui/cimgui_impl_vulkan.h"
#include "imgui/imgui_impl_modeler.h"

#include "modeler_win32.h"
#include "modeler.h"
#include "utils.h"

static void imVkCheck(VkResult result);

pthread_t initVulkanWin32(HINSTANCE hinstance, HWND hwnd, Queue *inputQueue, char **error)
{
	pthread_t thread;
	struct threadArguments *threadArgs = malloc(sizeof(*threadArgs));
	Win32Window *window = malloc(sizeof(Win32Window));
	window->hinstance = hinstance;
	window->hwnd = hwnd;
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
	threadArgs->initialExtent = getWindowExtent(window);
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
	HWND hwnd = ((Win32Window *) platformWindow)->hwnd;
	PostMessageW(hwnd, THREAD_FAILURE_NOTIFICATION_MESSAGE, 0, 0);
	pthread_exit(NULL);
}