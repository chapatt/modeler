#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <vulkan/vulkan.h>

#include "imgui/cimgui.h"
#include "imgui/cimgui_impl_vulkan.h"
#include "imgui/imgui_impl_modeler.h"

#include "modeler.h"
#include "instance.h"
#include "surface.h"
#include "physical_device.h"
#include "device.h"
#include "swapchain.h"
#include "image_view.h"
#include "render_pass.h"
#include "pipeline.h"
#include "framebuffer.h"
#include "utils.h"
#include "vulkan_utils.h"

#include "renderloop.h"

static void cleanupVulkan(VkInstance instance, VkSurfaceKHR surface, PhysicalDeviceCharacteristics *characteristics, PhysicalDeviceSurfaceCharacteristics *surfaceCharacteristics, VkDevice device, VkSwapchainKHR swapchain, VkImageView *imageViews, uint32_t imageViewCount, VkRenderPass renderPass, VkPipelineLayout pipelineLayout, VkPipeline pipeline, VkFramebuffer *framebuffers, uint32_t framebufferCount);
static void imVkCheck(VkResult result);

void terminateVulkan(Queue *inputQueue, pthread_t thread)
{
	enqueueInputEvent(inputQueue, TERMINATE, NULL);
	pthread_join(thread, NULL);
}

static void imVkCheck(VkResult result)
{
	if (result != VK_SUCCESS) {
		printf("IMGUI Vulkan impl failure: %s", string_VkResult(result));
	}
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
	if (!createInstance(instanceExtensions, instanceExtensionCount, &instance, error)) {
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
	if (!createDevice(physicalDevice, surface, characteristics, surfaceCharacteristics, &device, &queueInfo, error)) {
		sendThreadFailureSignal(platformWindow);
	}

	SwapchainInfo swapchainInfo = {};
	if (!createSwapchain(device, surface, surfaceCharacteristics, queueInfo.graphicsQueueFamilyIndex, queueInfo.presentationQueueFamilyIndex, initialExtent, &swapchainInfo, error)) {
		sendThreadFailureSignal(platformWindow);
	}

	VkImageView *imageViews;
	if (!createImageViews(device, swapchainInfo, &imageViews, error)) {
		sendThreadFailureSignal(platformWindow);
	}

	VkRenderPass renderPass;
	if (!createRenderPass(device, swapchainInfo, &renderPass, error)) {
		sendThreadFailureSignal(platformWindow);
	}

	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	if (!createPipeline(device, renderPass, resourcePath, swapchainInfo, &pipelineLayout, &pipeline, error)) {
		sendThreadFailureSignal(platformWindow);
	}

	VkFramebuffer *framebuffers;
	if (!createFramebuffers(device, swapchainInfo, imageViews, renderPass, &framebuffers, error)) {
		sendThreadFailureSignal(platformWindow);
	}

	VkDescriptorPool imDescriptorPool;
	VkDescriptorPoolSize pool_sizes[] =
	{
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
	};
	VkDescriptorPoolCreateInfo pool_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		.maxSets = 1,
		.poolSizeCount = (uint32_t) IM_ARRAYSIZE(pool_sizes),
		.pPoolSizes = pool_sizes
	};
	VkResult result;
	if ((result = vkCreateDescriptorPool(device, &pool_info, NULL, &imDescriptorPool)) != VK_SUCCESS) {
		asprintf(error, "Failed to create descriptor pool: %s", string_VkResult(result));
		sendThreadFailureSignal(platformWindow);
	}
	ImGui_CreateContext(NULL);
	ImGuiIO *io = ImGui_GetIO();
	io->IniFilename = NULL;
	ImGui_ImplModeler_Init(swapchainInfo.extent);
	ImGui_StyleColorsDark(NULL);
	ImGui_ImplVulkan_InitInfo imVulkanInitInfo = {
		.Instance = instance,
		.PhysicalDevice = physicalDevice,
		.Device = device,
		.QueueFamily = queueInfo.graphicsQueueFamilyIndex,
		.Queue = queueInfo.graphicsQueue,
		.PipelineCache = VK_NULL_HANDLE,
		.DescriptorPool = imDescriptorPool,
		.Subpass = 0,
		.MinImageCount = surfaceCharacteristics.capabilities.minImageCount,
		.ImageCount = swapchainInfo.imageCount,
		.MSAASamples = VK_SAMPLE_COUNT_1_BIT,
		.Allocator = NULL,
		.CheckVkResultFn = imVkCheck
	};

	draw(device, renderPass, pipeline, framebuffers, swapchainInfo, imageViews, swapchainInfo.imageCount, queueInfo.graphicsQueue, queueInfo.presentationQueue, queueInfo.graphicsQueueFamilyIndex, ".", inputQueue, imVulkanInitInfo);

	cleanupVulkan(instance, surface, &characteristics, &surfaceCharacteristics, device, swapchainInfo.swapchain, imageViews, swapchainInfo.imageCount, renderPass, pipelineLayout, pipeline, framebuffers, swapchainInfo.imageCount);

	return NULL;
}

static void cleanupVulkan(VkInstance instance, VkSurfaceKHR surface, PhysicalDeviceCharacteristics *characteristics, PhysicalDeviceSurfaceCharacteristics *surfaceCharacteristics, VkDevice device, VkSwapchainKHR swapchain, VkImageView *imageViews, uint32_t imageViewCount, VkRenderPass renderPass, VkPipelineLayout pipelineLayout, VkPipeline pipeline, VkFramebuffer *framebuffers, uint32_t framebufferCount)
{
	destroyFramebuffers(device, framebuffers, framebufferCount);
	destroyPipeline(device, pipeline);
	destroyPipelineLayout(device, pipelineLayout);
	destroyRenderPass(device, renderPass);
	destroyImageViews(device, imageViews, imageViewCount);
	destroySwapchain(device, swapchain);
	destroyDevice(device);
	freePhysicalDeviceCharacteristics(characteristics);
	freePhysicalDeviceSurfaceCharacteristics(surfaceCharacteristics);
	destroySurface(instance, surface);
	destroyInstance(instance);
}