#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <stdio.h>
#include <vulkan/vulkan.h>

#include "surface_win32.h"
#include "utils.h"
#include "vulkan_utils.h"

bool createSurfaceWin32(VkInstance instance, HINSTANCE hinstance, HWND hwnd, VkSurfaceKHR *surface, char **error)
{
	VkWin32SurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = hwnd;
	createInfo.hinstance = hinstance;
    
	VkResult result;
	if ((result = vkCreateWin32SurfaceKHR(instance, &createInfo, NULL, surface)) != VK_SUCCESS) {
		asprintf(error, "Failed to create surface: %s", string_VkResult(result));
		return false;
	}
    
	return surface;
}