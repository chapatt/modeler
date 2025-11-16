#ifndef MODELER_H
#define MODELER_H

#include <vulkan/vulkan.h>

#include "vk_mem_alloc.h"

#include "window.h"
#include "physical_device.h"
#include "device.h"
#include "swapchain.h"
#include "queue.h"

typedef struct swapchain_create_info_t *SwapchainCreateInfo;

struct threadArguments {
	void *platformWindow;
	Queue *inputQueue;
	char *resourcePath;
	const char **instanceExtensions;
	size_t instanceExtensionCount;
	WindowDimensions windowDimensions;
	char **error;
};

void *threadProc(void *arg);
bool recreateSwapchain(SwapchainCreateInfo swapchainCreateInfo, bool windowResized, char **error);
void sendCloseSignal(void *platformWindow);
void sendMaximizeSignal(void *platformWindow);
void sendMinimizeSignal(void *platformWindow);
void sendFullscreenSignal(void *platformWindow);
void sendExitFullscreenSignal(void *platformWindow);
void sendThreadFailureSignal(void *platformWindow);
void terminateVulkan(Queue *inputQueue, pthread_t thread);
void ackResize(ResizeInfo *resizeInfo);

#endif /* MODELER_H */
