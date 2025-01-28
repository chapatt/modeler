#ifndef MODELER_BUFFER_H
#define MODELER_BUFFER_H

#include <stdbool.h>
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

bool createBuffer(VkDevice device, VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryUsage, VkMemoryPropertyFlags memoryFlags, VkBuffer *buffer, VmaAllocation *allocation, char **error);
bool createMutableBufferWithStaging(VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, VkBufferUsageFlagBits usage, void **stagingMappedMemory, VkBuffer *stagingBuffer, VmaAllocation *stagingAllocation, VkBuffer *buffer, VmaAllocation *allocation, const void *vertices, size_t vertexCount, size_t vertexSize, char **error);
bool updateMutableBufferWithStaging(VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, void *stagingMappedMemory, VkBuffer *stagingBuffer, VkBuffer *buffer, const void *vertices, size_t vertexCount, size_t vertexSize, char **error);
bool createStaticBuffer(VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, VkBufferUsageFlagBits usage, VkBuffer *buffer, VmaAllocation *allocation, const void *vertices, size_t vertexCount, size_t vertexSize, char **error);
bool copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, char **error);
void destroyBuffer(VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation);

#endif /* MODELER_BUFFER_H */
