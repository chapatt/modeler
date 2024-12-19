#ifndef MODELER_H
#define MODELER_H

#include <vulkan/vulkan.h>

#include "vk_mem_alloc.h"

#include "window.h"
#include "physical_device.h"
#include "device.h"
#include "swapchain.h"
#include "queue.h"
#include "chess_board.h"

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
	VkCommandPool commandPool;
	VkRenderPass *renderPass;
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
	ChessBoard *chessBoard;
} SwapchainCreateInfo;

void *threadProc(void *arg);
bool recreateSwapchain(SwapchainCreateInfo swapchainCreateInfo, WindowDimensions windowDimensions, char **error);
void sendFullscreenSignal(void *platformWindow);
void sendExitFullscreenSignal(void *platformWindow);
void sendThreadFailureSignal(void *platformWindow);
void terminateVulkan(Queue *inputQueue, pthread_t thread);
void ackResize(ResizeInfo *resizeInfo);

#endif /* MODELER_H */
