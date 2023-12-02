#ifndef VK_USE_PLATFORM_METAL_EXT
#define VK_USE_PLATFORM_METAL_EXT
#endif

#include <stdlib.h>
#include <pthread.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_metal.h>

#include "modeler_metal.h"
#include "instance.h"
#include "surface.h"
#include "surface_metal.h"
#include "physical_device.h"
#include "device.h"
#include "swapchain.h"
#include "utils.h"

#include "renderloop.h"

struct threadArguments {
	void *surfaceLayer;
	int width;
	int height;
	char *resourcePath;
	Queue *inputQueue;
	char **error;
};

void *threadProc(void *arg);

bool initVulkanMetal(void *surfaceLayer, int width, int height, const char *resourcePath, Queue *inputQueue, char **error)
{
	pthread_t thread;
	struct threadArguments *threadArgs = (struct threadArguments *) malloc(sizeof(struct threadArguments));
	threadArgs->surfaceLayer = surfaceLayer;
	threadArgs->width = width;
	threadArgs->height = height;
	asprintf(&threadArgs->resourcePath, "%s", resourcePath);
	threadArgs->inputQueue = inputQueue;
	threadArgs->error = error;

	if (pthread_create(&thread, NULL, threadProc, (void *) threadArgs) != 0) {
		free(threadArgs->resourcePath);
		free(threadArgs);
		return false;
	}

	return true;
}

void *threadProc(void *arg)
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
		return false;
	}

	VkSurfaceKHR surface;
	if (!createSurfaceMetal(instance, surfaceLayer, &surface, error)) {
		return false;
	}

	VkPhysicalDevice physicalDevice;
	PhysicalDeviceCharacteristics characteristics;
	PhysicalDeviceSurfaceCharacteristics surfaceCharacteristics;
	if (!choosePhysicalDevice(instance, surface, &physicalDevice, &characteristics, &surfaceCharacteristics, error)) {
		return false;
	}

	VkDevice device;
	QueueInfo queueInfo = {};
	if (!createDevice(physicalDevice, surface, characteristics, surfaceCharacteristics, &device, &queueInfo, error)) {
		return false;
	}

	SwapchainInfo swapchainInfo = {};
	if (!createSwapchain(device, surface, surfaceCharacteristics, queueInfo.graphicsQueueFamilyIndex, queueInfo.presentationQueueFamilyIndex, windowExtent, &swapchainInfo, error)) {
		return false;
	}

	draw(device, swapchainInfo.swapchain, windowExtent, queueInfo.graphicsQueue, queueInfo.presentationQueue, queueInfo.graphicsQueueFamilyIndex, resourcePath, inputQueue);

	return true;
}
