#ifndef MODELER_H
#define MODELER_H

#include <vulkan/vulkan.h>

#include "physical_device.h"
#include "device.h"
#include "swapchain.h"
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

typedef struct swapchain_create_info_t {
	VkDevice device;
	VkPhysicalDevice physicalDevice;
	VkSurfaceKHR surface;
	PhysicalDeviceSurfaceCharacteristics *surfaceCharacteristics;
	QueueInfo queueInfo;
	VkRenderPass renderPass;
	SwapchainInfo *swapchainInfo;
	VkExtent2D extent;
	VkImageView **imageViews;
	VkFramebuffer **framebuffers;
} SwapchainCreateInfo;

void *threadProc(void *arg);
bool recreateSwapchain(SwapchainCreateInfo swapchainCreateInfo, VkExtent2D windowExtent, char **error);
void sendThreadFailureSignal(void *platformWindow);
void terminateVulkan(Queue *inputQueue, pthread_t thread);

#endif /* MODELER_H */
