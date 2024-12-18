#include <string.h>

#include "buffer.h"
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

bool createVertexBuffer(VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, VkBuffer *buffer, VmaAllocation *allocation, const Vertex *vertices, size_t vertexCount, char **error)
{
	VkResult result;

	VkDeviceSize bufferSize = sizeof(*vertices) * vertexCount;

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
	memcpy(data, vertices, (size_t) bufferSize);
	vmaUnmapMemory(allocator, stagingBufferAllocation);

	if (!createBuffer(device, allocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, allocation, error)) {
		return false;
	}

	if (!copyBuffer(device, commandPool, queue, stagingBuffer, *buffer, bufferSize, error)) {
		return false;
	}

	vmaDestroyBuffer(allocator, stagingBuffer, stagingBufferAllocation);

	return true;
}

bool createIndexBuffer(VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, VkBuffer *buffer, VmaAllocation *allocation, const uint16_t *indices, size_t indexCount, char **error)
{
	VkResult result;

	VkDeviceSize bufferSize = sizeof(*indices) * indexCount;

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
	memcpy(data, indices, (size_t) bufferSize);
	vmaUnmapMemory(allocator, stagingBufferAllocation);

	if (!createBuffer(device, allocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, allocation, error)) {
		return false;
	}

	if (!copyBuffer(device, commandPool, queue, stagingBuffer, *buffer, bufferSize, error)) {
		return false;
	}

	vmaDestroyBuffer(allocator, stagingBuffer, stagingBufferAllocation);

	return true;
}

bool copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, char **error)
{
	VkResult result;

	VkCommandBufferAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandPool = commandPool,
		.commandBufferCount = 1,
	};

	VkCommandBuffer commandBuffer;
	if ((result = vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer)) != VK_SUCCESS) {
		asprintf(error, "Failed to allocate command buffers: %s", string_VkResult(result));
		return false;
	}

	VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};

	if ((result = vkBeginCommandBuffer(commandBuffer, &beginInfo)) != VK_SUCCESS) {
		asprintf(error, "Failed to begin command buffer: %s", string_VkResult(result));
		return false;
	}

	VkBufferCopy copyRegion = {
		.srcOffset = 0,
		.dstOffset = 0,
		.size = size,
	};

	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	if ((result = vkEndCommandBuffer(commandBuffer)) != VK_SUCCESS) {
		asprintf(error, "Failed to end command buffer: %s", string_VkResult(result));
		return false;
	}

	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer,
	};

	if ((result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE)) != VK_SUCCESS) {
		asprintf(error, "Failed to submit queue: %s", string_VkResult(result));
		return false;
	}
	if ((result = vkQueueWaitIdle(queue)) != VK_SUCCESS) {
		asprintf(error, "Failed to wait for queue to be idle: %s", string_VkResult(result));
		return false;
	}

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);

	return true;
}

void destroyBuffer(VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation)
{
	vmaDestroyBuffer(allocator, buffer, allocation);
}
