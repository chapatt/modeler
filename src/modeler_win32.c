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
	threadArgs->error = error;

	if (pthread_create(&thread, NULL, threadProc, (void *) threadArgs) != 0) {
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