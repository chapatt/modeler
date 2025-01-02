#include <vulkan/vulkan.h>

#include "modeler_win32.h"
#include "surface.h"
#include "utils.h"
#include "vulkan_utils.h"

bool createSurface(VkInstance instance, void *platformWindow, VkSurfaceKHR *surface, char **error)
{
	Win32Window *window = (Win32Window *) platformWindow;

	VkWin32SurfaceCreateInfoKHR createInfo = {
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hwnd = window->hWnd,
		.hinstance = window->hInstance
	};
    
	VkResult result;
	if ((result = vkCreateWin32SurfaceKHR(instance, &createInfo, NULL, surface)) != VK_SUCCESS) {
		asprintf(error, "Failed to create surface: %s", string_VkResult(result));
		return false;
	}
    
	return surface;
}