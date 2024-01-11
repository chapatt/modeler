#ifndef MODELER_WIN32_H
#define MODELER_WIN32_H

#include <stdbool.h>
#include <pthread.h>
#include <windows.h>

#include "queue.h"

#define THREAD_FAILURE_NOTIFICATION_MESSAGE (WM_USER + 0)

typedef struct win32_window_t {
	HINSTANCE hinstance;
	HWND hwnd;
} Win32Window;

pthread_t initVulkanWin32(HINSTANCE hinstance, HWND hwnd, Queue *inputQueue, char **error);

#endif /* MODELER_WIN32_H */
