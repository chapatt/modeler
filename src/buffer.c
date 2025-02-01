#include <string.h>

#include "buffer.h"

#include "command_buffer.h"
#include "utils.h"
#include "vulkan_utils.h"

bool createBuffer(VkDevice device, VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryUsage, VkMemoryPropertyFlags memoryFlags, VkBuffer *buffer, VmaAllocation *allocation, char **error)
{
	VkResult result;

	VkBufferCreateInfo bufferCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = bufferUsage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};

	VmaAllocationCreateInfo allocationCreateInfo = {
		.usage = memoryUsage,
		.flags = memoryFlags,
	};

	if ((result = vmaCreateBuffer(allocator, &bufferCreateInfo, &allocationCreateInfo, buffer, allocation, NULL)) != VK_SUCCESS) {
		asprintf(error, "Failed to create buffer: %s", string_VkResult(result));
		return false;
	}

	return true;
}

bool createMutableBufferWithStaging(VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, VkBufferUsageFlagBits usage, void **stagingMappedMemory, VkBuffer *stagingBuffer, VmaAllocation *stagingAllocation, VkBuffer *buffer, VmaAllocation *allocation, const void *vertices, size_t vertexCount, size_t vertexSize, char **error)
{
	VkResult result;

	VkDeviceSize bufferSize = vertexSize * vertexCount;

	if (!createBuffer(device, allocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT, stagingBuffer, stagingAllocation, error)) {
		return false;
	}

	if ((result = vmaMapMemory(allocator, *stagingAllocation, stagingMappedMemory)) != VK_SUCCESS) {
		asprintf(error, "Failed to map memory: %s", string_VkResult(result));
		return false;
	}
	memcpy(*stagingMappedMemory, vertices, bufferSize);

	if (!createBuffer(device, allocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, VMA_MEMORY_USAGE_AUTO, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, allocation, error)) {
		return false;
	}

	if (!copyBuffer(device, commandPool, queue, *stagingBuffer, *buffer, bufferSize, error)) {
		return false;
	}

	return true;
}

bool updateMutableBufferWithStaging(VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, void *stagingMappedMemory, VkBuffer *stagingBuffer, VkBuffer *buffer, const void *vertices, size_t vertexCount, size_t vertexSize, char **error)
{
	VkDeviceSize bufferSize = vertexSize * vertexCount;

	memcpy(stagingMappedMemory, vertices, bufferSize);

	if (!copyBuffer(device, commandPool, queue, *stagingBuffer, *buffer, bufferSize, error)) {
		return false;
	}

	return true;
}

bool createStaticBuffer(VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, VkBufferUsageFlagBits usage, VkBuffer *buffer, VmaAllocation *allocation, const void *vertices, size_t vertexCount, size_t vertexSize, char **error)
{
	VkResult result;

	VkDeviceSize bufferSize = vertexSize * vertexCount;

	VkBuffer stagingBuffer;
	VmaAllocation stagingBufferAllocation;

	if (!createBuffer(device, allocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT, &stagingBuffer, &stagingBufferAllocation, error)) {
		return false;
	}

	void *data;
	if ((result = vmaMapMemory(allocator, stagingBufferAllocation, &data)) != VK_SUCCESS) {
		asprintf(error, "Failed to map memory: %s", string_VkResult(result));
		return false;
	}
	memcpy(data, vertices, bufferSize);
	vmaUnmapMemory(allocator, stagingBufferAllocation);

	if (!createBuffer(device, allocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, VMA_MEMORY_USAGE_AUTO, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, allocation, error)) {
		return false;
	}

	if (!copyBuffer(device, commandPool, queue, stagingBuffer, *buffer, bufferSize, error)) {
		return false;
	}

	vmaDestroyBuffer(allocator, stagingBuffer, stagingBufferAllocation);

	return true;
}

bool createHostVisibleMutableBuffer(VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, VkBufferUsageFlagBits usage, void **mappedMemory, VkBuffer *buffer, VmaAllocation *allocation, const void *vertices, size_t vertexCount, size_t vertexSize, char **error)
{
	VkResult result;

	VkDeviceSize bufferSize = vertexSize * vertexCount;

	if (!createBuffer(device, allocator, bufferSize, usage, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT, buffer, allocation, error)) {
		return false;
	}

	if ((result = vmaMapMemory(allocator, *allocation, mappedMemory)) != VK_SUCCESS) {
		asprintf(error, "Failed to map memory: %s", string_VkResult(result));
		return false;
	}
	memcpy(*mappedMemory, vertices, bufferSize);

	return true;
}

bool updateHostVisibleMutableBuffer(VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, void *mappedMemory, VkBuffer *buffer, const void *vertices, size_t vertexCount, size_t vertexSize, char **error)
{
	VkDeviceSize bufferSize = vertexSize * vertexCount;

	memcpy(mappedMemory, vertices, bufferSize);

	return true;
}

bool copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, char **error)
{
	VkResult result;

	VkCommandBuffer commandBuffer;
	if (!beginSingleTimeCommands(device, commandPool, &commandBuffer, error)) {
		return false;
	}

	VkBufferCopy copyRegion = {
		.srcOffset = 0,
		.dstOffset = 0,
		.size = size,
	};

	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	if (!endSingleTimeCommands(device, commandPool, queue, commandBuffer, error)) {
		return false;
	}

	return true;
}

void destroyBuffer(VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation)
{
	vmaDestroyBuffer(allocator, buffer, allocation);
}
