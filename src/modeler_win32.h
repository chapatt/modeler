#ifndef MODELER_WIN32_H
#define MODELER_WIN32_H

#include <stdbool.h>
#include <windows.h>

#include "queue.h"
#include "input_event.h"

#define THREAD_FAILURE_NOTIFICATION_MESSAGE (WM_USER + 0)

pthread_t initVulkanWin32(HINSTANCE hinstance, HWND hwnd, Queue *inputQueue, char **error);
void terminateVulkanWin32(Queue *inputQueue, pthread_t thread);

#endif /* MODELER_WIN32_H */
