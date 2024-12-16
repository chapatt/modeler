#include "buffer.h"
#include "utils.h"
#include "vulkan_utils.h"

bool createBuffer(VkDevice device, VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryUsage, VkBuffer *buffer, VmaAllocation *allocation, char **error)
{
	VkResult result;

	VkBufferCreateInfo bufferCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = bufferUsage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};

	VmaAllocationCreateInfo allocationCreateInfo = {
		.usage = memoryUsage
	};

	if ((result = vmaCreateBuffer(allocator, &bufferCreateInfo, &allocationCreateInfo, buffer, allocation, NULL)) != VK_SUCCESS) {
		asprintf(error, "Failed to create buffer: %s", string_VkResult(result));
		return false;
	}

	return true;
}

bool createVertexBuffer(VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, VkBuffer *buffer, VmaAllocation *allocation, const Vertex *vertices, size_t vertexCount, char **error)
{
	VkDeviceSize bufferSize = sizeof(*vertices) * vertexCount;

	VkBuffer stagingBuffer;
	VmaAllocation stagingBufferAllocation;

	if (!createBuffer(device, allocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferAllocation, error)) {
		return false;
	}

	void *data;
	vmaMapMemory(allocator, stagingBufferAllocation, &data);
	memcpy(data, vertices, (size_t) bufferSize);
	vmaUnmapMemory(allocator, stagingBufferAllocation);

	if (!createBuffer(device, allocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, allocation, error)) {
		return false;
	}

	copyBuffer(device, commandPool, queue, stagingBuffer, *buffer, bufferSize);

	vmaDestroyBuffer(allocator, stagingBuffer, stagingBufferAllocation);

	return true;
}

bool createIndexBuffer(VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, VkBuffer *buffer, VmaAllocation *allocation, const uint16_t *indices, size_t indexCount, char **error)
{
	VkDeviceSize bufferSize = sizeof(*indices) * indexCount;

	VkBuffer stagingBuffer;
	VmaAllocation stagingBufferAllocation;

	if (!createBuffer(device, allocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferAllocation, error)) {
		return false;
	}

	void *data;
	vmaMapMemory(allocator, stagingBufferAllocation, &data);
	memcpy(data, indices, (size_t) bufferSize);
	vmaUnmapMemory(allocator, stagingBufferAllocation);

	if (!createBuffer(device, allocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, allocation, error)) {
		return false;
	}

	copyBuffer(device, commandPool, queue, stagingBuffer, *buffer, bufferSize);

	vmaDestroyBuffer(allocator, stagingBuffer, stagingBufferAllocation);

	return true;
}

void copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBufferAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandPool = commandPool,
		.commandBufferCount = 1,
	};

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferCopy copyRegion = {
		.srcOffset = 0,
		.dstOffset = 0,
		.size = size,
	};

	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer,
	};

	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void destroyBuffer(VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation)
{
	vmaDestroyBuffer(allocator, buffer, allocation);
}