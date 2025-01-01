#ifndef MODELER_ANDROID_H
#define MODELER_ANDROID_H

#include <pthread.h>
#include <vulkan/vulkan.h>

#include "queue.h"

typedef struct android_window_t {
	ANativeWindow nativeWindow;
} AndroidWindow;

pthread_t initVulkanAndroid(ANativeWindow nativeWindow, Queue *inputQueue, char **error);

#endif /* MODELER_ANDROID_H */
