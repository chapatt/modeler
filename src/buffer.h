#ifndef MODELER_BUFFER_H
#define MODELER_BUFFER_H

#include <stdbool.h>
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

typedef struct vertex_t {
	float pos[2];
	float color[3];
	float texCoord[2];
} Vertex;

bool createBuffer(VkDevice device, VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryUsage, VkMemoryPropertyFlags memoryFlags, VkBuffer *buffer, VmaAllocation *allocation, char **error);
bool createMutableVertexBufferWithStaging(VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, void **stagingMappedMemory, VkBuffer *stagingBuffer, VmaAllocation *stagingAllocation, VkBuffer *buffer, VmaAllocation *allocation, const Vertex *vertices, size_t vertexCount, char **error);
bool updateMutableVertexBufferWithStaging(VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, void *stagingMappedMemory, VkBuffer *stagingBuffer, VkBuffer *buffer, const Vertex *vertices, size_t vertexCount, char **error);
bool createStaticVertexBuffer(VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, VkBuffer *buffer, VmaAllocation *allocation, const Vertex *vertices, size_t vertexCount, char **error);
bool createIndexBuffer(VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, VkBuffer *buffer, VmaAllocation *allocation, const uint16_t *indices, size_t indexCount, char **error);
bool copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, char **error);
void destroyBuffer(VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation);

#endif /* MODELER_BUFFER_H */
