#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <vulkan/vulkan.h>

#include "imgui/cimgui.h"
#include "imgui/cimgui_impl_vulkan.h"
#include "imgui/imgui_impl_modeler.h"

#include "vk_mem_alloc.h"

#include "modeler.h"
#include "instance.h"
#include "surface.h"
#include "image.h"
#include "image_view.h"
#include "render_pass.h"
#include "descriptor.h"
#include "pipeline.h"
#include "framebuffer.h"
#include "command_pool.h"
#include "command_buffer.h"
#include "synchronization.h"
#include "allocator.h"
#include "utils.h"
#include "vulkan_utils.h"

#ifdef EMBED_SHADERS
#include "../shader_window_border.vert.h"
#include "../shader_window_border.frag.h"
#include "../shader_triangle.vert.h"
#include "../shader_triangle.frag.h"
#endif /* EMBED_SHADERS */

#include "renderloop.h"

void initializeImgui(void *platformWindow, SwapchainInfo *swapchainInfo, PhysicalDeviceSurfaceCharacteristics surfaceCharacteristics, QueueInfo queueInfo, VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, VkRenderPass renderPass, char **error);
static void cleanupVulkan(VkInstance instance, VkDebugReportCallbackEXT debugCallback, VkSurfaceKHR surface, PhysicalDeviceCharacteristics *characteristics, PhysicalDeviceSurfaceCharacteristics *surfaceCharacteristics, VkDevice device, VmaAllocator allocator, VkSwapchainKHR swapchain, VkImage *offscreenImages, VmaAllocation *offscreenImageAllocations, size_t offscreenImageCount, VkImageView *offscreenImageViews, VkImageView *imageViews, uint32_t imageViewCount, VkRenderPass renderPass, VkPipelineLayout pipelineLayout, VkPipeline pipeline, VkPipelineLayout secondPipelineLayout, VkPipeline secondPipeline, VkFramebuffer *framebuffers, uint32_t framebufferCount, VkCommandPool commandPool, VkCommandBuffer *commandBuffers, uint32_t commandBufferCount, SynchronizationInfo synchronizationInfo);
static void imVkCheck(VkResult result);

void terminateVulkan(Queue *inputQueue, pthread_t thread)
{
	enqueueInputEvent(inputQueue, TERMINATE, NULL);
	pthread_join(thread, NULL);
}

void *threadProc(void *arg)
{
	struct threadArguments *threadArgs = (struct threadArguments *) arg;
	void *platformWindow = threadArgs->platformWindow;
	Queue *inputQueue = threadArgs->inputQueue;
	const char *resourcePath = threadArgs->resourcePath;
	const char **instanceExtensions = threadArgs->instanceExtensions;
	uint32_t instanceExtensionCount = threadArgs->instanceExtensionCount;
	VkExtent2D initialExtent = threadArgs->initialExtent;
	char **error = threadArgs->error;

	VkInstance instance;
	VkDebugReportCallbackEXT debugCallback;
	if (!createInstance(instanceExtensions, instanceExtensionCount, &instance, &debugCallback, error)) {
		sendThreadFailureSignal(platformWindow);
	}

	VkSurfaceKHR surface;
	if (!createSurface(instance, platformWindow, &surface, error)) {
		sendThreadFailureSignal(platformWindow);
	}

	VkPhysicalDevice physicalDevice;
	PhysicalDeviceCharacteristics characteristics;
	PhysicalDeviceSurfaceCharacteristics surfaceCharacteristics;
	if (!choosePhysicalDevice(instance, surface, &physicalDevice, &characteristics, &surfaceCharacteristics, error)) {
		sendThreadFailureSignal(platformWindow);
	}

	VkDevice device;
	QueueInfo queueInfo = {};
	if (!createDevice(physicalDevice, surface, characteristics, &device, &queueInfo, error)) {
		sendThreadFailureSignal(platformWindow);
	}

	VmaAllocator allocator;
	if (!createAllocator(instance, physicalDevice, device, &allocator, error)) {
		sendThreadFailureSignal(platformWindow);
	}

	SwapchainInfo swapchainInfo = {};
	if (!createSwapchain(device, surface, surfaceCharacteristics, queueInfo.graphicsQueueFamilyIndex, queueInfo.presentationQueueFamilyIndex, initialExtent, VK_NULL_HANDLE, &swapchainInfo, error)) {
		sendThreadFailureSignal(platformWindow);
	}

	VkImageView *imageViews = malloc(sizeof(imageViews) * swapchainInfo.imageCount);
	if (!createImageViews(device, swapchainInfo.images, swapchainInfo.imageCount, swapchainInfo.surfaceFormat.format, imageViews, error)) {
		sendThreadFailureSignal(platformWindow);
	}

#define DRAW_WINDOW_DECORATION = true
#ifdef DRAW_WINDOW_DECORATION
	VkImage offscreenImage;
	VmaAllocation offscreenImageAllocation;
	if (!createImage(device, allocator, swapchainInfo.extent, swapchainInfo.surfaceFormat.format, &offscreenImage, &offscreenImageAllocation, error)) {
		sendThreadFailureSignal(platformWindow);
	}

	VkImageView offscreenImageView;
	if (!createImageViews(device, &offscreenImage, 1, swapchainInfo.surfaceFormat.format, &offscreenImageView, error)) {
		sendThreadFailureSignal(platformWindow);
	}
#endif /* DRAW_WINDOW_DECORATION */

	VkRenderPass renderPass;
	if (!createRenderPass(device, swapchainInfo, &renderPass, error)) {
		sendThreadFailureSignal(platformWindow);
	}

	VkImageLayout imageLayouts[] = {VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
	VkSampler samplers[] = {VK_NULL_HANDLE};
	CreateDescriptorSetInfo createDescriptorSetInfo = {
		&offscreenImageView,
		imageLayouts,
		samplers,
		1,
		NULL,
		NULL,
		NULL,
		0
	};
	VkDescriptorPool descriptorPool;
	VkDescriptorSet *imageDescriptorSets;
	VkDescriptorSetLayout *imageDescriptorSetLayouts;
	VkDescriptorSet *bufferDescriptorSets;
	VkDescriptorSetLayout *bufferDescriptorSetLayouts;
	createDescriptorSets(device, createDescriptorSetInfo, &descriptorPool, &imageDescriptorSets, &imageDescriptorSetLayouts, &bufferDescriptorSets, &bufferDescriptorSetLayouts, error);

#ifndef EMBED_SHADERS
	char *vertexShaderPath;
	char *fragmentShaderPath;
	asprintf(&vertexShaderPath, "%s/%s", resourcePath, "window_border.vert.spv");
	asprintf(&fragmentShaderPath, "%s/%s", resourcePath, "window_border.frag.spv");
	char *vertexShaderBytes;
	char *fragmentShaderBytes;
	uint32_t vertexShaderSize = 0;
	uint32_t fragmentShaderSize = 0;

	if ((vertexShaderSize = readFileToString(vertexShaderPath, &vertexShaderBytes)) == -1) {
		asprintf(error, "Failed to open window border vertex shader for reading.\n");
		sendThreadFailureSignal(platformWindow);
	}
	if ((fragmentShaderSize = readFileToString(fragmentShaderPath, &fragmentShaderBytes)) == -1) {
		asprintf(error, "Failed to open window border fragment shader for reading.\n");
		sendThreadFailureSignal(platformWindow);
	}
#endif /* EMBED_SHADERS */

#ifndef EMBED_SHADERS
	char *vertexShaderPathTri;
	char *fragmentShaderPathTri;
	asprintf(&vertexShaderPathTri, "%s/%s", resourcePath, "triangle.vert.spv");
	asprintf(&fragmentShaderPathTri, "%s/%s", resourcePath, "triangle.frag.spv");
	char *vertexShaderBytesTri;
	char *fragmentShaderBytesTri;
	uint32_t vertexShaderSizeTri = 0;
	uint32_t fragmentShaderSizeTri = 0;

	if ((vertexShaderSizeTri = readFileToString(vertexShaderPathTri, &vertexShaderBytesTri)) == -1) {
		asprintf(error, "Failed to open triangle vertex shader for reading.\n");
		sendThreadFailureSignal(platformWindow);
	}
	if ((fragmentShaderSizeTri = readFileToString(fragmentShaderPathTri, &fragmentShaderBytesTri)) == -1) {
		asprintf(error, "Failed to open triangle fragment shader for reading.\n");
		sendThreadFailureSignal(platformWindow);
	}
#endif /* EMBED_SHADERS */

#ifdef DRAW_WINDOW_DECORATION
	VkPipelineLayout pipelineLayoutTriangle;
	VkPipeline pipelineTriangle;
	bool pipelineCreateSuccessTriangle = createPipeline(device, renderPass, 0, vertexShaderBytesTri, vertexShaderSizeTri, fragmentShaderBytesTri, fragmentShaderSizeTri, swapchainInfo.extent, NULL, 0, &pipelineLayoutTriangle, &pipelineTriangle, error);
#ifndef EMBED_SHADERS
	free(fragmentShaderBytesTri);
	free(vertexShaderBytesTri);
#endif /* EMBED_SHADERS */
	if (!pipelineCreateSuccessTriangle) {
		sendThreadFailureSignal(platformWindow);
	}
#endif /* DRAW_WINDOW_DECORATION */

	VkPipelineLayout pipelineLayoutWindowDecoration;
	VkPipeline pipelineWindowDecoration;
	bool pipelineCreateSuccessWindowDecoration = createPipeline(device, renderPass, 1, vertexShaderBytes, vertexShaderSize, fragmentShaderBytes, fragmentShaderSize, swapchainInfo.extent, imageDescriptorSetLayouts, 1, &pipelineLayoutWindowDecoration, &pipelineWindowDecoration, error);
#ifndef EMBED_SHADERS
	free(fragmentShaderBytes);
	free(vertexShaderBytes);
#endif /* EMBED_SHADERS */
	if (!pipelineCreateSuccessWindowDecoration) {
		sendThreadFailureSignal(platformWindow);
	}

	VkFramebuffer *framebuffers;
	if (!createFramebuffers(device, swapchainInfo, &offscreenImageView, imageViews, renderPass, &framebuffers, error)) {
		sendThreadFailureSignal(platformWindow);
	}

	VkCommandPool commandPool;
	if (!createCommandPool(device, queueInfo, &commandPool, error)) {
		sendThreadFailureSignal(platformWindow);
	}

	VkCommandBuffer *commandBuffers;
	if (!createCommandBuffers(device, swapchainInfo, commandPool, &commandBuffers, error)) {
		sendThreadFailureSignal(platformWindow);
	}

	SynchronizationInfo synchronizationInfo;
	if (!createSynchronization(device, swapchainInfo, &synchronizationInfo, error)) {
		sendThreadFailureSignal(platformWindow);
	}

	SwapchainCreateInfo swapchainCreateInfo = {
		.device = device,
		.physicalDevice = physicalDevice,
		.surface = surface,
		.surfaceCharacteristics = &surfaceCharacteristics,
		.queueInfo = queueInfo,
		.renderPass = renderPass,
		.swapchainInfo = &swapchainInfo,
		.extent = initialExtent,
		.offscreenImageView = &offscreenImageView,
		.imageViews = &imageViews,
		.framebuffers = &framebuffers
	};

	VkDescriptorPool imDescriptorPool;
	ImGui_ImplVulkan_InitInfo imVulkanInitInfo;
	// initializeImgui(platformWindow, &swapchainInfo, surfaceCharacteristics, queueInfo, instance, physicalDevice, device, renderPass, error);

	if (!draw(device, imageDescriptorSets[0], renderPass, pipelineTriangle, pipelineLayoutTriangle, pipelineWindowDecoration, pipelineLayoutWindowDecoration, &framebuffers, &commandBuffers, synchronizationInfo, &swapchainInfo, imageViews, queueInfo.graphicsQueue, queueInfo.presentationQueue, queueInfo.graphicsQueueFamilyIndex, ".", inputQueue, imVulkanInitInfo, swapchainCreateInfo, error)) {
		sendThreadFailureSignal(platformWindow);
	}

#ifdef DRAW_WINDOW_DECORATION
	size_t offscreenImageCount = 1;
	VkImage offscreenImages[] = {offscreenImage};
	VmaAllocation offscreenImageAllocations[] = {offscreenImageAllocation};
	VkImageView offscreenImageViews[] = {offscreenImageView};
#else
	VkImage *offscreenImages = NULL;
	VmaAllocation *offscreenImageAllocations = NULL;
	size_t offscreenImageCount = 0;
#endif /* DRAW_WINDOW_DECORATION */

	cleanupVulkan(instance, debugCallback, surface, &characteristics, &surfaceCharacteristics, device, allocator, swapchainInfo.swapchain, offscreenImages, offscreenImageAllocations, offscreenImageCount, offscreenImageViews, imageViews, swapchainInfo.imageCount, renderPass, pipelineLayoutWindowDecoration, pipelineWindowDecoration, pipelineLayoutTriangle, pipelineTriangle, framebuffers, swapchainInfo.imageCount, commandPool, commandBuffers, swapchainInfo.imageCount, synchronizationInfo);

	return NULL;
}

static void imVkCheck(VkResult result)
{
	if (result != VK_SUCCESS) {
		printf("IMGUI Vulkan impl failure: %s", string_VkResult(result));
	}
}

void initializeImgui(void *platformWindow, SwapchainInfo *swapchainInfo, PhysicalDeviceSurfaceCharacteristics surfaceCharacteristics, QueueInfo queueInfo, VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, VkRenderPass renderPass, char **error)
{
	VkDescriptorPool descriptorPool;
	VkDescriptorPoolSize pool_sizes[] = {
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
	};
	VkDescriptorPoolCreateInfo pool_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		.maxSets = 1,
		.poolSizeCount = (uint32_t) IM_ARRAYSIZE(pool_sizes),
		.pPoolSizes = pool_sizes
	};
	VkResult result;
	if ((result = vkCreateDescriptorPool(device, &pool_info, NULL, &descriptorPool)) != VK_SUCCESS) {
		asprintf(error, "Failed to create descriptor pool: %s", string_VkResult(result));
		sendThreadFailureSignal(platformWindow);
	}
	ImGui_CreateContext(NULL);
	ImGuiIO *io = ImGui_GetIO();
	io->IniFilename = NULL;
	ImGui_ImplModeler_Init(swapchainInfo);
	ImGui_StyleColorsDark(NULL);
	ImGui_ImplVulkan_InitInfo imVulkanInitInfo = {
		.Instance = instance,
		.PhysicalDevice = physicalDevice,
		.Device = device,
		.QueueFamily = queueInfo.graphicsQueueFamilyIndex,
		.Queue = queueInfo.graphicsQueue,
		.PipelineCache = VK_NULL_HANDLE,
		.DescriptorPool = descriptorPool,
		.Subpass = 0,
		.MinImageCount = surfaceCharacteristics.capabilities.minImageCount,
		.ImageCount = swapchainInfo->imageCount,
		.MSAASamples = VK_SAMPLE_COUNT_1_BIT,
		.Allocator = NULL,
		.CheckVkResultFn = imVkCheck
	};
	cImGui_ImplVulkan_Init(&imVulkanInitInfo, renderPass);
}

bool recreateSwapchain(SwapchainCreateInfo swapchainCreateInfo, VkExtent2D windowExtent, char **error)
{
	vkDeviceWaitIdle(swapchainCreateInfo.device);

	destroyFramebuffers(swapchainCreateInfo.device, *swapchainCreateInfo.framebuffers, swapchainCreateInfo.swapchainInfo->imageCount);
	destroyImageViews(swapchainCreateInfo.device, *swapchainCreateInfo.imageViews, swapchainCreateInfo.swapchainInfo->imageCount);
	free(*swapchainCreateInfo.imageViews);
	freePhysicalDeviceSurfaceCharacteristics(swapchainCreateInfo.surfaceCharacteristics);

	if (!getPhysicalDeviceSurfaceCharacteristics(swapchainCreateInfo.physicalDevice, swapchainCreateInfo.surface, swapchainCreateInfo.surfaceCharacteristics, error)) {
		return false;
	}

	if (!createSwapchain(swapchainCreateInfo.device, swapchainCreateInfo.surface, *swapchainCreateInfo.surfaceCharacteristics, swapchainCreateInfo.queueInfo.graphicsQueueFamilyIndex, swapchainCreateInfo.queueInfo.presentationQueueFamilyIndex, windowExtent, swapchainCreateInfo.swapchainInfo->swapchain, swapchainCreateInfo.swapchainInfo, error)) {
		return false;
	}

	*swapchainCreateInfo.imageViews = malloc(sizeof(*swapchainCreateInfo.imageViews) * swapchainCreateInfo.swapchainInfo->imageCount);
	if (!createImageViews(swapchainCreateInfo.device, swapchainCreateInfo.swapchainInfo->images, swapchainCreateInfo.swapchainInfo->imageCount, swapchainCreateInfo.swapchainInfo->surfaceFormat.format, *swapchainCreateInfo.imageViews, error)) {
		return false;
	}

	if (!createFramebuffers(swapchainCreateInfo.device, *swapchainCreateInfo.swapchainInfo, swapchainCreateInfo.offscreenImageView, *swapchainCreateInfo.imageViews, swapchainCreateInfo.renderPass, swapchainCreateInfo.framebuffers, error)) {
		return false;
	}

	return true;
}

static void cleanupVulkan(VkInstance instance, VkDebugReportCallbackEXT debugCallback, VkSurfaceKHR surface, PhysicalDeviceCharacteristics *characteristics, PhysicalDeviceSurfaceCharacteristics *surfaceCharacteristics, VkDevice device, VmaAllocator allocator, VkSwapchainKHR swapchain, VkImage *offscreenImages, VmaAllocation *offscreenImageAllocations, size_t offscreenImageCount, VkImageView *offscreenImageViews, VkImageView *imageViews, uint32_t imageViewCount, VkRenderPass renderPass, VkPipelineLayout pipelineLayout, VkPipeline pipeline, VkPipelineLayout secondPipelineLayout, VkPipeline secondPipeline, VkFramebuffer *framebuffers, uint32_t framebufferCount, VkCommandPool commandPool, VkCommandBuffer *commandBuffers, uint32_t commandBufferCount, SynchronizationInfo synchronizationInfo)
{
	destroySynchronization(device, synchronizationInfo);
	freeCommandBuffers(device, commandPool, commandBuffers, commandBufferCount);
	destroyCommandPool(device, commandPool);
	destroyFramebuffers(device, framebuffers, framebufferCount);
	destroyPipeline(device, secondPipeline);
	destroyPipelineLayout(device, secondPipelineLayout);
	destroyPipeline(device, pipeline);
	destroyPipelineLayout(device, pipelineLayout);
	destroyRenderPass(device, renderPass);
	destroyImageViews(device, imageViews, imageViewCount);
	free(imageViews);
	destroyImageViews(device, offscreenImageViews, offscreenImageCount);
	for (size_t i = 0; i < offscreenImageCount; ++i) {
		destroyImage(allocator, offscreenImages[i], offscreenImageAllocations[i]);
	}
	destroySwapchain(device, swapchain);
	destroyAllocator(allocator);
	destroyDevice(device);
	freePhysicalDeviceCharacteristics(characteristics);
	freePhysicalDeviceSurfaceCharacteristics(surfaceCharacteristics);
	destroySurface(instance, surface);
	destroyInstance(instance, debugCallback);
}
