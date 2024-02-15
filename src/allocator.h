#ifndef MODELER_ALLOCATOR_H
#define MODELER_ALLOCATOR_H

#include <stdbool.h>
#include <vulkan/vulkan.h>

#include "vk_mem_alloc.h"

bool createAllocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator *allocator, char **error);
void destroyAllocator(VmaAllocator allocator);

#endif /* MODELER_ALLOCATOR_H */