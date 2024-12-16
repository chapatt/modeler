#ifndef MODELER_BUFFER_H
#define MODELER_BUFFER_H

#include <stdbool.h>
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

typedef struct vertex_t {
	int32_t pos[2];
	int32_t color[3];
} Vertex;

bool createBuffer(VkDevice device, VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryUsage, VkBuffer *buffer, VmaAllocation *allocation, char **error);
bool createVertexBuffer(VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, VkBuffer *buffer, VmaAllocation *allocation, const Vertex *vertices, size_t vertexCount, char **error);
bool createIndexBuffer(VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, VkBuffer *buffer, VmaAllocation *allocation, const uint16_t *indices, size_t indexCount, char **error);
void copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
void destroyBuffer(VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation);

#endif /* MODELER_BUFFER_H */