#include "surface.h"

void destroySurface(VkInstance instance, VkSurfaceKHR surface)
{
	vkDestroySurfaceKHR(instance, surface, NULL);
}