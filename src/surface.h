#ifndef MODELER_SURFACE_H
#define MODELER_SURFACE_H

#include <stdbool.h>
#include <vulkan/vulkan.h>

bool createSurface(VkInstance instance, void *platformWindow, VkSurfaceKHR *surface, char **error);
void destroySurface(VkInstance instance, VkSurfaceKHR surface);

#endif /* MODELER_SURFACE_H */