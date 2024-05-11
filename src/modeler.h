#ifndef MODELER_H
#define MODELER_H

#include <vulkan/vulkan.h>

#include "vk_mem_alloc.h"

#include "physical_device.h"
#include "device.h"
#include "swapchain.h"
#include "queue.h"

typedef struct window_dimensions_t {
	VkExtent2D surfaceArea;
	VkRect2D activeArea;
	int cornerRadius;
} WindowDimensions;

typedef struct resize_info_t {
	WindowDimensions windowDimensions;
	int scale;
	void *platformWindow;
} ResizeInfo;

struct threadArguments {
	void *platformWindow;
	Queue *inputQueue;
	char *resourcePath;
	const char **instanceExtensions;
	size_t instanceExtensionCount;
	WindowDimensions windowDimensions;
	char **error;
};

typedef struct swapchain_create_info_t {
	VkDevice device;
	VmaAllocator allocator;
	VkPhysicalDevice physicalDevice;
	VkSurfaceKHR surface;
	PhysicalDeviceSurfaceCharacteristics *surfaceCharacteristics;
	QueueInfo queueInfo;
	VkRenderPass renderPass;
	SwapchainInfo *swapchainInfo;
	VkImage *offscreenImage;
	uint32_t offscreenImageCount;
	VkImageView *offscreenImageView;
	VmaAllocation *offscreenImageAllocation;
	VkImageView **imageViews;
	VkFramebuffer **framebuffers;
	VkDescriptorPool *descriptorPool;
	VkDescriptorSet **imageDescriptorSets;
	VkDescriptorSetLayout **imageDescriptorSetLayouts;
	VkDescriptorSet **bufferDescriptorSets;
	VkDescriptorSetLayout **bufferDescriptorSetLayouts;
} SwapchainCreateInfo;

void *threadProc(void *arg);
bool recreateSwapchain(SwapchainCreateInfo swapchainCreateInfo, VkExtent2D windowExtent, char **error);
void sendThreadFailureSignal(void *platformWindow);
void terminateVulkan(Queue *inputQueue, pthread_t thread);
void ackResize(ResizeInfo *resizeInfo);

#endif /* MODELER_H */
