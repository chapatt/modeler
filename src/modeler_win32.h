#ifndef MODELER_WIN32_H
#define MODELER_WIN32_H

#include <stdbool.h>
#include <pthread.h>
#include <windows.h>

#include "queue.h"

#define THREAD_FAILURE_NOTIFICATION_MESSAGE (WM_USER + 0)
#define FULLSCREEN_NOTIFICATION_MESSAGE (WM_USER + 1)
#define EXIT_FULLSCREEN_NOTIFICATION_MESSAGE (WM_USER + 2)
#define CLOSE_NOTIFICATION_MESSAGE (WM_USER + 3)

typedef struct win32_window_t {
	HINSTANCE hInstance;
	HWND hWnd;
} Win32Window;

pthread_t initVulkanWin32(HINSTANCE hInstance, HWND hWnd, Queue *inputQueue, char **error);

#endif /* MODELER_WIN32_H */
