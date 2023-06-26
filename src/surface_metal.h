#ifndef MODELER_SURFACE_METAL_H
#define MODELER_SURFACE_METAL_H

#include <stdbool.h>
#include <vulkan/vulkan.h>

bool createSurfaceMetal(VkInstance instance, const CAMetalLayer *layer, VkSurfaceKHR *surface, char **error);

#endif /* MODELER_SURFACE_METAL_H */