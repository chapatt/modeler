#include <stdio.h>
#include <stdlib.h>

#include "modeler_android.h"
#include "surface.h"
#include "utils.h"
#include "vulkan_utils.h"

bool createSurface(VkInstance instance, void *platformWindow, VkSurfaceKHR *surface, char **error)
{
	AndroidWindow *window = (AndroidWindow *) platformWindow;

	VkAndroidSurfaceCreateInfoKHR createInfo = {
		.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
		.window = window->nativeWindow
	};

	VkResult result;
	if ((result = vkCreateAndroidSurfaceKHR(instance, &createInfo, NULL, surface)) != VK_SUCCESS) {
		asprintf(error, "Failed to create surface: %s", string_VkResult(result));
		return false;
	}

	return surface;
}
