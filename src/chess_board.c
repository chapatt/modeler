#include <stdlib.h>

#include "chess_board.h"
#include "pipeline.h"
#include "utils.h"

#define CHESS_SQUARE_COUNT 8 * 8
#define CHESS_VERTEX_COUNT CHESS_SQUARE_COUNT * 4
#define CHESS_INDEX_COUNT CHESS_SQUARE_COUNT * 6

struct chess_board_component_t {
	VkDevice device;
	VmaAllocator allocator;
	VkCommandPool commandPool;
	VkQueue queue;
	VkRenderPass renderPass;
	uint32_t subpass;
	VkExtent2D extent;
	const char *resourcePath;
	float aspectRatio;
	float width;
	float originX;
	float originY;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	VkBuffer vertexBuffer;
	VmaAllocation vertexBufferAllocation;
	VkBuffer indexBuffer;
	VmaAllocation indexBufferAllocation;
};

bool createChessBoardVertexBuffer(ChessBoard self, char **error);
bool createChessBoardPipeline(ChessBoard self, char **error);

bool createChessBoard(ChessBoard *chessBoard, VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, VkRenderPass renderPass, uint32_t subpass, VkExtent2D extent, const char *resourcePath, float aspectRatio, float width, float originX, float originY, char **error)
{
	*chessBoard = malloc(sizeof(**chessBoard));

	ChessBoard self = *chessBoard;

	self->device = device;
	self->allocator = allocator;
	self->commandPool = commandPool;
	self->queue = queue;
	self->renderPass = renderPass;
	self->subpass = subpass;
	self->extent = extent;
	self->resourcePath = resourcePath;
	self->aspectRatio = aspectRatio;
	self->width = width;
	self->originX = originX;
	self->originY = originY;

	if (!createChessBoardVertexBuffer(self, error)) {
		asprintf(error, "Failed to create chess board vertex buffer.\n");
		return false;
	}

	if (!createChessBoardPipeline(self, error)) {
		asprintf(error, "Failed to create chess board pipeline.\n");
		return false;
	}
}

bool createChessBoardVertexBuffer(ChessBoard self, char **error)
{
	const float black[3] = {0.0f, 0.0f, 0.0f};
	const float white[3] = {1.0f, 1.0f, 1.0f};
	Vertex triangleVertices[CHESS_VERTEX_COUNT];
	uint16_t triangleIndices[CHESS_INDEX_COUNT];
	for (size_t i = 0; i < 64; ++i) {
		size_t verticesOffset = i * 4;
		size_t indicesOffset = i * 6;
		size_t offsetX = i % 8;
		size_t offsetY = i / 8;
		float squareWidth = self->width / 8.0f;
		float squareHeight = squareWidth * self->aspectRatio;
		float squareOriginX = self->originX + offsetX * squareWidth;
		float squareOriginY = self->originY + offsetY * squareHeight;
		const float *color = (offsetY % 2) ?
			((offsetX % 2) ? black : white) :
			(offsetX % 2) ? white : black;

		triangleVertices[verticesOffset] = (Vertex) {{squareOriginX, squareOriginY}, {color[0], color[1], color[2]}};
		triangleVertices[verticesOffset + 1] = (Vertex) {{squareOriginX + squareWidth, squareOriginY}, {color[0], color[1], color[2]}};
		triangleVertices[verticesOffset + 2] = (Vertex) {{squareOriginX + squareWidth, squareOriginY + squareHeight}, {color[0], color[1], color[2]}};
		triangleVertices[verticesOffset + 3] = (Vertex) {{squareOriginX, squareOriginY + squareHeight}, {color[0], color[1], color[2]}};

		triangleIndices[indicesOffset] = verticesOffset + 0;
		triangleIndices[indicesOffset + 1] = verticesOffset + 1;
		triangleIndices[indicesOffset + 2] = verticesOffset + 2;
		triangleIndices[indicesOffset + 3] = verticesOffset + 2;
		triangleIndices[indicesOffset + 4] = verticesOffset + 3;
		triangleIndices[indicesOffset + 5] = verticesOffset + 0;
	}

	if (!createVertexBuffer(self->device, self->allocator, self->commandPool, self->queue, &self->vertexBuffer, &self->vertexBufferAllocation, triangleVertices, sizeof(triangleVertices) / sizeof(triangleVertices[0]), error)) {
		return false;
	}

	if (!createIndexBuffer(self->device, self->allocator, self->commandPool, self->queue, &self->indexBuffer, &self->indexBufferAllocation, triangleIndices, sizeof(triangleIndices) / sizeof(triangleIndices[0]), error)) {
		return false;
	}

	return true;
}

bool createChessBoardPipeline(ChessBoard self, char **error)
{
	VkVertexInputBindingDescription vertexBindingDescription = {
		.binding = 0,
		.stride = sizeof(Vertex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	};

	VkVertexInputAttributeDescription vertexAttributeDescriptions[] = {
		{
			.binding = 0,
			.location = 0,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = offsetof(Vertex, pos),
		}, {
			.binding = 0,
			.location = 1,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(Vertex, color),
		}
	};

#ifndef EMBED_SHADERS
	char *triangleVertShaderPath;
	char *triangleFragShaderPath;
	asprintf(&triangleVertShaderPath, "%s/%s", self->resourcePath, "triangle.vert.spv");
	asprintf(&triangleFragShaderPath, "%s/%s", self->resourcePath, "triangle.frag.spv");
	char *triangleVertShaderBytes;
	char *triangleFragShaderBytes;
	uint32_t triangleVertShaderSize = 0;
	uint32_t triangleFragShaderSize = 0;

	if ((triangleVertShaderSize = readFileToString(triangleVertShaderPath, &triangleVertShaderBytes)) == -1) {
		asprintf(error, "Failed to open triangle vertex shader for reading.\n");
		return false;
	}
	if ((triangleFragShaderSize = readFileToString(triangleFragShaderPath, &triangleFragShaderBytes)) == -1) {
		asprintf(error, "Failed to open triangle fragment shader for reading.\n");
		return false;
	}
#endif /* EMBED_SHADERS */

	PipelineCreateInfo pipelineCreateInfo = {
		.device = self->device,
		.renderPass = self->renderPass,
		.subpassIndex = self->subpass,
		.vertexShaderBytes = triangleVertShaderBytes,
		.vertexShaderSize = triangleVertShaderSize,
		.fragmentShaderBytes = triangleFragShaderBytes,
		.fragmentShaderSize = triangleFragShaderSize,
		.extent = self->extent,
		.vertexBindingDescriptionCount = 1,
		.vertexBindingDescriptions = &vertexBindingDescription,
		.vertexAttributeDescriptionCount = 2,
		.VertexAttributeDescriptions = vertexAttributeDescriptions,
		.descriptorSetLayouts = NULL,
		.descriptorSetLayoutCount = 0,
	};
	bool pipelineCreateSuccess = createPipeline(pipelineCreateInfo, &self->pipelineLayout, &self->pipeline, error);
#ifndef EMBED_SHADERS
	free(triangleFragShaderBytes);
	free(triangleVertShaderBytes);
#endif /* EMBED_SHADERS */
	if (!pipelineCreateSuccess) {
		return false;
	}

	return true;
}

bool updateChessBoard(ChessBoard self, float aspectRatio, float width, float originX, float originY, char **error)
{
	self->aspectRatio = aspectRatio;
	self->width = width;
	self->originX = originX;
	self->originY = originY;

	destroyBuffer(self->allocator, self->vertexBuffer, self->vertexBufferAllocation);
	destroyBuffer(self->allocator, self->indexBuffer, self->indexBufferAllocation);

	if (!createChessBoardVertexBuffer(self, error)) {
		return false;
	}

	return true;
}

bool drawChessBoard(ChessBoard self, VkCommandBuffer commandBuffer, WindowDimensions windowDimensions, char **error)
{
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &self->vertexBuffer, offsets);
	vkCmdBindIndexBuffer(commandBuffer, self->indexBuffer, 0, VK_INDEX_TYPE_UINT16);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, self->pipeline);
	VkViewport viewport = {
		.x = windowDimensions.activeArea.offset.x,
		.y = windowDimensions.activeArea.offset.y,
		.width = windowDimensions.activeArea.extent.width,
		.height = windowDimensions.activeArea.extent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};
	VkOffset2D scissorOffset = {
		.x = 0,
		.y = 0
	};
	VkRect2D scissor = {
		.offset = scissorOffset,
		.extent = windowDimensions.activeArea.extent
	};
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	PushConstants pushConstants = {
		.extent = {windowDimensions.activeArea.extent.width, windowDimensions.activeArea.extent.height},
		.offset = {windowDimensions.activeArea.offset.x, windowDimensions.activeArea.offset.y},
		.cornerRadius = windowDimensions.cornerRadius
	};
	vkCmdPushConstants(commandBuffer, self->pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);
	vkCmdDrawIndexed(commandBuffer, CHESS_INDEX_COUNT, 1, 0, 0, 0);

	return true;
}

void destroyChessBoard(ChessBoard self)
{
	destroyBuffer(self->allocator, self->vertexBuffer, self->vertexBufferAllocation);
	destroyBuffer(self->allocator, self->indexBuffer, self->indexBufferAllocation);
	destroyPipeline(self->device, self->pipeline);
	destroyPipelineLayout(self->device, self->pipelineLayout);
	free(self);
}
