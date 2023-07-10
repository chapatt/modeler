#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <windows.h>

#include "modeler_win32.h"
#include "instance.h"
#include "surface_win32.h"
#include "physical_device.h"
#include "device.h"
#include "swapchain.h"
#include "utils.h"

#include "renderloop.h"

bool initVulkanWin32(HINSTANCE hinstance, HWND hwnd, char **error)
{
	RECT rect;
	if (!GetClientRect(hwnd, &rect)) {
		asprintf(error, "Failed to get window extent");
		return false;
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
		return false;
	}

	VkSurfaceKHR surface;
	if (!createSurfaceWin32(instance, hinstance, hwnd, &surface, error)) {
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

	draw(device, swapchain, windowExtent, graphicsQueue, presentationQueue, graphicsQueueFamilyIndex);

	return true;
}