#ifndef MODELER_IMAGE_H
#define MODELER_IMAGE_H

#include <stdbool.h>
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

bool createImage(VkDevice device, VmaAllocator allocator, VkExtent2D extent, VkFormat format, VkImage *image, VmaAllocation *allocation, char **error);
void destroyImage(VmaAllocator allocator, VkImage image, VmaAllocation allocation);

#endif /* MODELER_IMAGE_H */