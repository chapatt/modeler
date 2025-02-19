#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>

#ifdef ENABLE_IMGUI
#include "imgui/cimgui.h"
#include "imgui/cimgui_impl_vulkan.h"
#include "imgui/imgui_impl_modeler.h"
#endif /* ENABLE_IMGUI */

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
#include "buffer.h"
#include "utils.h"
#include "vulkan_utils.h"
#include "chess_engine.h"
#include "renderloop.h"

#ifdef EMBED_SHADERS
#include "../shader_window_border.vert.h"
#include "../shader_window_border.frag.h"
#endif /* EMBED_SHADERS */

struct swapchain_create_info_t {
	VkDevice device;
	VmaAllocator allocator;
	VkPhysicalDevice physicalDevice;
	VkSurfaceKHR surface;
	PhysicalDeviceCharacteristics physicalDeviceCharacteristics;
	PhysicalDeviceSurfaceCharacteristics *surfaceCharacteristics;
	QueueInfo queueInfo;
	VkCommandPool commandPool;
	VkRenderPass *renderPass;
	SwapchainInfo *swapchainInfo;
	VkImage *multisampleImage;
	VkImageView *multisampleImageView;
	VmaAllocation *multisampleImageAllocation;
	VkImage *offscreenImage;
	uint32_t offscreenImageCount;
	VkImageView *offscreenImageView;
	VmaAllocation *offscreenImageAllocation;
	VkImageView **imageViews;
	VkFramebuffer **framebuffers;
	VkDescriptorPool *descriptorPool;
	VkDescriptorSet *imageDescriptorSets;
	VkDescriptorSetLayout *imageDescriptorSetLayouts;
	WindowDimensions *windowDimensions;
	VkImage *depthImage;
	VmaAllocation *depthImageAllocation;
	VkFormat *depthImageFormat;
	VkImageView *depthImageView;
};

static void cleanupVulkan(VkInstance instance, VkDebugReportCallbackEXT debugCallback, VkSurfaceKHR surface, PhysicalDeviceCharacteristics *physicalDeviceCharacteristics, PhysicalDeviceSurfaceCharacteristics *surfaceCharacteristics, VkDevice device, VmaAllocator allocator, VkSwapchainKHR swapchain, VkImage *offscreenImages, VmaAllocation *offscreenImageAllocations, size_t offscreenImageCount, VkImageView *offscreenImageViews, VkImageView *imageViews, uint32_t imageViewCount, VkRenderPass renderPass, VkPipelineLayout *pipelineLayouts, VkPipeline *pipelines, size_t pipelineCount, VkFramebuffer *framebuffers, uint32_t framebufferCount, VkCommandPool commandPool, VkCommandBuffer *commandBuffers, uint32_t commandBufferCount, SynchronizationInfo synchronizationInfo, VkDescriptorPool descriptorPool, VkDescriptorSet *imageDescriptorSets, VkDescriptorSetLayout *imageDescriptorSetLayouts, ChessBoard chessBoard, VkImage depthImage, VmaAllocation depthImageAllocation, VkImageView depthImageView, VkImage multisampleImage, VkImageView multisampleImageView, VmaAllocation multisampleImageAllocation, SwapchainCreateInfo swapchainCreateInfo, VkDescriptorPool imDescriptorPool);
#ifdef ENABLE_IMGUI
void initializeImgui(void *platformWindow, SwapchainInfo *swapchainInfo, PhysicalDeviceCharacteristics physicalDeviceCharacteristics, PhysicalDeviceSurfaceCharacteristics surfaceCharacteristics, QueueInfo queueInfo, VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, VkRenderPass renderPass, VkDescriptorPool *descriptorPool, char **error);
static void imVkCheck(VkResult result);
#endif /* ENABLE_IMGUI */
static void destroyAppSwapchain(SwapchainCreateInfo swapchainCreateInfo);
static bool createAppSwapchain(SwapchainCreateInfo swapchainCreateInfo, char **error);
static bool createDepthBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator allocator, VkExtent2D extent, VkSampleCountFlagBits sampleCount, VkImage *image, VmaAllocation *imageAllocation, VkFormat *format, VkImageView *imageView, char **error);
static void destroyDepthBuffer(VkDevice device, VmaAllocator allocator, VkImage image, VmaAllocation imageAllocation, VkImageView imageView);

static inline Orientation negateRotation(Orientation orientation)
{
	switch (orientation) {
	case ROTATE_90:
		return ROTATE_270;
	case ROTATE_270:
		return ROTATE_90;
	case ROTATE_0: case ROTATE_180: default:
		return orientation;
	}
}

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
	WindowDimensions windowDimensions = threadArgs->windowDimensions;
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
	PhysicalDeviceCharacteristics physicalDeviceCharacteristics;
	PhysicalDeviceSurfaceCharacteristics surfaceCharacteristics;
	if (!choosePhysicalDevice(instance, surface, &physicalDevice, &physicalDeviceCharacteristics, &surfaceCharacteristics, error)) {
		sendThreadFailureSignal(platformWindow);
	}

#ifdef ANDROID
	uint32_t width = surfaceCharacteristics.capabilities.currentExtent.width;
	uint32_t height = surfaceCharacteristics.capabilities.currentExtent.height;
	windowDimensions.surfaceArea.width = width;
	windowDimensions.surfaceArea.height = height;
	windowDimensions.activeArea.extent.width = width;
	windowDimensions.activeArea.extent.height = height;
#endif /* ANDROID */

	VkDevice device;
	QueueInfo queueInfo = {};
	if (!createDevice(physicalDevice, surface, physicalDeviceCharacteristics, &device, &queueInfo, error)) {
		sendThreadFailureSignal(platformWindow);
	}

	VmaAllocator allocator;
	if (!createAllocator(instance, physicalDevice, device, &allocator, error)) {
		sendThreadFailureSignal(platformWindow);
	}

	VkCommandPool commandPool;
	if (!createCommandPool(device, queueInfo, &commandPool, error)) {
		sendThreadFailureSignal(platformWindow);
	}

	VkCommandBuffer *commandBuffers;
	if (!createCommandBuffers(device, commandPool, &commandBuffers, error)) {
		sendThreadFailureSignal(platformWindow);
	}

	SwapchainInfo swapchainInfo = {};
	VkImageView *imageViews;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet imageDescriptorSet;
	VkDescriptorSetLayout imageDescriptorSetLayout;
#ifdef DRAW_WINDOW_DECORATION
	VkImage offscreenImage;
	VmaAllocation offscreenImageAllocation;
	VkImageView offscreenImageView;
#endif /* DRAW_WINDOW_DECORATION */
	VkRenderPass renderPass;
	VkFramebuffer *framebuffers;
	VkImage multisampleImage;
	VkImageView multisampleImageView;
	VmaAllocation multisampleImageAllocation;
	VkImage depthImage;
	VmaAllocation depthImageAllocation;
	VkFormat depthImageFormat;
	VkImageView depthImageView;

	struct swapchain_create_info_t swapchainCreateInfo = {
		.device = device,
		.allocator = allocator,
		.physicalDevice = physicalDevice,
		.surface = surface,
		.physicalDeviceCharacteristics = physicalDeviceCharacteristics,
		.surfaceCharacteristics = &surfaceCharacteristics,
		.queueInfo = queueInfo,
		.renderPass = &renderPass,
		.swapchainInfo = &swapchainInfo,
		.imageViews = &imageViews,
		.framebuffers = &framebuffers,
		.windowDimensions = &windowDimensions,
		.commandPool = commandPool,
		.depthImage = &depthImage,
		.depthImageAllocation = &depthImageAllocation,
		.depthImageFormat = &depthImageFormat,
		.depthImageView = &depthImageView,
		.multisampleImage = &multisampleImage,
		.multisampleImageView = &multisampleImageView,
		.multisampleImageAllocation = &multisampleImageAllocation,
#ifdef DRAW_WINDOW_DECORATION
		.descriptorPool = &descriptorPool,
		.imageDescriptorSetLayouts = &imageDescriptorSetLayout,
		.imageDescriptorSets = &imageDescriptorSet,
		.offscreenImage = &offscreenImage,
		.offscreenImageCount = 1,
		.offscreenImageView = &offscreenImageView,
		.offscreenImageAllocation = &offscreenImageAllocation
#else
		.offscreenImage = NULL,
		.offscreenImageCount = 0,
		.offscreenImageView = NULL,
		.offscreenImageAllocation = NULL
#endif /* DRAW_WINDOW_DECORATION */
	};

	if (!createAppSwapchain(&swapchainCreateInfo, error)) {
		sendThreadFailureSignal(platformWindow);
	}

	SynchronizationInfo synchronizationInfo;
	if (!createSynchronization(device, &synchronizationInfo, error)) {
		sendThreadFailureSignal(platformWindow);
	}

	ChessBoard chessBoard;
	ChessEngine chessEngine;
	createChessEngine(&chessEngine, &chessBoard);

	if (!createChessBoard(&chessBoard, chessEngine, device, allocator, commandPool, queueInfo.graphicsQueue, renderPass, 0, getMaxSampleCount(physicalDeviceCharacteristics.deviceProperties), resourcePath, 1.0f, -0.5f, -0.5f, negateRotation(windowDimensions.orientation), true, PERSPECTIVE, error)) {
		sendThreadFailureSignal(platformWindow);
	}

#ifndef EMBED_SHADERS
#ifdef DRAW_WINDOW_DECORATION
	char *windowBorderVertShaderPath;
	char *windowBorderFragShaderPath;
	asprintf(&windowBorderVertShaderPath, "%s/%s", resourcePath, "window_border.vert.spv");
	asprintf(&windowBorderFragShaderPath, "%s/%s", resourcePath, "window_border.frag.spv");
	char *windowBorderVertShaderBytes;
	char *windowBorderFragShaderBytes;
	uint32_t windowBorderVertShaderSize = 0;
	uint32_t windowBorderFragShaderSize = 0;

	if ((windowBorderVertShaderSize = readFileToString(windowBorderVertShaderPath, &windowBorderVertShaderBytes)) == -1) {
		asprintf(error, "Failed to open window border vertex shader for reading.\n");
		sendThreadFailureSignal(platformWindow);
	}
	if ((windowBorderFragShaderSize = readFileToString(windowBorderFragShaderPath, &windowBorderFragShaderBytes)) == -1) {
		asprintf(error, "Failed to open window border fragment shader for reading.\n");
		sendThreadFailureSignal(platformWindow);
	}
#endif /* DRAW_WINDOW_DECORATION */
#endif /* EMBED_SHADERS */

#ifdef DRAW_WINDOW_DECORATION
	VkPipelineLayout pipelineLayoutWindowDecoration;
	VkPipeline pipelineWindowDecoration;
	VkPushConstantRange pushConstantRange = {
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.offset = 0,
		.size = sizeof(PushConstants)
	};
	VkPipelineDepthStencilStateCreateInfo depthStencilState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_FALSE,
		.depthWriteEnable = VK_FALSE,
		.depthCompareOp = VK_COMPARE_OP_NEVER,
		.depthBoundsTestEnable = VK_FALSE,
		.minDepthBounds = 0.0f,
		.maxDepthBounds = 1.0f,
		.stencilTestEnable = VK_FALSE,
		.front = {},
		.back = {}
	};
	PipelineCreateInfo pipelineCreateInfoWindowDecoration = {
		.device = device,
		.renderPass = renderPass,
#ifdef ENABLE_IMGUI
		.subpassIndex = 2,
#else
		.subpassIndex = 1,
#endif /* ENABLE_IMGUI */
		.vertexShaderBytes = windowBorderVertShaderBytes,
		.vertexShaderSize = windowBorderVertShaderSize,
		.fragmentShaderBytes = windowBorderFragShaderBytes,
		.fragmentShaderSize = windowBorderFragShaderSize,
		.vertexBindingDescriptionCount = 0,
		.vertexBindingDescriptions = NULL,
		.vertexAttributeDescriptionCount = 0,
		.VertexAttributeDescriptions = NULL,
		.descriptorSetLayouts = &imageDescriptorSetLayout,
		.descriptorSetLayoutCount = 1,
		.pushConstantRange = pushConstantRange,
		.depthStencilState = depthStencilState,
		.sampleCount = VK_SAMPLE_COUNT_1_BIT
	};
	bool pipelineCreateSuccessWindowDecoration = createPipeline(pipelineCreateInfoWindowDecoration, &pipelineLayoutWindowDecoration, &pipelineWindowDecoration, error);
#ifndef EMBED_SHADERS
	free(windowBorderFragShaderBytes);
	free(windowBorderVertShaderBytes);
#endif /* EMBED_SHADERS */
	if (!pipelineCreateSuccessWindowDecoration) {
		sendThreadFailureSignal(platformWindow);
	}
#endif /* DRAW_WINDOW_DECORATION */

	VkDescriptorPool imDescriptorPool;
#ifdef ENABLE_IMGUI
	ImGui_ImplVulkan_InitInfo imVulkanInitInfo;
	initializeImgui(platformWindow, &swapchainInfo, physicalDeviceCharacteristics, surfaceCharacteristics, queueInfo, instance, physicalDevice, device, renderPass, &imDescriptorPool, error);
#endif /* ENABLE_IMGUI */

#if DRAW_WINDOW_DECORATION
	VkPipeline pipelines[] = {pipelineWindowDecoration};
	VkPipelineLayout pipelineLayouts[] = {pipelineLayoutWindowDecoration};
	size_t pipelineCount = 1;
	VkDescriptorSet *drawDescriptorSets = &imageDescriptorSet;
#else
	VkPipeline pipelines[] = {};
	VkPipelineLayout pipelineLayouts[] = {};
	size_t pipelineCount = 0;
	VkDescriptorSet *drawDescriptorSets = NULL;
#endif /* DRAW_WINDOW_DECORATION */

	if (!draw(device, platformWindow, &windowDimensions, drawDescriptorSets, &renderPass, pipelines, pipelineLayouts, &framebuffers, commandBuffers, synchronizationInfo, &swapchainInfo, queueInfo.graphicsQueue, queueInfo.presentationQueue, queueInfo.graphicsQueueFamilyIndex, resourcePath, inputQueue, &swapchainCreateInfo, chessBoard, chessEngine, error)) {
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
	VkImageView *offscreenImageViews = NULL;
#endif /* DRAW_WINDOW_DECORATION */

	cleanupVulkan(instance, debugCallback, surface, &physicalDeviceCharacteristics, &surfaceCharacteristics, device, allocator, swapchainInfo.swapchain, offscreenImages, offscreenImageAllocations, offscreenImageCount, offscreenImageViews, imageViews, swapchainInfo.imageCount, renderPass, pipelineLayouts, pipelines, pipelineCount, framebuffers, swapchainInfo.imageCount, commandPool, commandBuffers, MAX_FRAMES_IN_FLIGHT, synchronizationInfo, descriptorPool, &imageDescriptorSet, &imageDescriptorSetLayout, chessBoard, depthImage, depthImageAllocation, depthImageView, multisampleImage, multisampleImageView, multisampleImageAllocation, &swapchainCreateInfo, imDescriptorPool);

	return NULL;
}

#ifdef ENABLE_IMGUI
static void imVkCheck(VkResult result)
{
	if (result != VK_SUCCESS) {
		fprintf(stderr, "IMGUI Vulkan impl failure: %s", string_VkResult(result));
	}
}

void initializeImgui(void *platformWindow, SwapchainInfo *swapchainInfo, PhysicalDeviceCharacteristics physicalDeviceCharacteristics, PhysicalDeviceSurfaceCharacteristics surfaceCharacteristics, QueueInfo queueInfo, VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, VkRenderPass renderPass, VkDescriptorPool *descriptorPool, char **error)
{
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
	if ((result = vkCreateDescriptorPool(device, &pool_info, NULL, descriptorPool)) != VK_SUCCESS) {
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
		.DescriptorPool = *descriptorPool,
		.Subpass = 1,
		.MinImageCount = surfaceCharacteristics.capabilities.minImageCount,
		.ImageCount = swapchainInfo->imageCount,
		.MSAASamples = getMaxSampleCount(physicalDeviceCharacteristics.deviceProperties),
		.Allocator = NULL,
		.CheckVkResultFn = imVkCheck
	};
	cImGui_ImplVulkan_Init(&imVulkanInitInfo, renderPass);
}
#endif /* ENABLE_IMGUI */

bool recreateSwapchain(SwapchainCreateInfo swapchainCreateInfo, char **error)
{
	vkDeviceWaitIdle(swapchainCreateInfo->device);

	destroyAppSwapchain(swapchainCreateInfo);

	if (!getPhysicalDeviceSurfaceCharacteristics(swapchainCreateInfo->physicalDevice, swapchainCreateInfo->surface, swapchainCreateInfo->surfaceCharacteristics, error)) {
		return false;
	}

	createAppSwapchain(swapchainCreateInfo, error);

	return true;
}

void destroyAppSwapchain(SwapchainCreateInfo swapchainCreateInfo)
{
	destroyFramebuffers(swapchainCreateInfo->device, *swapchainCreateInfo->framebuffers, swapchainCreateInfo->swapchainInfo->imageCount);
	destroyRenderPass(swapchainCreateInfo->device, *swapchainCreateInfo->renderPass);
#ifdef DRAW_WINDOW_DECORATION
	for (size_t i = 0; i < swapchainCreateInfo->offscreenImageCount; ++i) {
		destroyDescriptorSetLayout(swapchainCreateInfo->device, (swapchainCreateInfo->imageDescriptorSetLayouts)[i]);
	}
	destroyDescriptorPool(swapchainCreateInfo->device, *swapchainCreateInfo->descriptorPool);
	destroyImageView(swapchainCreateInfo->device, *swapchainCreateInfo->offscreenImageView);
	destroyImage(swapchainCreateInfo->allocator, *swapchainCreateInfo->offscreenImage, *swapchainCreateInfo->offscreenImageAllocation);
#endif
	destroyImageView(swapchainCreateInfo->device, *swapchainCreateInfo->multisampleImageView);
	destroyImage(swapchainCreateInfo->allocator, *swapchainCreateInfo->multisampleImage, *swapchainCreateInfo->multisampleImageAllocation);
	destroyDepthBuffer(swapchainCreateInfo->device, swapchainCreateInfo->allocator, *swapchainCreateInfo->depthImage, *swapchainCreateInfo->depthImageAllocation, *swapchainCreateInfo->depthImageView);
	destroyImageViews(swapchainCreateInfo->device, *swapchainCreateInfo->imageViews, swapchainCreateInfo->swapchainInfo->imageCount);
	free(*swapchainCreateInfo->imageViews);
	freePhysicalDeviceSurfaceCharacteristics(swapchainCreateInfo->surfaceCharacteristics);
}

bool createAppSwapchain(SwapchainCreateInfo swapchainCreateInfo, char **error)
{
	VkExtent2D requestedExtent = swapchainCreateInfo->windowDimensions->surfaceArea;

	if (swapchainCreateInfo->surfaceCharacteristics->capabilities.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR ||
		swapchainCreateInfo->surfaceCharacteristics->capabilities.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR
	) {
		uint32_t width = requestedExtent.width;
		requestedExtent.width = requestedExtent.height;
		requestedExtent.height = width;
	}

	enum VkSurfaceTransformFlagBitsKHR transform = swapchainCreateInfo->surfaceCharacteristics->capabilities.currentTransform;
	if (transform & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR) {
		swapchainCreateInfo->windowDimensions->orientation = ROTATE_90;
	} else if (transform & VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR) {
		swapchainCreateInfo->windowDimensions->orientation = ROTATE_180;
	} else if (transform & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR) {
		swapchainCreateInfo->windowDimensions->orientation = ROTATE_270;
	} else {
		swapchainCreateInfo->windowDimensions->orientation = ROTATE_0;
	}

	if (!createSwapchain(swapchainCreateInfo->device, swapchainCreateInfo->surface, *swapchainCreateInfo->surfaceCharacteristics, swapchainCreateInfo->queueInfo.graphicsQueueFamilyIndex, swapchainCreateInfo->queueInfo.presentationQueueFamilyIndex, requestedExtent, swapchainCreateInfo->swapchainInfo->swapchain, swapchainCreateInfo->swapchainInfo, error)) {
		return false;
	}

	VkExtent2D savedExtent = swapchainCreateInfo->swapchainInfo->extent;

	if (swapchainCreateInfo->surfaceCharacteristics->capabilities.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR ||
		swapchainCreateInfo->surfaceCharacteristics->capabilities.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR
	) {
		uint32_t width = savedExtent.width;
		savedExtent.width = savedExtent.height;
		savedExtent.height = width;
	}

	swapchainCreateInfo->windowDimensions->surfaceArea = savedExtent;
	swapchainCreateInfo->windowDimensions->activeArea.extent = savedExtent;

	*swapchainCreateInfo->imageViews = malloc(sizeof(*swapchainCreateInfo->imageViews) * swapchainCreateInfo->swapchainInfo->imageCount);
	if (!createImageViews(swapchainCreateInfo->device, swapchainCreateInfo->swapchainInfo->images, swapchainCreateInfo->swapchainInfo->imageCount, swapchainCreateInfo->swapchainInfo->surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, *swapchainCreateInfo->imageViews, error)) {
		return false;
	}

#ifdef DRAW_WINDOW_DECORATION
	if (!createImage(swapchainCreateInfo->device, swapchainCreateInfo->allocator, swapchainCreateInfo->swapchainInfo->extent, swapchainCreateInfo->swapchainInfo->surfaceFormat.format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, 1, VK_SAMPLE_COUNT_1_BIT, swapchainCreateInfo->offscreenImage, swapchainCreateInfo->offscreenImageAllocation, error)) {
		return false;
	}

	if (!createImageView(swapchainCreateInfo->device, *swapchainCreateInfo->offscreenImage, swapchainCreateInfo->swapchainInfo->surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, 1, swapchainCreateInfo->offscreenImageView, error)) {
		return false;
	}

	VkDescriptorImageInfo imageDescriptorInfo = {
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.imageView = *swapchainCreateInfo->offscreenImageView,
		.sampler = NULL
	};

	VkDescriptorSetLayoutBinding imageBinding = {
		.binding = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = NULL
	};

	void *descriptorSetDescriptorInfos[] = {&imageDescriptorInfo};
	VkDescriptorSetLayoutBinding descriptorSetBindings[] = {imageBinding};
	CreateDescriptorSetInfo createDescriptorSetInfo = {
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.descriptorInfos = descriptorSetDescriptorInfos,
		.descriptorCount = 1,
		.bindings = descriptorSetBindings,
		.bindingCount = 1
	};

	if (!createDescriptorSets(swapchainCreateInfo->device, &createDescriptorSetInfo, 1, swapchainCreateInfo->descriptorPool, swapchainCreateInfo->imageDescriptorSets, swapchainCreateInfo->imageDescriptorSetLayouts, error)) {
		return false;
	}
#endif /* DRAW_WINDOW_DECORATION */

	VkSampleCountFlagBits sampleCount = getMaxSampleCount(swapchainCreateInfo->physicalDeviceCharacteristics.deviceProperties);
	if (!createImage(swapchainCreateInfo->device, swapchainCreateInfo->allocator, swapchainCreateInfo->swapchainInfo->extent, swapchainCreateInfo->swapchainInfo->surfaceFormat.format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, 1, sampleCount, swapchainCreateInfo->multisampleImage, swapchainCreateInfo->multisampleImageAllocation, error)) {
		return false;
	}

	if (!createImageView(swapchainCreateInfo->device, *swapchainCreateInfo->multisampleImage, swapchainCreateInfo->swapchainInfo->surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, 1, swapchainCreateInfo->multisampleImageView, error)) {
		return false;
	}

	if (!createDepthBuffer(swapchainCreateInfo->physicalDevice, swapchainCreateInfo->device, swapchainCreateInfo->allocator, swapchainCreateInfo->swapchainInfo->extent, sampleCount, swapchainCreateInfo->depthImage, swapchainCreateInfo->depthImageAllocation, swapchainCreateInfo->depthImageFormat, swapchainCreateInfo->depthImageView, error)) {
		return false;
	}

	if (!createRenderPass(swapchainCreateInfo->device, *swapchainCreateInfo->swapchainInfo, *swapchainCreateInfo->depthImageFormat, sampleCount, swapchainCreateInfo->renderPass, error)) {
		return false;
	}

	*swapchainCreateInfo->framebuffers = malloc(sizeof(**swapchainCreateInfo->framebuffers) * swapchainCreateInfo->swapchainInfo->imageCount);
	for (uint32_t i = 0; i < swapchainCreateInfo->swapchainInfo->imageCount; ++i) {
#ifdef DRAW_WINDOW_DECORATION
		VkImageView attachments[] = {*swapchainCreateInfo->multisampleImageView, *swapchainCreateInfo->depthImageView, *swapchainCreateInfo->offscreenImageView, (*swapchainCreateInfo->imageViews)[i]};
		uint32_t attachmentCount = 4;
#else
		VkImageView attachments[] = {*swapchainCreateInfo->multisampleImageView, *swapchainCreateInfo->depthImageView, (*swapchainCreateInfo->imageViews)[i]};
		uint32_t attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
#endif /* DRAW_WINDOW_DECORATION */

		if (!createFramebuffer(swapchainCreateInfo->device, *swapchainCreateInfo->swapchainInfo, attachments, attachmentCount, *swapchainCreateInfo->renderPass, *swapchainCreateInfo->framebuffers + i, error)) {
			return false;
		}
	}

	return true;
}

static bool createDepthBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator allocator, VkExtent2D extent, VkSampleCountFlagBits sampleCount, VkImage *image, VmaAllocation *imageAllocation, VkFormat *format, VkImageView *imageView, char **error)
{
	VkFormat formats[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
	*format = findSupportedFormat(physicalDevice,
		formats,
		sizeof(formats),
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);

	if (*format == VK_FORMAT_UNDEFINED) {
		asprintf(error, "Failed to find suitable depth buffer format.\n");
		return false;
	}

	if (!createImage(device, allocator, extent, *format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 1, sampleCount, image, imageAllocation, error)) {
		return false;
	}

	if (!createImageView(device, *image, *format, VK_IMAGE_ASPECT_DEPTH_BIT, 1, imageView, error)) {
		return false;
	}

	return true;
}

static void destroyDepthBuffer(VkDevice device, VmaAllocator allocator, VkImage image, VmaAllocation imageAllocation, VkImageView imageView)
{
	destroyImageView(device, imageView);
	destroyImage(allocator, image, imageAllocation);
}

static void cleanupVulkan(VkInstance instance, VkDebugReportCallbackEXT debugCallback, VkSurfaceKHR surface, PhysicalDeviceCharacteristics *physicalDeviceCharacteristics, PhysicalDeviceSurfaceCharacteristics *surfaceCharacteristics, VkDevice device, VmaAllocator allocator, VkSwapchainKHR swapchain, VkImage *offscreenImages, VmaAllocation *offscreenImageAllocations, size_t offscreenImageCount, VkImageView *offscreenImageViews, VkImageView *imageViews, uint32_t imageViewCount, VkRenderPass renderPass, VkPipelineLayout *pipelineLayouts, VkPipeline *pipelines, size_t pipelineCount, VkFramebuffer *framebuffers, uint32_t framebufferCount, VkCommandPool commandPool, VkCommandBuffer *commandBuffers, uint32_t commandBufferCount, SynchronizationInfo synchronizationInfo, VkDescriptorPool descriptorPool, VkDescriptorSet *imageDescriptorSets, VkDescriptorSetLayout *imageDescriptorSetLayouts, ChessBoard chessBoard, VkImage depthImage, VmaAllocation depthImageAllocation, VkImageView depthImageView, VkImage multisampleImage, VkImageView multisampleImageView, VmaAllocation multisampleImageAllocation, SwapchainCreateInfo swapchainCreateInfo, VkDescriptorPool imDescriptorPool)
{
#ifdef ENABLE_IMGUI
	cImGui_ImplVulkan_Shutdown();
	destroyDescriptorPool(device, imDescriptorPool);
#endif /* ENABLE_IMGUI */
	destroyChessBoard(chessBoard);
	destroySynchronization(device, synchronizationInfo);
	freeCommandBuffers(device, commandPool, commandBuffers, commandBufferCount);
	destroyCommandPool(device, commandPool);
	for (size_t i = 0; i < pipelineCount; ++i) {
		destroyPipeline(device, pipelines[i]);
		destroyPipelineLayout(device, pipelineLayouts[i]);
	}
	destroyAppSwapchain(swapchainCreateInfo);
	destroySwapchain(device, swapchain);
	freePhysicalDeviceCharacteristics(physicalDeviceCharacteristics);
	destroySurface(instance, surface);
	destroyAllocator(allocator);
	destroyDevice(device);
	destroyInstance(instance, debugCallback);
}
