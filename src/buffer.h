#ifndef MODELER_BUFFER_H
#define MODELER_BUFFER_H

#include <stdbool.h>
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

bool createBuffer(VkDevice device, VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryUsage, VkMemoryPropertyFlags memoryFlags, VkBuffer *buffer, VmaAllocation *allocation, char **error);
bool createMutableVertexBufferWithStaging(VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, void **stagingMappedMemory, VkBuffer *stagingBuffer, VmaAllocation *stagingAllocation, VkBuffer *buffer, VmaAllocation *allocation, const void *vertices, size_t vertexCount, size_t vertexSize, char **error);
bool updateMutableVertexBufferWithStaging(VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, void *stagingMappedMemory, VkBuffer *stagingBuffer, VkBuffer *buffer, const void *vertices, size_t vertexCount, size_t vertexSize, char **error);
bool createStaticVertexBuffer(VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, VkBuffer *buffer, VmaAllocation *allocation, const void *vertices, size_t vertexCount, size_t vertexSize, char **error);
bool copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, char **error);
void destroyBuffer(VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation);

#endif /* MODELER_BUFFER_H */
