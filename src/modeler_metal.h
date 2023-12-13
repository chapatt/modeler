#ifndef MODELER_METAL_H
#define MODELER_METAL_H

#include <stdbool.h>

#include "queue.h"
#include "input_event.h"

#define THREAD_FAILURE_NOTIFICATION_NAME "THREAD_FAILURE"

bool initVulkanMetal(void *surfaceLayer, int width, int height, const char *resourcePath, Queue *inputQueue, char **error);

#endif /* MODELER_METAL_H */
