#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "chess_board.h"
#include "descriptor.h"
#include "image.h"
#include "image_view.h"
#include "pipeline.h"
#include "sampler.h"
#include "utils.h"

#ifdef EMBED_SHADERS
#include "../shader_chess_board.vert.h"
#include "../shader_chess_board.frag.h"
#endif /* EMBED_SHADERS */

#define CHESS_VERTEX_COUNT CHESS_SQUARE_COUNT * 4
#define CHESS_INDEX_COUNT CHESS_SQUARE_COUNT * 6
#define TEXTURE_WIDTH 2048
#define TEXTURE_HEIGHT 2048

float pieceSpriteOriginMap[13][2] = {
	{0.75f, 0.75f},
	{0.25f, 0.75f},
	{0.75f, 0.5f},
	{0.5f, 0.5f},
	{0.0f, 0.75f},
	{0.25f, 0.5f},
	{0.0f, 0.5f},
	{0.25f, 0.25f},
	{0.75f, 0.0f},
	{0.5f, 0.0f},
	{0.0f, 0.25f},
	{0.25f, 0.0f},
	{0.0f, 0.0f}
};

float iconSpriteOriginMap[3][2] = {
	{0.75f, 0.75f},
	{0.5f, 0.25f},
	{0.75f, 0.25f}
};

struct chess_board_t {
	VkDevice device;
	VmaAllocator allocator;
	VkCommandPool commandPool;
	VkQueue queue;
	VkRenderPass renderPass;
	uint32_t subpass;
	const char *resourcePath;
	float anisotropy;
	float aspectRatio;
	float width;
	float originX;
	float originY;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	Vertex vertices[CHESS_VERTEX_COUNT];
	void *stagingVertexBufferMappedMemory;
	VkBuffer stagingVertexBuffer;
	VmaAllocation stagingVertexBufferAllocation;
	VkBuffer vertexBuffer;
	VmaAllocation vertexBufferAllocation;
	VkBuffer indexBuffer;
	VmaAllocation indexBufferAllocation;
	VkImage textureImage;
	VmaAllocation textureImageAllocation;
	VkImageView textureImageView;
	VkSampler sampler;
	VkDescriptorPool textureDescriptorPool;
	VkDescriptorSet *textureDescriptorSets;
	VkDescriptorSetLayout *textureDescriptorSetLayouts;
	Board8x8 board;
	MoveBoard8x8 move;
};

static void initializePieces(ChessBoard self);
static void initializeMove(ChessBoard self);
static bool createChessBoardTexture(ChessBoard self, char **error);
static bool createChessBoardSampler(ChessBoard self, char **error);
static bool createChessBoardDescriptors(ChessBoard self, char **error);
static bool createChessBoardVertexBuffer(ChessBoard self, char **error);
static bool createChessBoardIndexBuffer(ChessBoard self, char **error);
static bool createChessBoardPipeline(ChessBoard self, char **error);
static void updateVertices(ChessBoard self);

bool createChessBoard(ChessBoard *chessBoard, VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, VkRenderPass renderPass, uint32_t subpass, const char *resourcePath, float anisotropy, float aspectRatio, float width, float originX, float originY, char **error)
{
	*chessBoard = malloc(sizeof(**chessBoard));

	ChessBoard self = *chessBoard;

	self->device = device;
	self->allocator = allocator;
	self->commandPool = commandPool;
	self->queue = queue;
	self->renderPass = renderPass;
	self->subpass = subpass;
	self->resourcePath = resourcePath;
	self->anisotropy = anisotropy;
	self->aspectRatio = aspectRatio;
	self->width = width;
	self->originX = originX;
	self->originY = originY;

	initializePieces(self);
	initializeMove(self);

	if (!createChessBoardVertexBuffer(self, error)) {
		asprintf(error, "Failed to create chess board vertex buffer.\n");
		return false;
	}

	if (!createChessBoardIndexBuffer(self, error)) {
		asprintf(error, "Failed to create chess board index buffer.\n");
		return false;
	}

	if (!createChessBoardTexture(self, error)) {
		asprintf(error, "Failed to create chess board texture.\n");
		return false;
	}

	if (!createChessBoardSampler(self, error)) {
		asprintf(error, "Failed to create chess board sampler.\n");
		return false;
	}

	if (!createChessBoardDescriptors(self, error)) {
		asprintf(error, "Failed to create chess board descriptors.\n");
		return false;
	}

	if (!createChessBoardPipeline(self, error)) {
		asprintf(error, "Failed to create chess board pipeline.\n");
		return false;
	}

	return true;
}

static void initializePieces(ChessBoard self)
{
	Board8x8 initialSetup = {
		BLACK_ROOK, BLACK_KNIGHT, BLACK_BISHOP, BLACK_QUEEN, BLACK_KING, BLACK_BISHOP, BLACK_KNIGHT, BLACK_ROOK,
		BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN,
		EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
		EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
		EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
		EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
		WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN,
		WHITE_ROOK, WHITE_KNIGHT, WHITE_BISHOP, WHITE_QUEEN, WHITE_KING, WHITE_BISHOP, WHITE_KNIGHT, WHITE_ROOK
	};

	setBoard(self, initialSetup);
}

static void initializeMove(ChessBoard self)
{
	MoveBoard8x8 initialSetup = {
		ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL,
		ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL,
		ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL,
		ILLEGAL, ILLEGAL, ILLEGAL, OPEN, CAPTURE, ILLEGAL, ILLEGAL, ILLEGAL,
		ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL,
		ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL,
		ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL,
		ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL
	};

	setMove(self, initialSetup);
}

static bool createChessBoardTexture(ChessBoard self, char **error)
{
	VkExtent2D textureExtent = {
		.width = TEXTURE_WIDTH,
		.height = TEXTURE_HEIGHT
	};

	char *texturePath;
	asprintf(&texturePath, "%s/%s", self->resourcePath, "pieces.rgba");
	char *textureBytes;
	uint32_t textureSize = 0;

	if ((textureSize = readFileToString(texturePath, &textureBytes)) == -1) {
		asprintf(error, "Failed to open texture for reading.\n");
		return false;
	}

	VkBuffer stagingBuffer;
	VmaAllocation stagingBufferAllocation;
	if (!createBuffer(self->device, self->allocator, textureSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT, &stagingBuffer, &stagingBufferAllocation, error)) {
		return false;
	}

	VkResult result;
	void *data;
	if ((result = vmaMapMemory(self->allocator, stagingBufferAllocation, &data)) != VK_SUCCESS) {
		asprintf(error, "Failed to map memory: %s", string_VkResult(result));
		return false;
	}
	memcpy(data, textureBytes, textureSize);
	vmaUnmapMemory(self->allocator, stagingBufferAllocation);

	if (!createImage(self->device, self->allocator, textureExtent, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, &self->textureImage, &self->textureImageAllocation, error)) {
		return false;
	}

	if (!transitionImageLayout(self->device, self->commandPool, self->queue, self->textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, error)) {
		return false;
	}
	if (!copyBufferToImage(self->device, self->commandPool, self->queue, stagingBuffer, self->textureImage, TEXTURE_WIDTH, TEXTURE_HEIGHT, error)) {
		return false;
	}
	if (!transitionImageLayout(self->device, self->commandPool, self->queue, self->textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, error)) {
		return false;
	}

	destroyBuffer(self->allocator, stagingBuffer, stagingBufferAllocation);

	if (!createImageView(self->device, self->textureImage, VK_FORMAT_R8G8B8A8_SRGB, &self->textureImageView, error)) {
		return false;
	}

	return true;
}

static bool createChessBoardSampler(ChessBoard self, char **error)
{
	if (!createSampler(self->device, self->anisotropy, &self->sampler, error)) {
		return false;
	}

	return true;
}

static bool createChessBoardDescriptors(ChessBoard self, char **error)
{
	VkImageLayout imageLayouts[] = {VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
	VkSampler samplers[] = {VK_NULL_HANDLE};
	CreateDescriptorSetInfo createDescriptorSetInfo = {
		.imageViews = &self->textureImageView,
		.imageLayouts = imageLayouts,
		.imageSamplers = &self->sampler,
		.imageCount = 1,
		.buffers = NULL,
		.bufferOffsets = NULL,
		.bufferRanges = NULL,
		.bufferCount = 0
	};
	if (!createDescriptorSets(self->device, createDescriptorSetInfo, &self->textureDescriptorPool, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &self->textureDescriptorSets, &self->textureDescriptorSetLayouts, NULL, NULL, error)) {
		return false;
	}

	return true;
}

static bool createChessBoardVertexBuffer(ChessBoard self, char **error)
{
	if (!createMutableVertexBufferWithStaging(self->device, self->allocator, self->commandPool, self->queue, &self->stagingVertexBufferMappedMemory, &self->stagingVertexBuffer, &self->stagingVertexBufferAllocation, &self->vertexBuffer, &self->vertexBufferAllocation, self->vertices, CHESS_VERTEX_COUNT, error)) {
		return false;
	}

	return true;
}

static bool createChessBoardIndexBuffer(ChessBoard self, char **error)
{
	uint16_t triangleIndices[CHESS_INDEX_COUNT];

	for (size_t i = 0; i < CHESS_SQUARE_COUNT; ++i) {
		size_t verticesOffset = i * 4;
		size_t indicesOffset = i * 6;

		triangleIndices[indicesOffset] = verticesOffset + 0;
		triangleIndices[indicesOffset + 1] = verticesOffset + 1;
		triangleIndices[indicesOffset + 2] = verticesOffset + 2;
		triangleIndices[indicesOffset + 3] = verticesOffset + 2;
		triangleIndices[indicesOffset + 4] = verticesOffset + 3;
		triangleIndices[indicesOffset + 5] = verticesOffset + 0;
	}

	if (!createIndexBuffer(self->device, self->allocator, self->commandPool, self->queue, &self->indexBuffer, &self->indexBufferAllocation, triangleIndices, CHESS_INDEX_COUNT, error)) {
		return false;
	}

	return true;
}

static bool createChessBoardPipeline(ChessBoard self, char **error)
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
		}, {
			.binding = 0,
			.location = 2,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = offsetof(Vertex, texCoord),
		}, {
			.binding = 0,
			.location = 3,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = offsetof(Vertex, texCoord2),
		}
	};

#ifndef EMBED_SHADERS
	char *chessBoardVertShaderPath;
	char *chessBoardFragShaderPath;
	asprintf(&chessBoardVertShaderPath, "%s/%s", self->resourcePath, "chess_board.vert.spv");
	asprintf(&chessBoardFragShaderPath, "%s/%s", self->resourcePath, "chess_board.frag.spv");
	char *chessBoardVertShaderBytes;
	char *chessBoardFragShaderBytes;
	uint32_t chessBoardVertShaderSize = 0;
	uint32_t chessBoardFragShaderSize = 0;

	if ((chessBoardVertShaderSize = readFileToString(chessBoardVertShaderPath, &chessBoardVertShaderBytes)) == -1) {
		asprintf(error, "Failed to open triangle vertex shader for reading.\n");
		return false;
	}
	if ((chessBoardFragShaderSize = readFileToString(chessBoardFragShaderPath, &chessBoardFragShaderBytes)) == -1) {
		asprintf(error, "Failed to open triangle fragment shader for reading.\n");
		return false;
	}
#endif /* EMBED_SHADERS */

	PipelineCreateInfo pipelineCreateInfo = {
		.device = self->device,
		.renderPass = self->renderPass,
		.subpassIndex = self->subpass,
		.vertexShaderBytes = chessBoardVertShaderBytes,
		.vertexShaderSize = chessBoardVertShaderSize,
		.fragmentShaderBytes = chessBoardFragShaderBytes,
		.fragmentShaderSize = chessBoardFragShaderSize,
		.vertexBindingDescriptionCount = 1,
		.vertexBindingDescriptions = &vertexBindingDescription,
		.vertexAttributeDescriptionCount = sizeof(vertexAttributeDescriptions) / sizeof(*vertexAttributeDescriptions),
		.VertexAttributeDescriptions = vertexAttributeDescriptions,
		.descriptorSetLayouts = self->textureDescriptorSetLayouts,
		.descriptorSetLayoutCount = 1,
	};
	bool pipelineCreateSuccess = createPipeline(pipelineCreateInfo, &self->pipelineLayout, &self->pipeline, error);
#ifndef EMBED_SHADERS
	free(chessBoardFragShaderBytes);
	free(chessBoardVertShaderBytes);
#endif /* EMBED_SHADERS */
	if (!pipelineCreateSuccess) {
		return false;
	}

	return true;
}

void setSize(ChessBoard self, float aspectRatio, float width, float originX, float originY)
{
	self->aspectRatio = aspectRatio;
	self->width = width;
	self->originX = originX;
	self->originY = originY;

	updateVertices(self);
}

void setBoard(ChessBoard self, Board8x8 board)
{
	for (size_t i = 0; i < CHESS_SQUARE_COUNT; ++i) {
		self->board[i] = board[i];
	}

	updateVertices(self);
}

void setMove(ChessBoard self, MoveBoard8x8 move)
{
	for (size_t i = 0; i < CHESS_SQUARE_COUNT; ++i) {
		self->move[i] = move[i];
	}

	updateVertices(self);
}

static void updateVertices(ChessBoard self)
{
	float dark[] = {0.71f, 0.533f, 0.388f};
	srgbToLinear(dark);
	float light[] = {0.941f, 0.851f, 0.71f};
	srgbToLinear(light);
	float previousDark[] = {0.671f, 0.635f, 0.227f};
	srgbToLinear(previousDark);
	float previousLight[] = {0.808f, 0.824f, 0.42f};
	srgbToLinear(previousLight);
	float selectedDark[] = {0.749f, 0.475f, 0.271f};
	srgbToLinear(selectedDark);
	float selectedLight[] = {0.914f, 0.694f, 0.494f};
	srgbToLinear(selectedLight);

	const float VIEWPORT_WIDTH = 2.0f;
	const float VIEWPORT_HEIGHT = 2.0f;

	for (size_t i = 0; i < CHESS_SQUARE_COUNT; ++i) {
		float *spriteOrigin = pieceSpriteOriginMap[self->board[i]];
		float *sprite2Origin = iconSpriteOriginMap[self->move[i]];
		size_t verticesOffset = i * 4;
		size_t offsetX = i % 8;
		size_t offsetY = i / 8;
		float squareWidth = VIEWPORT_WIDTH / 8.0f;
		float squareHeight = squareWidth;
		float squareOriginX = (offsetX * squareWidth) - (VIEWPORT_WIDTH / 2);
		float squareOriginY = (offsetY * squareHeight) - (VIEWPORT_HEIGHT / 2);
		const float *color = (offsetY % 2) ?
			((offsetX % 2) ? light : dark) :
			(offsetX % 2) ? dark : light;

		if (offsetX == 3) {
			if (offsetY == 4) {
				color = previousDark;
			} else if (offsetY == 5) {
				color = previousLight;
			}
		}

		if (offsetX == 4) {
			if (offsetY == 4) {
				color = selectedLight;
			} else if (offsetY == 5) {
				color = selectedDark;
			}
		}

		self->vertices[verticesOffset] = (Vertex) {{squareOriginX, squareOriginY}, {color[0], color[1], color[2]}, {spriteOrigin[0], spriteOrigin[1]}, {sprite2Origin[0], sprite2Origin[1]}};
		self->vertices[verticesOffset + 1] = (Vertex) {{squareOriginX + squareWidth, squareOriginY}, {color[0], color[1], color[2]}, {spriteOrigin[0] + 0.25f, spriteOrigin[1]}, {sprite2Origin[0] + 0.25f, sprite2Origin[1]}};
		self->vertices[verticesOffset + 2] = (Vertex) {{squareOriginX + squareWidth, squareOriginY + squareHeight}, {color[0], color[1], color[2]}, {spriteOrigin[0] + 0.25f, spriteOrigin[1] + 0.25f}, {sprite2Origin[0] + 0.25f, sprite2Origin[1] + 0.25f}};
		self->vertices[verticesOffset + 3] = (Vertex) {{squareOriginX, squareOriginY + squareHeight}, {color[0], color[1], color[2]}, {spriteOrigin[0], spriteOrigin[1] + 0.25f}, {sprite2Origin[0], sprite2Origin[1] + 0.25f}};
	}
}

bool updateChessBoard(ChessBoard self, char **error)
{
	if (!updateMutableVertexBufferWithStaging(self->device, self->allocator, self->commandPool, self->queue, self->stagingVertexBufferMappedMemory, &self->stagingVertexBuffer, &self->vertexBuffer, self->vertices, CHESS_VERTEX_COUNT, error)) {
		return false;
	}

	return true;
}

bool drawChessBoard(ChessBoard self, VkCommandBuffer commandBuffer, char **error)
{
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &self->vertexBuffer, offsets);
	vkCmdBindIndexBuffer(commandBuffer, self->indexBuffer, 0, VK_INDEX_TYPE_UINT16);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, self->pipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, self->pipelineLayout, 0, 1, self->textureDescriptorSets, 0, NULL);
	vkCmdDrawIndexed(commandBuffer, CHESS_INDEX_COUNT, 1, 0, 0, 0);

	return true;
}

void chessBoardHandleInputEvent(void *chessBoard, InputEvent *inputEvent)
{
	switch(inputEvent->type) {
	case POINTER_LEAVE:
		printf("got a leave event\n");
		break;
	case BUTTON_DOWN:
		printf("got a button down event\n");
		break;
	case BUTTON_UP:
		printf("got a button up event\n");
		break;
	case NORMALIZED_POINTER_MOVE:
		NormalizedPointerPosition *pointerPosition = inputEvent->data;
		printf("got a move event: %f, %f\n", pointerPosition->x, pointerPosition->y);
		break;
	}
}

void destroyChessBoard(ChessBoard self)
{
	destroyPipeline(self->device, self->pipeline);
	destroyPipelineLayout(self->device, self->pipelineLayout);
	destroyDescriptorPool(self->device, self->textureDescriptorPool);
	destroySampler(self->device, self->sampler);
	vmaUnmapMemory(self->allocator, self->stagingVertexBufferAllocation);
	destroyBuffer(self->allocator, self->stagingVertexBuffer, self->stagingVertexBufferAllocation);
	destroyBuffer(self->allocator, self->vertexBuffer, self->vertexBufferAllocation);
	destroyBuffer(self->allocator, self->indexBuffer, self->indexBufferAllocation);
	destroyImageView(self->device, self->textureImageView);
	destroyImage(self->allocator, self->textureImage, self->textureImageAllocation);
	free(self);
}
