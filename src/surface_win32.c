#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <stdio.h>
#include <vulkan/vulkan.h>

#include "surface_win32.h"

VkSurfaceKHR createSurfaceWin32(VkInstance instance, HINSTANCE hinstance, HWND hwnd)
{
	VkWin32SurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = hwnd;
	createInfo.hinstance = hinstance;
    
	VkSurfaceKHR surface;
	VkResult result;
	if (vkCreateWin32SurfaceKHR(instance, &createInfo, NULL, &surface) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create surface!\n");
		exit(EXIT_FAILURE);
	}
    
	return surface;
}