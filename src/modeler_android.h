#ifndef MODELER_ANDROID_H
#define MODELER_ANDROID_H

#include <pthread.h>

#include "queue.h"
#include "input_event.h"
#include "vulkan_utils.h"

typedef struct android_window_t {
	struct ANativeWindow *nativeWindow;
	struct ANativeActivity *nativeActivity;
	int fd;
} AndroidWindow;

pthread_t initVulkanAndroid(struct ANativeWindow *nativeWindow, struct ANativeActivity *nativeActivity, Queue *inputQueue, int fd, char **error);

#endif /* MODELER_ANDROID_H */
