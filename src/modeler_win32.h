#ifndef MODELER_WIN32_H
#define MODELER_WIN32_H

#include <stdbool.h>

#include "queue.h"
#include "input_event.h"

bool initVulkanWin32(HINSTANCE hinstance, HWND hwnd, Queue *inputQueue, char **error);

#endif /* MODELER_WIN32_H */
