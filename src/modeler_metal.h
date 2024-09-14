#ifndef MODELER_METAL_H
#define MODELER_METAL_H

#include <stdbool.h>

#include "modeler.h"
#include "queue.h"
#include "input_event.h"

#define THREAD_FAILURE_NOTIFICATION_NAME "THREAD_FAILURE"
#define FULLSCREEN_NOTIFICATION_NAME "FULLSCREEN"
#define EXIT_FULLSCREEN_NOTIFICATION_NAME "EXIT_FULLSCREEN"

typedef struct metal_window_t {
	void *surfaceLayer;
} MetalWindow;

pthread_t initVulkanMetal(void *surfaceLayer, int width, int height, float scale, const char *resourcePath, Queue *inputQueue, char **error);
void enqueueResizeEvent(Queue *queue, WindowDimensions windowDimensions, void *surfaceLayer);

#endif /* MODELER_METAL_H */
