#ifndef MODELER_METAL_H
#define MODELER_METAL_H

#include <stdbool.h>

#include "modeler.h"
#include "queue.h"
#include "input_event.h"

#define THREAD_FAILURE_NOTIFICATION_NAME "THREAD_FAILURE"

typedef struct metal_window_t {
	void *surfaceLayer;
} MetalWindow;

pthread_t initVulkanMetal(void *surfaceLayer, int width, int height, const char *resourcePath, Queue *inputQueue, char **error);

#endif /* MODELER_METAL_H */
