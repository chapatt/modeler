#ifndef MODELER_ANDROID_H
#define MODELER_ANDROID_H

#include <pthread.h>

#include "queue.h"
#include "vulkan_utils.h"

typedef struct android_window_t {
	struct ANativeWindow *nativeWindow;
} AndroidWindow;

pthread_t initVulkanAndroid(struct ANativeWindow *nativeWindow, Queue *inputQueue, char **error);

#endif /* MODELER_ANDROID_H */
