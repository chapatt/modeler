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
#include "buffer.h"
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
static void cleanupVulkan(VkInstance instance, VkDebugReportCallbackEXT debugCallback, VkSurfaceKHR surface, PhysicalDeviceCharacteristics *characteristics, PhysicalDeviceSurfaceCharacteristics *surfaceCharacteristics, VkDevice device, VmaAllocator allocator, VkSwapchainKHR swapchain, VkImage *offscreenImages, VmaAllocation *offscreenImageAllocations, size_t offscreenImageCount, VkImageView *offscreenImageViews, VkImageView *imageViews, uint32_t imageViewCount, VkRenderPass renderPass, VkPipelineLayout *pipelineLayouts, VkPipeline *pipelines, size_t pipelineCount, VkFramebuffer *framebuffers, uint32_t framebufferCount, VkCommandPool commandPool, VkCommandBuffer *commandBuffers, uint32_t commandBufferCount, SynchronizationInfo synchronizationInfo, VkDescriptorPool descriptorPool, VkDescriptorSet *imageDescriptorSets, VkDescriptorSetLayout *imageDescriptorSetLayouts, VkBuffer vertexBuffer, VmaAllocation vertexBufferAllocation, VkBuffer indexBuffer, VmaAllocation indexBufferAllocation);
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
	if (!createSwapchain(device, surface, surfaceCharacteristics, queueInfo.graphicsQueueFamilyIndex, queueInfo.presentationQueueFamilyIndex, windowDimensions.surfaceArea, VK_NULL_HANDLE, &swapchainInfo, error)) {
		sendThreadFailureSignal(platformWindow);
	}

	VkImageView *imageViews = malloc(sizeof(imageViews) * swapchainInfo.imageCount);
	if (!createImageViews(device, swapchainInfo.images, swapchainInfo.imageCount, swapchainInfo.surfaceFormat.format, imageViews, error)) {
		sendThreadFailureSignal(platformWindow);
	}

	VkDescriptorPool descriptorPool;
	VkDescriptorSet *imageDescriptorSets;
	VkDescriptorSetLayout *imageDescriptorSetLayouts;
	VkDescriptorSet *bufferDescriptorSets;
	VkDescriptorSetLayout *bufferDescriptorSetLayouts;
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
	createDescriptorSets(device, createDescriptorSetInfo, &descriptorPool, &imageDescriptorSets, &imageDescriptorSetLayouts, &bufferDescriptorSets, &bufferDescriptorSetLayouts, error);
#endif /* DRAW_WINDOW_DECORATION */

	VkRenderPass renderPass;
	if (!createRenderPass(device, swapchainInfo, &renderPass, error)) {
		sendThreadFailureSignal(platformWindow);
	}

	VkFramebuffer *framebuffers = malloc(sizeof(framebuffers) * swapchainInfo.imageCount);
	for (uint32_t i = 0; i < swapchainInfo.imageCount; ++i) {
#ifdef DRAW_WINDOW_DECORATION
		VkImageView attachments[] = {offscreenImageView, imageViews[i]};
		uint32_t attachmentCount = 2;
#else
		VkImageView attachments[] = {imageViews[i]};
		uint32_t attachmentCount = 1;
#endif /* DRAW_WINDOW_DECORATION */

		if (!createFramebuffer(device, swapchainInfo, attachments, attachmentCount, renderPass, framebuffers + i, error)) {
			sendThreadFailureSignal(platformWindow);
		}
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

	VkVertexInputBindingDescription bindingDescription = {
		.binding = 0,
		.stride = sizeof(Vertex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	};

	VkVertexInputAttributeDescription attributeDescriptions[] = {
		{
			.binding = 0,
			.location = 0,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = offsetof(Vertex, pos),
		}, {
			.binding = 0,
			.location = 1,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(Vertex, color),
		}
	};

	float black[3] = {0.0f, 0.0f, 0.0f};
	float white[3] = {1.0f, 1.0f, 1.0f};
	Vertex triangleVertices[256];
	uint16_t triangleIndices[384];
	for (size_t i = 0; i < 64; ++i) {
		size_t verticesOffset = i * 4;
		size_t indicesOffset = i * 6;
		size_t offsetX = i % 8;
		size_t offsetY = i / 8;
		float width = 1.0f / 8.0f;
		float height = 1.0f / 8.0f;
		float originX = offsetX * width;
		float originY = offsetY * height;
		float *color;
		color = (offsetY % 2) ?
			((offsetX % 2) ? black : white) :
			(offsetX % 2) ? white : black;

		triangleVertices[verticesOffset] = (Vertex) {{originX, originY}, {color[0], color[1], color[2]}};
		triangleVertices[verticesOffset + 1] = (Vertex) {{originX + width, originY}, {color[0], color[1], color[2]}};
		triangleVertices[verticesOffset + 2] = (Vertex) {{originX + width, originY + height}, {color[0], color[1], color[2]}};
		triangleVertices[verticesOffset + 3] = (Vertex) {{originX, originY + height}, {color[0], color[1], color[2]}};
	
		triangleIndices[indicesOffset] = verticesOffset + 0;
		triangleIndices[indicesOffset + 1] = verticesOffset + 1;
		triangleIndices[indicesOffset + 2] = verticesOffset + 2;
		triangleIndices[indicesOffset + 3] = verticesOffset + 2;
		triangleIndices[indicesOffset + 4] = verticesOffset + 3;
		triangleIndices[indicesOffset + 5] = verticesOffset + 0;
	}

	VkBuffer vertexBuffer;
	VmaAllocation vertexBufferAllocation;
	VkBuffer indexBuffer;
	VmaAllocation indexBufferAllocation;

	if (!createVertexBuffer(device, allocator, commandPool, queueInfo.graphicsQueue, &vertexBuffer, &vertexBufferAllocation, triangleVertices, sizeof(triangleVertices) / sizeof(triangleVertices[0]), error)) {
		sendThreadFailureSignal(platformWindow);
	}
	if (!createIndexBuffer(device, allocator, commandPool, queueInfo.graphicsQueue, &indexBuffer, &indexBufferAllocation, triangleIndices, sizeof(triangleIndices) / sizeof(triangleIndices[0]), error)) {
		sendThreadFailureSignal(platformWindow);
	}

#ifndef EMBED_SHADERS
	char *triangleVertShaderPath;
	char *triangleFragShaderPath;
	asprintf(&triangleVertShaderPath, "%s/%s", resourcePath, "triangle.vert.spv");
	asprintf(&triangleFragShaderPath, "%s/%s", resourcePath, "triangle.frag.spv");
	char *triangleVertShaderBytes;
	char *triangleFragShaderBytes;
	uint32_t triangleVertShaderSize = 0;
	uint32_t triangleFragShaderSize = 0;

	if ((triangleVertShaderSize = readFileToString(triangleVertShaderPath, &triangleVertShaderBytes)) == -1) {
		asprintf(error, "Failed to open triangle vertex shader for reading.\n");
		sendThreadFailureSignal(platformWindow);
	}
	if ((triangleFragShaderSize = readFileToString(triangleFragShaderPath, &triangleFragShaderBytes)) == -1) {
		asprintf(error, "Failed to open triangle fragment shader for reading.\n");
		sendThreadFailureSignal(platformWindow);
	}

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

	VkPipelineLayout pipelineLayoutTriangle;
	VkPipeline pipelineTriangle;
	PipelineCreateInfo pipelineCreateInfoTriangle = {
		.device = device,
		.renderPass = renderPass,
		.subpassIndex = 0,
		.vertexShaderBytes = triangleVertShaderBytes,
		.vertexShaderSize = triangleVertShaderSize,
		.fragmentShaderBytes = triangleFragShaderBytes,
		.fragmentShaderSize = triangleFragShaderSize,
		.extent = swapchainInfo.extent,
		.vertexBindingDescriptionCount = 1,
		.vertexBindingDescriptions = &bindingDescription,
		.vertexAttributeDescriptionCount = 2,
		.VertexAttributeDescriptions = attributeDescriptions,
		.descriptorSetLayouts = NULL,
		.descriptorSetLayoutCount = 0,
	};
	bool pipelineCreateSuccessTriangle = createPipeline(pipelineCreateInfoTriangle, &pipelineLayoutTriangle, &pipelineTriangle, error);
#ifndef EMBED_SHADERS
	free(triangleFragShaderBytes);
	free(triangleVertShaderBytes);
#endif /* EMBED_SHADERS */
	if (!pipelineCreateSuccessTriangle) {
		sendThreadFailureSignal(platformWindow);
	}

#ifdef DRAW_WINDOW_DECORATION
	VkPipelineLayout pipelineLayoutWindowDecoration;
	VkPipeline pipelineWindowDecoration;
	PipelineCreateInfo pipelineCreateInfoWindowDecoration = {
		.device = device,
		.renderPass = renderPass,
		.subpassIndex = 2,
		.vertexShaderBytes = windowBorderVertShaderBytes,
		.vertexShaderSize = windowBorderVertShaderSize,
		.fragmentShaderBytes = windowBorderFragShaderBytes,
		.fragmentShaderSize = windowBorderFragShaderSize,
		.extent = swapchainInfo.extent,
		.vertexBindingDescriptionCount = 0,
		.vertexBindingDescriptions = NULL,
		.vertexAttributeDescriptionCount = 0,
		.VertexAttributeDescriptions = NULL,
		.descriptorSetLayouts = imageDescriptorSetLayouts,
		.descriptorSetLayoutCount = 1,
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

#if DRAW_WINDOW_DECORATION
	VkPipeline pipelines[] = {pipelineTriangle, pipelineWindowDecoration};
	VkPipelineLayout pipelineLayouts[] = {pipelineLayoutTriangle, pipelineLayoutWindowDecoration};
	size_t pipelineCount = 2;
	VkDescriptorSet **drawDescriptorSets = &imageDescriptorSets;
#else
	VkPipeline pipelines[] = {pipelineTriangle};
	VkPipelineLayout pipelineLayouts[] = {pipelineLayoutTriangle};
	size_t pipelineCount = 1;
	VkDescriptorSet **drawDescriptorSets = NULL;
#endif /* DRAW_WINDOW_DECORATION */

	SwapchainCreateInfo swapchainCreateInfo = {
		.device = device,
		.allocator = allocator,
		.physicalDevice = physicalDevice,
		.surface = surface,
		.surfaceCharacteristics = &surfaceCharacteristics,
		.queueInfo = queueInfo,
		.renderPass = &renderPass,
		.swapchainInfo = &swapchainInfo,
		.imageViews = &imageViews,
		.framebuffers = &framebuffers,
#ifdef DRAW_WINDOW_DECORATION
		.descriptorPool = &descriptorPool,
		.imageDescriptorSetLayouts = &imageDescriptorSetLayouts,
		.imageDescriptorSets = &imageDescriptorSets,
		.bufferDescriptorSetLayouts = &bufferDescriptorSetLayouts,
		.bufferDescriptorSets = &bufferDescriptorSets,
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

	VkDescriptorPool imDescriptorPool;
	ImGui_ImplVulkan_InitInfo imVulkanInitInfo;
	initializeImgui(platformWindow, &swapchainInfo, surfaceCharacteristics, queueInfo, instance, physicalDevice, device, renderPass, error);

	if (!draw(device, platformWindow, windowDimensions, drawDescriptorSets, &renderPass, pipelines, pipelineLayouts, &framebuffers, &commandBuffers, synchronizationInfo, &swapchainInfo, queueInfo.graphicsQueue, queueInfo.presentationQueue, queueInfo.graphicsQueueFamilyIndex, resourcePath, inputQueue, swapchainCreateInfo, vertexBuffer, indexBuffer, sizeof(triangleIndices) / sizeof(*triangleIndices), error)) {
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

	cleanupVulkan(instance, debugCallback, surface, &characteristics, &surfaceCharacteristics, device, allocator, swapchainInfo.swapchain, offscreenImages, offscreenImageAllocations, offscreenImageCount, offscreenImageViews, imageViews, swapchainInfo.imageCount, renderPass, pipelineLayouts, pipelines, pipelineCount, framebuffers, swapchainInfo.imageCount, commandPool, commandBuffers, swapchainInfo.imageCount, synchronizationInfo, descriptorPool, imageDescriptorSets, imageDescriptorSetLayouts, vertexBuffer, vertexBufferAllocation, indexBuffer, indexBufferAllocation);

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
#ifdef ENABLE_IMGUI
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
		.Subpass = 1,
		.MinImageCount = surfaceCharacteristics.capabilities.minImageCount,
		.ImageCount = swapchainInfo->imageCount,
		.MSAASamples = VK_SAMPLE_COUNT_1_BIT,
		.Allocator = NULL,
		.CheckVkResultFn = imVkCheck
	};
	cImGui_ImplVulkan_Init(&imVulkanInitInfo, renderPass);
#endif /* ENABLE_IMGUI */
}

bool recreateSwapchain(SwapchainCreateInfo swapchainCreateInfo, VkExtent2D windowExtent, char **error)
{
	vkDeviceWaitIdle(swapchainCreateInfo.device);

	destroyFramebuffers(swapchainCreateInfo.device, *swapchainCreateInfo.framebuffers, swapchainCreateInfo.swapchainInfo->imageCount);
	destroyRenderPass(swapchainCreateInfo.device, *swapchainCreateInfo.renderPass);
#ifdef DRAW_WINDOW_DECORATION
	for (size_t i = 0; i < swapchainCreateInfo.offscreenImageCount; ++i) {
		destroyDescriptorSetLayout(swapchainCreateInfo.device, (*swapchainCreateInfo.imageDescriptorSetLayouts)[i]);
	}
	free(*swapchainCreateInfo.imageDescriptorSetLayouts);
	destroyDescriptorPool(swapchainCreateInfo.device, *swapchainCreateInfo.descriptorPool);
	free(*swapchainCreateInfo.imageDescriptorSets);
	destroyImageViews(swapchainCreateInfo.device, swapchainCreateInfo.offscreenImageView, 1);
	destroyImage(swapchainCreateInfo.allocator, *swapchainCreateInfo.offscreenImage, *swapchainCreateInfo.offscreenImageAllocation);
#endif
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

#ifdef DRAW_WINDOW_DECORATION
	if (!createImage(swapchainCreateInfo.device, swapchainCreateInfo.allocator, swapchainCreateInfo.swapchainInfo->extent, swapchainCreateInfo.swapchainInfo->surfaceFormat.format, swapchainCreateInfo.offscreenImage, swapchainCreateInfo.offscreenImageAllocation, error)) {
		return false;
	}

	if (!createImageViews(swapchainCreateInfo.device, swapchainCreateInfo.offscreenImage, 1, swapchainCreateInfo.swapchainInfo->surfaceFormat.format, swapchainCreateInfo.offscreenImageView, error)) {
		return false;
	}

	VkImageLayout imageLayouts[] = {VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
	VkSampler samplers[] = {VK_NULL_HANDLE};
	CreateDescriptorSetInfo createDescriptorSetInfo = {
		swapchainCreateInfo.offscreenImageView,
		imageLayouts,
		samplers,
		1,
		NULL,
		NULL,
		NULL,
		0
	};
	createDescriptorSets(swapchainCreateInfo.device, createDescriptorSetInfo, swapchainCreateInfo.descriptorPool, swapchainCreateInfo.imageDescriptorSets, swapchainCreateInfo.imageDescriptorSetLayouts, swapchainCreateInfo.bufferDescriptorSets, swapchainCreateInfo.bufferDescriptorSetLayouts, error);
#endif /* DRAW_WINDOW_DECORATION */

	if (!createRenderPass(swapchainCreateInfo.device, *swapchainCreateInfo.swapchainInfo, swapchainCreateInfo.renderPass, error)) {
		return false;
	}

	*swapchainCreateInfo.framebuffers = malloc(sizeof(**swapchainCreateInfo.framebuffers) * swapchainCreateInfo.swapchainInfo->imageCount);
	for (uint32_t i = 0; i < swapchainCreateInfo.swapchainInfo->imageCount; ++i) {
#ifdef DRAW_WINDOW_DECORATION
		VkImageView attachments[] = {*swapchainCreateInfo.offscreenImageView, (*swapchainCreateInfo.imageViews)[i]};
		uint32_t attachmentCount = 2;
#else
		VkImageView attachments[] = {(*swapchainCreateInfo.imageViews)[i]};
		uint32_t attachmentCount = 1;
#endif /* DRAW_WINDOW_DECORATION */

		if (!createFramebuffer(swapchainCreateInfo.device, *swapchainCreateInfo.swapchainInfo, attachments, attachmentCount, *swapchainCreateInfo.renderPass, *swapchainCreateInfo.framebuffers + i, error)) {
			return false;
		}
	}

	return true;
}

static void cleanupVulkan(VkInstance instance, VkDebugReportCallbackEXT debugCallback, VkSurfaceKHR surface, PhysicalDeviceCharacteristics *characteristics, PhysicalDeviceSurfaceCharacteristics *surfaceCharacteristics, VkDevice device, VmaAllocator allocator, VkSwapchainKHR swapchain, VkImage *offscreenImages, VmaAllocation *offscreenImageAllocations, size_t offscreenImageCount, VkImageView *offscreenImageViews, VkImageView *imageViews, uint32_t imageViewCount, VkRenderPass renderPass, VkPipelineLayout *pipelineLayouts, VkPipeline *pipelines, size_t pipelineCount, VkFramebuffer *framebuffers, uint32_t framebufferCount, VkCommandPool commandPool, VkCommandBuffer *commandBuffers, uint32_t commandBufferCount, SynchronizationInfo synchronizationInfo, VkDescriptorPool descriptorPool, VkDescriptorSet *imageDescriptorSets, VkDescriptorSetLayout *imageDescriptorSetLayouts, VkBuffer vertexBuffer, VmaAllocation vertexBufferAllocation, VkBuffer indexBuffer, VmaAllocation indexBufferAllocation)
{
#ifdef ENABLE_IMGUI
	cImGui_ImplVulkan_Shutdown();
#endif /* ENABLE_IMGUI */
	destroyBuffer(allocator, vertexBuffer, vertexBufferAllocation);
	destroyBuffer(allocator, indexBuffer, indexBufferAllocation);
	destroySynchronization(device, synchronizationInfo);
	freeCommandBuffers(device, commandPool, commandBuffers, commandBufferCount);
	destroyCommandPool(device, commandPool);
	for (size_t i = 0; i < pipelineCount; ++i) {
		destroyPipeline(device, pipelines[i]);
		destroyPipelineLayout(device, pipelineLayouts[i]);
	}
	destroyRenderPass(device, renderPass);
	destroyFramebuffers(device, framebuffers, framebufferCount);
#ifdef DRAW_WINDOW_DECORATION
	for (size_t i = 0; i < offscreenImageCount; ++i) {
		destroyDescriptorSetLayout(device, imageDescriptorSetLayouts[i]);
	}
	free(imageDescriptorSetLayouts);
	destroyDescriptorPool(device, descriptorPool);
	free(imageDescriptorSets);
	destroyImageViews(device, offscreenImageViews, offscreenImageCount);
	for (size_t i = 0; i < offscreenImageCount; ++i) {
		destroyImage(allocator, offscreenImages[i], offscreenImageAllocations[i]);
	}
#endif
	destroyImageViews(device, imageViews, imageViewCount);
	free(imageViews);
	destroySwapchain(device, swapchain);
	freePhysicalDeviceCharacteristics(characteristics);
	freePhysicalDeviceSurfaceCharacteristics(surfaceCharacteristics);
	destroySurface(instance, surface);
	destroyAllocator(allocator);
	destroyDevice(device);
	destroyInstance(instance, debugCallback);
}
