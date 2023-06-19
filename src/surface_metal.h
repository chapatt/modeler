#ifndef MODELER_SURFACE_METAL_H
#define MODELER_SURFACE_METAL_H

#include <vulkan/vulkan.h>

VkSurfaceKHR createSurfaceMetal(VkInstance instance, const CAMetalLayer *layer);

#endif /* MODELER_SURFACE_METAL_H */