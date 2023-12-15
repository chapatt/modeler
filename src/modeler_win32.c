#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <vulkan/vulkan.h>

#include "imgui/cimgui.h"
#include "imgui/cimgui_impl_vulkan.h"
#include "imgui/imgui_impl_modeler.h"

#include "modeler_win32.h"
#include "instance.h"
#include "surface.h"
#include "surface_win32.h"
#include "physical_device.h"
#include "device.h"
#include "swapchain.h"
#include "image_view.h"
#include "utils.h"
#include "vulkan_utils.h"

#include "renderloop.h"

struct threadArguments {
	HINSTANCE hinstance;
	HWND hwnd;
	Queue *inputQueue;
	char **error;
};

static void *threadProc(void *arg);
static void imVkCheck(VkResult result);
static void sendThreadFailureSignal(HWND hwnd);
static void imVkCheck(VkResult result);

pthread_t initVulkanWin32(HINSTANCE hinstance, HWND hwnd, Queue *inputQueue, char **error)
{
	pthread_t thread;
	struct threadArguments *threadArgs = malloc(sizeof(*threadArgs));
	threadArgs->hinstance = hinstance;
	threadArgs->hwnd = hwnd;
	threadArgs->inputQueue = inputQueue;
	threadArgs->error = error;

	if (pthread_create(&thread, NULL, threadProc, (void *) threadArgs) != 0) {
		free(threadArgs);
		asprintf(error, "Failed to start Vulkan thread");
		return 0;
	}

	return thread;
}

void terminateVulkanWin32(Queue *inputQueue, pthread_t thread)
{
	enqueueInputEvent(inputQueue, TERMINATE);
	pthread_join(thread, NULL);
}

static void imVkCheck(VkResult result)
{
	if (result != VK_SUCCESS) {
		printf("IMGUI Vulkan impl failure: %s", string_VkResult(result));
	}
}

static void *threadProc(void *arg)
{
	struct threadArguments *threadArgs = (struct threadArguments *) arg;
	HINSTANCE hinstance = threadArgs->hinstance;
	HWND hwnd = threadArgs->hwnd;
	Queue *inputQueue = threadArgs->inputQueue;
	char **error = threadArgs->error;

	RECT rect;
	if (!GetClientRect(hwnd, &rect)) {
		asprintf(error, "Failed to get window extent");
		sendThreadFailureSignal(hwnd);
	}
	VkExtent2D windowExtent = {
		.width = rect.right - rect.left,
		.height = rect.bottom - rect.top
	};

	const char *instanceExtensions[] = {
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME
	};
	VkInstance instance;
	if (!createInstance(instanceExtensions, 3, &instance, error)) {
		sendThreadFailureSignal(hwnd);
	}

	VkSurfaceKHR surface;
	if (!createSurfaceWin32(instance, hinstance, hwnd, &surface, error)) {
		sendThreadFailureSignal(hwnd);
	}

	VkPhysicalDevice physicalDevice;
	PhysicalDeviceCharacteristics characteristics;
	PhysicalDeviceSurfaceCharacteristics surfaceCharacteristics;
	if (!choosePhysicalDevice(instance, surface, &physicalDevice, &characteristics, &surfaceCharacteristics, error)) {
		sendThreadFailureSignal(hwnd);
	}

	VkDevice device;
	QueueInfo queueInfo = {};
	if (!createDevice(physicalDevice, surface, characteristics, surfaceCharacteristics, &device, &queueInfo, error)) {
		sendThreadFailureSignal(hwnd);
	}

	SwapchainInfo swapchainInfo = {};
	if (!createSwapchain(device, surface, surfaceCharacteristics, queueInfo.graphicsQueueFamilyIndex, queueInfo.presentationQueueFamilyIndex, windowExtent, &swapchainInfo, error)) {
		sendThreadFailureSignal(hwnd);
	}

	VkImageView *imageViews;
	if (!createImageViews(device, &swapchainInfo, &imageViews, error)) {
		sendThreadFailureSignal(hwnd);
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
		sendThreadFailureSignal(hwnd);
	}
	ImGui_CreateContext(NULL);
	ImGuiIO *io = ImGui_GetIO();
	io->IniFilename = NULL;
	ImGui_ImplModeler_Init(windowExtent);
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

	draw(device, swapchainInfo.swapchain, imageViews, swapchainInfo.imageCount, windowExtent, queueInfo.graphicsQueue, queueInfo.presentationQueue, queueInfo.graphicsQueueFamilyIndex, ".", inputQueue, imVulkanInitInfo);

	return NULL;
}

static void sendThreadFailureSignal(HWND hwnd) {
	PostMessageW(hwnd, THREAD_FAILURE_NOTIFICATION_MESSAGE, 0, 0);
	pthread_exit(NULL);
}

void cleanupVulkan(VkInstance instance, VkSurfaceKHR surface, VkDevice device, VkSwapchainKHR swapchain, VkImageView *imageViews, uint32_t imageViewCount, PhysicalDeviceCharacteristics *characteristics, PhysicalDeviceSurfaceCharacteristics *surfaceCharacteristics)
{
	destroyImageViews(device, imageViews, imageViewCount);
	destroySwapchain(device, swapchain);
	destroyDevice(device);
	freePhysicalDeviceCharacteristics(characteristics);
	freePhysicalDeviceSurfaceCharacteristics(surfaceCharacteristics);
	destroySurface(instance, surface);
	destroyInstance(instance);
}