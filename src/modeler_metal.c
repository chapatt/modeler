#ifndef VK_USE_PLATFORM_METAL_EXT
#define VK_USE_PLATFORM_METAL_EXT
#endif

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_metal.h>

#include "modeler_metal.h"
#include "instance.h"
#include "surface_metal.h"
#include "physical_device.h"
#include "device.h"
#include "swapchain.h"

#include "renderloop.h"

bool initVulkanMetal(void *surfaceLayer, int width, int height, const char *resourcePath, char **error)
{
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
	VkQueue graphicsQueue;
	VkQueue presentationQueue;
	uint32_t graphicsQueueFamilyIndex;
	uint32_t presentationQueueFamilyIndex;
	if (!createDevice(physicalDevice, surface, characteristics, surfaceCharacteristics,
		&device, &graphicsQueue, &presentationQueue, &graphicsQueueFamilyIndex, &presentationQueueFamilyIndex, error))
	{
		return false;
	}

	VkSwapchainKHR swapchain;
	if (!createSwapchain(device, surface, surfaceCharacteristics, graphicsQueueFamilyIndex, presentationQueueFamilyIndex, windowExtent, &swapchain, error)) {
		return false;
	}

	draw(device, swapchain, windowExtent, graphicsQueue, presentationQueue, graphicsQueueFamilyIndex, resourcePath);

	return true;
}
