#ifndef MODELER_H
#define MODELER_H

#include <vulkan/vulkan.h>

#include "queue.h"

struct threadArguments {
	void *platformWindow;
	Queue *inputQueue;
	char *resourcePath;
	const char **instanceExtensions;
	size_t instanceExtensionCount;
	VkExtent2D initialExtent;
	char **error;
};

void *threadProc(void *arg);
void sendThreadFailureSignal(void *platformWindow);
void terminateVulkan(Queue *inputQueue, pthread_t thread);

#endif /* MODELER_H */
