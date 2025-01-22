#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "lodepng.h"

#include "chess_board.h"
#include "descriptor.h"
#include "image.h"
#include "image_view.h"
#include "pipeline.h"
#include "sampler.h"
#include "utils.h"
#include "tinyobj_loader_c.h"

#ifdef EMBED_SHADERS
#include "../shader_chess_board.vert.h"
#include "../shader_chess_board.frag.h"
#include "../shader_phong.vert.h"
#include "../shader_phong.frag.h"
#endif /* EMBED_SHADERS */

#ifdef EMBED_TEXTURES
#include "../texture_pieces.h"
#endif /* EMBED_TEXTURES */

typedef struct board_vertex_t {
	float pos[2];
	float color[3];
	float texCoord[2];
	float texCoord2[2];
} BoardVertex;

typedef struct mesh_vertex_t {
	float pos[3];
	float normal[3];
} MeshVertex;

#define CHESS_VERTEX_COUNT CHESS_SQUARE_COUNT * 4
#define CHESS_INDEX_COUNT CHESS_SQUARE_COUNT * 6
#define PIECES_TEXTURE_MIP_LEVELS 7

typedef struct chess_board_push_constants_t {
	float mvp[16];
} ChessBoardPushConstants;

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
	float width;
	float originX;
	float originY;
	Orientation orientation;
	VkPipelineLayout boardPipelineLayout;
	VkPipeline boardPipeline;
	BoardVertex boardVertices[CHESS_VERTEX_COUNT];
	void *boardStagingVertexBufferMappedMemory;
	VkBuffer boardStagingVertexBuffer;
	VmaAllocation boardStagingVertexBufferAllocation;
	VkBuffer boardVertexBuffer;
	VmaAllocation boardVertexBufferAllocation;
	VkBuffer boardIndexBuffer;
	VmaAllocation boardIndexBufferAllocation;
	size_t piecesVertexCount;
	VkBuffer piecesVertexBuffer;
	VmaAllocation piecesVertexBufferAllocation;
	VkBuffer piecesIndexBuffer;
	VmaAllocation piecesIndexBufferAllocation;
	VkPipelineLayout piecesPipelineLayout;
	VkPipeline piecesPipeline;
	VkImage textureImage;
	VmaAllocation textureImageAllocation;
	VkImageView textureImageView;
	VkSampler sampler;
	VkDescriptorPool textureDescriptorPool;
	VkDescriptorSet *textureDescriptorSets;
	VkDescriptorSetLayout *textureDescriptorSetLayouts;
	Board8x8 board;
	MoveBoard8x8 move;
	ChessSquare selected;
	LastMove lastMove;
	NormalizedPointerPosition pointerPosition;
	ChessEngine engine;
};

static void basicSetMove(ChessBoard self, MoveBoard8x8 move);
static void basicSetBoard(ChessBoard self, Board8x8 board);
static void initializePieces(ChessBoard self);
static void initializeMove(ChessBoard self);
static void readObj(void* ctx, const char* filename, const int is_mtl, const char* obj_filename, char** data, size_t* len);
static bool createChessBoardTexture(ChessBoard self, char **error);
static bool createChessBoardSampler(ChessBoard self, char **error);
static bool createChessBoardDescriptors(ChessBoard self, char **error);
static bool createChessBoardVertexBuffer(ChessBoard self, char **error);
static bool createChessBoardIndexBuffer(ChessBoard self, char **error);
static bool createChessBoardPipeline(ChessBoard self, char **error);
static bool chessBoardLoadPieceMeshes(ChessBoard self, char **error);
static bool createPiecesPipeline(ChessBoard self, char **error);
static void updateVertices(ChessBoard self);
static ChessSquare squareFromPointerPosition(NormalizedPointerPosition pointerPosition, Orientation orientation);

bool createChessBoard(ChessBoard *chessBoard, ChessEngine engine, VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, VkRenderPass renderPass, uint32_t subpass, const char *resourcePath, float width, float originX, float originY, Orientation orientation, char **error)
{
	*chessBoard = malloc(sizeof(**chessBoard));

	ChessBoard self = *chessBoard;

	self->engine = engine;
	self->device = device;
	self->allocator = allocator;
	self->commandPool = commandPool;
	self->queue = queue;
	self->renderPass = renderPass;
	self->subpass = subpass;
	self->resourcePath = resourcePath;
	self->width = width;
	self->originX = originX;
	self->originY = originY;
	self->orientation = orientation;

	self->selected = CHESS_SQUARE_COUNT;
	self->lastMove = (LastMove) {
		.from = CHESS_SQUARE_COUNT,
		.to = CHESS_SQUARE_COUNT
	};
	initializePieces(self);
	initializeMove(self);
	updateVertices(self);

	if (!createChessBoardVertexBuffer(self, error)) {
		return false;
	}

	if (!createChessBoardIndexBuffer(self, error)) {
		return false;
	}

	if (!createChessBoardTexture(self, error)) {
		return false;
	}

	if (!createChessBoardSampler(self, error)) {
		return false;
	}

	if (!createChessBoardDescriptors(self, error)) {
		return false;
	}

	if (!createChessBoardPipeline(self, error)) {
		return false;
	}

	if (!chessBoardLoadPieceMeshes(self, error)) {
		return false;
	}

	if (!createPiecesPipeline(self, error)) {
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

	basicSetBoard(self, initialSetup);
}

static void initializeMove(ChessBoard self)
{
	MoveBoard8x8 initialSetup = {
		ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL,
		ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL,
		ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL,
		ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL,
		ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL,
		ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL,
		ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL,
		ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL
	};

	basicSetMove(self, initialSetup);
}

static bool createChessBoardTexture(ChessBoard self, char **error)
{
#ifndef EMBED_TEXTURES
	char *piecesTexturePath;
	asprintf(&piecesTexturePath, "%s/%s", self->resourcePath, "pieces.png");
	char *piecesTextureBytes;
	uint32_t piecesTextureSize = 0;

	if ((piecesTextureSize = readFileToString(piecesTexturePath, &piecesTextureBytes)) == -1) {
		asprintf(error, "Failed to open texture for reading.\n");
		return false;
	}
#endif /* EMBED_TEXTURES */

	unsigned lodepngResult;
	unsigned char *piecesTextureDecodedBytes;
	unsigned piecesTextureDecodedWidth;
	unsigned piecesTextureDecodedHeight;

	if (lodepngResult = lodepng_decode32(&piecesTextureDecodedBytes, &piecesTextureDecodedWidth, &piecesTextureDecodedHeight, piecesTextureBytes, piecesTextureSize)) {
		asprintf(error, "Failed to decode PNG: %s\n", lodepng_error_text(lodepngResult));
		return false;
	}

	VkExtent2D textureExtent = {
		.width = piecesTextureDecodedHeight,
		.height = piecesTextureDecodedHeight
	};
	unsigned piecesTextureDecodedSize = (piecesTextureDecodedWidth * piecesTextureDecodedHeight) * (32 / sizeof(piecesTextureDecodedBytes));

#ifndef EMBED_TEXTURES
	free(piecesTextureBytes);
#endif /* EMBED_TEXTURES */

	VkBuffer stagingBuffer;
	VmaAllocation stagingBufferAllocation;
	if (!createBuffer(self->device, self->allocator, piecesTextureDecodedSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT, &stagingBuffer, &stagingBufferAllocation, error)) {
		return false;
	}

	VkResult result;
	void *data;
	if ((result = vmaMapMemory(self->allocator, stagingBufferAllocation, &data)) != VK_SUCCESS) {
		asprintf(error, "Failed to map memory: %s", string_VkResult(result));
		return false;
	}
	memcpy(data, piecesTextureDecodedBytes, piecesTextureDecodedSize);
	free(piecesTextureDecodedBytes);
	vmaUnmapMemory(self->allocator, stagingBufferAllocation);

	if (!createImage(self->device, self->allocator, textureExtent, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, PIECES_TEXTURE_MIP_LEVELS, &self->textureImage, &self->textureImageAllocation, error)) {
		return false;
	}

	if (!transitionImageLayout(self->device, self->commandPool, self->queue, self->textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, PIECES_TEXTURE_MIP_LEVELS, error)) {
		return false;
	}

	if (!copyBufferToImage(self->device, self->commandPool, self->queue, stagingBuffer, self->textureImage, piecesTextureDecodedWidth, piecesTextureDecodedHeight, PIECES_TEXTURE_MIP_LEVELS, error)) {
		return false;
	}

	if (!transitionImageLayout(self->device, self->commandPool, self->queue, self->textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, PIECES_TEXTURE_MIP_LEVELS, error)) {
		return false;
	}

	destroyBuffer(self->allocator, stagingBuffer, stagingBufferAllocation);

	if (!createImageView(self->device, self->textureImage, VK_FORMAT_R8G8B8A8_SRGB, PIECES_TEXTURE_MIP_LEVELS, &self->textureImageView, error)) {
		return false;
	}

	return true;
}

static bool createChessBoardSampler(ChessBoard self, char **error)
{
	if (!createSampler(self->device, 0, PIECES_TEXTURE_MIP_LEVELS, &self->sampler, error)) {
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
	if (!createMutableVertexBufferWithStaging(self->device, self->allocator, self->commandPool, self->queue, &self->boardStagingVertexBufferMappedMemory, &self->boardStagingVertexBuffer, &self->boardStagingVertexBufferAllocation, &self->boardVertexBuffer, &self->boardVertexBufferAllocation, self->boardVertices, CHESS_VERTEX_COUNT, sizeof(*self->boardVertices), error)) {
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

	if (!createIndexBuffer(self->device, self->allocator, self->commandPool, self->queue, &self->boardIndexBuffer, &self->boardIndexBufferAllocation, triangleIndices, CHESS_INDEX_COUNT, error)) {
		return false;
	}

	return true;
}

static bool createPiecesPipeline(ChessBoard self, char **error)
{
	VkVertexInputBindingDescription vertexBindingDescription = {
		.binding = 0,
		.stride = sizeof(MeshVertex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	};

	VkVertexInputAttributeDescription vertexAttributeDescriptions[] = {
		{
			.binding = 0,
			.location = 0,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = offsetof(MeshVertex, pos),
		},
		{
			.binding = 0,
			.location = 1,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = offsetof(MeshVertex, normal),
		}
	};

#ifndef EMBED_SHADERS
	char *phongVertShaderPath;
	char *phongFragShaderPath;
	asprintf(&phongVertShaderPath, "%s/%s", self->resourcePath, "phong.vert.spv");
	asprintf(&phongFragShaderPath, "%s/%s", self->resourcePath, "phong.frag.spv");
	char *phongVertShaderBytes;
	char *phongFragShaderBytes;
	uint32_t phongVertShaderSize = 0;
	uint32_t phongFragShaderSize = 0;

	if ((phongVertShaderSize = readFileToString(phongVertShaderPath, &phongVertShaderBytes)) == -1) {
		asprintf(error, "Failed to open phong vertex shader for reading.\n");
		return false;
	}
	if ((phongFragShaderSize = readFileToString(phongFragShaderPath, &phongFragShaderBytes)) == -1) {
		asprintf(error, "Failed to open phong fragment shader for reading.\n");
		return false;
	}
#endif /* EMBED_SHADERS */

	VkPushConstantRange pushConstantRange = {
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.offset = 0,
		.size = sizeof(ChessBoardPushConstants)
	};

	PipelineCreateInfo pipelineCreateInfo = {
		.device = self->device,
		.renderPass = self->renderPass,
		.subpassIndex = self->subpass,
		.vertexShaderBytes = phongVertShaderBytes,
		.vertexShaderSize = phongVertShaderSize,
		.fragmentShaderBytes = phongFragShaderBytes,
		.fragmentShaderSize = phongFragShaderSize,
		.vertexBindingDescriptionCount = 1,
		.vertexBindingDescriptions = &vertexBindingDescription,
		.vertexAttributeDescriptionCount = sizeof(vertexAttributeDescriptions) / sizeof(*vertexAttributeDescriptions),
		.VertexAttributeDescriptions = vertexAttributeDescriptions,
		.descriptorSetLayouts = self->textureDescriptorSetLayouts,
		.descriptorSetLayoutCount = 1,
		.pushConstantRange = pushConstantRange
	};
	bool pipelineCreateSuccess = createPipeline(pipelineCreateInfo, &self->piecesPipelineLayout, &self->piecesPipeline, error);
#ifndef EMBED_SHADERS
	free(phongFragShaderBytes);
	free(phongVertShaderBytes);
#endif /* EMBED_SHADERS */
	if (!pipelineCreateSuccess) {
		return false;
	}

	return true;
}

static bool createChessBoardPipeline(ChessBoard self, char **error)
{
	VkVertexInputBindingDescription vertexBindingDescription = {
		.binding = 0,
		.stride = sizeof(BoardVertex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	};

	VkVertexInputAttributeDescription vertexAttributeDescriptions[] = {
		{
			.binding = 0,
			.location = 0,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = offsetof(BoardVertex, pos),
		}, {
			.binding = 0,
			.location = 1,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(BoardVertex, color),
		}, {
			.binding = 0,
			.location = 2,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = offsetof(BoardVertex, texCoord),
		}, {
			.binding = 0,
			.location = 3,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = offsetof(BoardVertex, texCoord2),
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

	VkPushConstantRange pushConstantRange = {
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.offset = 0,
		.size = sizeof(ChessBoardPushConstants)
	};

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
		.pushConstantRange = pushConstantRange
	};
	bool pipelineCreateSuccess = createPipeline(pipelineCreateInfo, &self->boardPipelineLayout, &self->boardPipeline, error);
#ifndef EMBED_SHADERS
	free(chessBoardFragShaderBytes);
	free(chessBoardVertShaderBytes);
#endif /* EMBED_SHADERS */
	if (!pipelineCreateSuccess) {
		return false;
	}

	return true;
}

void setDimensions(ChessBoard self, float width, float originX, float originY, Orientation orientation)
{
	self->width = width;
	self->originX = originX;
	self->originY = originY;
	self->orientation = orientation;

	updateVertices(self);
}

void basicSetBoard(ChessBoard self, Board8x8 board)
{
	for (size_t i = 0; i < CHESS_SQUARE_COUNT; ++i) {
		self->board[i] = board[i];
	}
}

void setBoard(ChessBoard self, Board8x8 board)
{
	basicSetBoard(self, board);
	updateVertices(self);
}

void basicSetMove(ChessBoard self, MoveBoard8x8 move)
{
	for (size_t i = 0; i < CHESS_SQUARE_COUNT; ++i) {
		self->move[i] = move[i];
	}
}

void setMove(ChessBoard self, MoveBoard8x8 move)
{
	basicSetMove(self, move);
	updateVertices(self);
}

void setSelected(ChessBoard self, ChessSquare selected)
{
	self->selected = selected;

	updateVertices(self);
}

void setLastMove(ChessBoard self, LastMove lastMove)
{
	self->lastMove = lastMove;

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

		float *thisLight = light;
		float *thisDark = dark;
		if (i == self->lastMove.from || i == self->lastMove.to) {
			thisLight = previousLight;
			thisDark = previousDark;
		}
		if (i == self->selected) {
			thisLight = selectedLight;
			thisDark = selectedDark;
		}
		const float *color = (offsetY % 2) ?
			((offsetX % 2) ? thisLight : thisDark) :
			(offsetX % 2) ? thisDark : thisLight;

		self->boardVertices[verticesOffset] = (BoardVertex) {{squareOriginX, squareOriginY}, {color[0], color[1], color[2]}, {spriteOrigin[0], spriteOrigin[1]}, {sprite2Origin[0], sprite2Origin[1]}};
		self->boardVertices[verticesOffset + 1] = (BoardVertex) {{squareOriginX + squareWidth, squareOriginY}, {color[0], color[1], color[2]}, {spriteOrigin[0] + 0.25f, spriteOrigin[1]}, {sprite2Origin[0] + 0.25f, sprite2Origin[1]}};
		self->boardVertices[verticesOffset + 2] = (BoardVertex) {{squareOriginX + squareWidth, squareOriginY + squareHeight}, {color[0], color[1], color[2]}, {spriteOrigin[0] + 0.25f, spriteOrigin[1] + 0.25f}, {sprite2Origin[0] + 0.25f, sprite2Origin[1] + 0.25f}};
		self->boardVertices[verticesOffset + 3] = (BoardVertex) {{squareOriginX, squareOriginY + squareHeight}, {color[0], color[1], color[2]}, {spriteOrigin[0], spriteOrigin[1] + 0.25f}, {sprite2Origin[0], sprite2Origin[1] + 0.25f}};
	}
}

bool updateChessBoard(ChessBoard self, char **error)
{
	if (!updateMutableVertexBufferWithStaging(self->device, self->allocator, self->commandPool, self->queue, self->boardStagingVertexBufferMappedMemory, &self->boardStagingVertexBuffer, &self->boardVertexBuffer, self->boardVertices, CHESS_VERTEX_COUNT, sizeof(*self->boardVertices), error)) {
		return false;
	}

	return true;
}

bool drawChessBoard(ChessBoard self, VkCommandBuffer commandBuffer, char **error)
{
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &self->boardVertexBuffer, offsets);
	vkCmdBindIndexBuffer(commandBuffer, self->boardIndexBuffer, 0, VK_INDEX_TYPE_UINT16);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, self->boardPipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, self->boardPipelineLayout, 0, 1, self->textureDescriptorSets, 0, NULL);
	float rotation;
	switch (self->orientation) {
		case ROTATE_0:
			rotation = 0;
			break;
		case ROTATE_90:
			rotation = M_PI_2;
			break;
		case ROTATE_180:
			rotation = M_PI;
			break;
		case ROTATE_270:
			rotation = -M_PI_2;
			break;
	}
	ChessBoardPushConstants pushConstants = {
		.mvp = {
			cos(rotation), -sin(rotation), 0, 0,
			sin(rotation), cos(rotation), 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		}
	};
	vkCmdPushConstants(commandBuffer, self->boardPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);
	vkCmdDrawIndexed(commandBuffer, CHESS_INDEX_COUNT, 1, 0, 0, 0);

	pushConstants = (ChessBoardPushConstants) {
		.mvp = {
			0.2, 0, 0, 0,
			0, 0.2, 0, 0,
			0, 0, 0.2, 0,
			0, 0, 0, 1
		}
	};
	vkCmdPushConstants(commandBuffer, self->boardPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &self->piecesVertexBuffer, offsets);
	vkCmdBindIndexBuffer(commandBuffer, self->piecesIndexBuffer, 0, VK_INDEX_TYPE_UINT16);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, self->piecesPipeline);
	vkCmdDrawIndexed(commandBuffer, self->piecesVertexCount, 1, 0, 0, 0);

	return true;
}

static ChessSquare squareFromPointerPosition(NormalizedPointerPosition pointerPosition, Orientation orientation)
{
	switch (orientation) {
	case ROTATE_0:
		return (floor(pointerPosition.y * 8) * 8) + floor(pointerPosition.x * 8);
	case ROTATE_90:
		return ((int) floor(pointerPosition.x * 8) * 8) + (int) floor((1.0 - pointerPosition.y) * 8);
	case ROTATE_180:
		return (floor((1 - pointerPosition.y) * 8) * 8) + floor((1.0 - pointerPosition.x) * 8);
	case ROTATE_270:
		return ((int) floor((1.0 - pointerPosition.x) * 8) * 8) + (int) floor(pointerPosition.y * 8);
	}
}

void chessBoardHandleInputEvent(void *chessBoard, InputEvent *inputEvent)
{
	ChessBoard self = (ChessBoard) chessBoard;
	ChessSquare square;

	switch(inputEvent->type) {
	case POINTER_LEAVE:
		break;
	case BUTTON_DOWN:
		break;
	case BUTTON_UP:
		square = squareFromPointerPosition(self->pointerPosition, self->orientation);
		chessEngineSquareSelected(self->engine, square);
		break;
	case NORMALIZED_POINTER_MOVE:
		self->pointerPosition = *(NormalizedPointerPosition *) inputEvent->data;
		break;
	}
}

static void readObj(void* ctx, const char* filename, const int is_mtl, const char* obj_filename, char** data, size_t* len)
{
	if ((*len = readFileToString(obj_filename, data)) == -1) {
    		*data = NULL;
    		*len = 0;
	}
}

static bool chessBoardLoadPieceMeshes(ChessBoard self, char **error)
{
	tinyobj_attrib_t attrib;
	tinyobj_shape_t *shapes = NULL;
	size_t shapeCount;
	tinyobj_material_t *materials = NULL;
	size_t materialCount;
	char *piecesMeshPath;
	asprintf(&piecesMeshPath, "%s/%s", self->resourcePath, "model.obj");

        if (tinyobj_parse_obj(&attrib, &shapes, &shapeCount, &materials, &materialCount, piecesMeshPath, readObj, NULL, TINYOBJ_FLAG_TRIANGULATE) != TINYOBJ_SUCCESS) {
		asprintf(error, "Failed to load mesh.\n");
		return false;
	}

	self->piecesVertexCount = attrib.num_face_num_verts * 3;
	MeshVertex *vertices = malloc(sizeof(*vertices) * self->piecesVertexCount);

	size_t faceOffset = 0;

	for (size_t i = 0; i < attrib.num_face_num_verts; i++) {
		for (size_t f = 0; f < (size_t) attrib.face_num_verts[i] / 3; f++) {
			tinyobj_vertex_index_t idx0 = attrib.faces[faceOffset + 3 * f + 0];
			tinyobj_vertex_index_t idx1 = attrib.faces[faceOffset + 3 * f + 1];
			tinyobj_vertex_index_t idx2 = attrib.faces[faceOffset + 3 * f + 2];

			float v[3][3];

			for (size_t k = 0; k < 3; k++) {
				int f0 = idx0.v_idx;
				int f1 = idx1.v_idx;
				int f2 = idx2.v_idx;

				v[0][k] = attrib.vertices[3 * (size_t) f0 + k];
				v[1][k] = attrib.vertices[3 * (size_t) f1 + k];
				v[2][k] = attrib.vertices[3 * (size_t) f2 + k];
			}

			float n[3][3];

			if (attrib.num_normals > 0) {
				int f0 = idx0.vn_idx;
				int f1 = idx1.vn_idx;
				int f2 = idx2.vn_idx;

				if (f0 >= 0 && f1 >= 0 && f2 >= 0) {
 					for (size_t k = 0; k < 3; k++) {
						n[0][k] = attrib.normals[3 * (size_t)f0 + k];
						n[1][k] = attrib.normals[3 * (size_t)f1 + k];
						n[2][k] = attrib.normals[3 * (size_t)f2 + k];
					}
				}
			}

			for (size_t j = 0; j < 3; ++j) {
				vertices[faceOffset + (f * 3) + j] = (MeshVertex) {
					.pos = {v[j][0], v[j][1], v[j][2]},
					.normal = {n[j][0], n[j][1], n[j][2]}
				};
			}
		}

		faceOffset += (size_t) attrib.face_num_verts[i];
	}

	if (!createStaticVertexBuffer(self->device, self->allocator, self->commandPool, self->queue, &self->piecesVertexBuffer, &self->piecesVertexBufferAllocation, vertices, self->piecesVertexCount, sizeof(*vertices), error)) {
		return false;
	}

	uint16_t *indices = malloc(sizeof(*indices) * self->piecesVertexCount);

	for (size_t i = 0; i < self->piecesVertexCount; ++i) {
		indices[i] = i;
	}

	if (!createIndexBuffer(self->device, self->allocator, self->commandPool, self->queue, &self->piecesIndexBuffer, &self->piecesIndexBufferAllocation, indices, self->piecesVertexCount, error)) {
		return false;
	}

	free(indices);

	return true;
}

void destroyChessBoard(ChessBoard self)
{
#ifndef EMBED_TEXTURES
	/* TODO: free textures and meshes */
#endif /* EMBED_TEXTURES */
	destroyPipeline(self->device, self->boardPipeline);
	destroyPipelineLayout(self->device, self->boardPipelineLayout);
	destroyPipeline(self->device, self->piecesPipeline);
	destroyPipelineLayout(self->device, self->piecesPipelineLayout);
	destroyDescriptorPool(self->device, self->textureDescriptorPool);
	destroySampler(self->device, self->sampler);
	vmaUnmapMemory(self->allocator, self->boardStagingVertexBufferAllocation);
	destroyBuffer(self->allocator, self->boardStagingVertexBuffer, self->boardStagingVertexBufferAllocation);
	destroyBuffer(self->allocator, self->boardVertexBuffer, self->boardVertexBufferAllocation);
	destroyBuffer(self->allocator, self->boardIndexBuffer, self->boardIndexBufferAllocation);
	destroyBuffer(self->allocator, self->piecesVertexBuffer, self->piecesVertexBufferAllocation);
	destroyBuffer(self->allocator, self->piecesIndexBuffer, self->piecesIndexBufferAllocation);
	destroyImageView(self->device, self->textureImageView);
	destroyImage(self->allocator, self->textureImage, self->textureImageAllocation);
	free(self);
}
