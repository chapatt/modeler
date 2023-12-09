#ifndef VK_USE_PLATFORM_METAL_EXT
#define VK_USE_PLATFORM_METAL_EXT
#endif

#include "objc/message.h"
#include "objc/runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_metal.h>

#include "imgui/cimgui.h"
#include "imgui/cimgui_impl_vulkan.h"
#include "imgui/imgui_impl_modeler.h"

#include "modeler_metal.h"
#include "instance.h"
#include "surface.h"
#include "surface_metal.h"
#include "physical_device.h"
#include "device.h"
#include "swapchain.h"
#include "image_view.h"
#include "utils.h"
#include "vulkan_utils.h"

#include "renderloop.h"

struct threadArguments {
	void *surfaceLayer;
	int width;
	int height;
	char *resourcePath;
	Queue *inputQueue;
	char **error;
};

static void *threadProc(void *arg);
static void sendThreadFailureSignal(void);
static void sendNSNotification(char *message);

bool initVulkanMetal(void *surfaceLayer, int width, int height, const char *resourcePath, Queue *inputQueue, char **error)
{
	pthread_t thread;
	struct threadArguments *threadArgs = malloc(sizeof(*threadArgs));
	threadArgs->surfaceLayer = surfaceLayer;
	threadArgs->width = width;
	threadArgs->height = height;
	asprintf(&threadArgs->resourcePath, "%s", resourcePath);
	threadArgs->inputQueue = inputQueue;
	threadArgs->error = error;

	if (pthread_create(&thread, NULL, threadProc, (void *) threadArgs) != 0) {
		free(threadArgs->resourcePath);
		free(threadArgs);
		asprintf(error, "Failed to start Vulkan thread");
		return false;
	}

	return true;
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
	void *surfaceLayer = threadArgs->surfaceLayer;
	int width = threadArgs->width;
	int height = threadArgs->height;
	char *resourcePath = threadArgs->resourcePath;
	Queue *inputQueue = threadArgs->inputQueue;
	char **error = threadArgs->error;

	VkExtent2D windowExtent = {
		.width = width,
		.height = height
	};

	const char *instanceExtensions[] = {
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_EXT_METAL_SURFACE_EXTENSION_NAME
	};
	VkInstance instance;
	if (!createInstance(instanceExtensions, 3, &instance, error)) {
		sendThreadFailureSignal();
	}

	VkSurfaceKHR surface;
	if (!createSurfaceMetal(instance, surfaceLayer, &surface, error)) {
		sendThreadFailureSignal();
	}

	VkPhysicalDevice physicalDevice;
	PhysicalDeviceCharacteristics characteristics;
	PhysicalDeviceSurfaceCharacteristics surfaceCharacteristics;
	if (!choosePhysicalDevice(instance, surface, &physicalDevice, &characteristics, &surfaceCharacteristics, error)) {
		sendThreadFailureSignal();
	}

	VkDevice device;
	QueueInfo queueInfo = {};
	if (!createDevice(physicalDevice, surface, characteristics, surfaceCharacteristics, &device, &queueInfo, error)) {
		sendThreadFailureSignal();
	}

	SwapchainInfo swapchainInfo = {};
	if (!createSwapchain(device, surface, surfaceCharacteristics, queueInfo.graphicsQueueFamilyIndex, queueInfo.presentationQueueFamilyIndex, windowExtent, &swapchainInfo, error)) {
		sendThreadFailureSignal();
	}

	VkImageView *imageViews;
	if (!createImageViews(device, &swapchainInfo, &imageViews, error)) {
		sendThreadFailureSignal();
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
		sendThreadFailureSignal();
	}
	ImGui_CreateContext(NULL);
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

	draw(device, swapchainInfo.swapchain, imageViews, swapchainInfo.imageCount, windowExtent, queueInfo.graphicsQueue, queueInfo.presentationQueue, queueInfo.graphicsQueueFamilyIndex, resourcePath, inputQueue, imVulkanInitInfo);

	return NULL;
}

static void sendThreadFailureSignal(void)
{
	sendNSNotification(FAILURE_NOTIFICATION_NAME);
}

static void sendNSNotification(char *message)
{
	id (*postNotification)(id, SEL, id) = (id (*)(id, SEL, id)) objc_msgSend;
	id (*notificationWithNameObject)(Class, SEL, id, id) = (id (*)(Class, SEL, id, id)) objc_msgSend;
	id (*stringWithUTF8String)(Class, SEL, char *) = (id (*)(Class, SEL, char *)) objc_msgSend;
	id (*defaultCenter)(Class, SEL) = (id (*)(Class, SEL)) objc_msgSend;

	Class NSStringClass = objc_getClass("NSString");
	SEL stringWithUTF8StringSelector = sel_registerName("stringWithUTF8String:");
	id name = stringWithUTF8String(NSStringClass, stringWithUTF8StringSelector, message);

	Class NSNotificationClass = objc_getClass("NSNotification");
	SEL notifcationWithNameObjectSelector = sel_registerName("notificationWithName:object:");
	id notification = notificationWithNameObject(NSNotificationClass, notifcationWithNameObjectSelector, name, NULL);

	Class NSNotificationCenterClass = objc_getClass("NSNotificationCenter");
	SEL defaultCenterSelector = sel_registerName("defaultCenter");
	id notificationCenter = defaultCenter(NSNotificationCenterClass, defaultCenterSelector);

	SEL postNotificationSelector = sel_registerName("postNotification:");
	postNotification(notificationCenter, postNotificationSelector, notification);

	pthread_exit(NULL);
}
