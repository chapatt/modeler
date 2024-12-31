#ifndef MODELER_IMAGE_H
#define MODELER_IMAGE_H

#include <stdbool.h>
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

bool createImage(VkDevice device, VmaAllocator allocator, VkExtent2D extent, VkFormat format, VkImageUsageFlagBits usage, uint32_t mipLevels, VkImage *image, VmaAllocation *allocation, char **error);
bool transitionImageLayout(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, char **error);
bool copyBufferToImage(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t mipLevels, char **error);
void destroyImage(VmaAllocator allocator, VkImage image, VmaAllocation allocation);

#endif /* MODELER_IMAGE_H */
