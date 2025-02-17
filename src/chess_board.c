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
#include "synchronization.h"
#include "utils.h"
#include "matrix_utils.h"
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

#ifdef EMBED_MESHES
#include "../mesh_pawn.h"
#endif /* EMBED_MESHES */

typedef struct board_vertex_t {
	float pos[3];
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
	float MV[16];
	float P[16];
	float normalMatrix[16];
} ChessBoardPushConstants;

typedef struct transform_uniform_t {
	float MV[16];
	float P[16];
	float normalMatrix[16];
} TransformUniform;

size_t pieceMeshIndexMap[13] = {
	0,
	5,
	4,
	3,
	2,
	1,
	0,
	5,
	4,
	3,
	2,
	1,
	0
};

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
	VkSampleCountFlagBits sampleCount;
	const char *resourcePath;
	float width;
	float originX;
	float originY;
	Orientation orientation;
	bool enable3d;
	Projection projection;
	VkPipelineLayout boardPipelineLayout;
	VkPipeline boardPipeline;
	BoardVertex boardVertices[CHESS_VERTEX_COUNT + 32];
	void *boardStagingVertexBufferMappedMemory;
	VkBuffer boardStagingVertexBuffer;
	VmaAllocation boardStagingVertexBufferAllocation;
	VkBuffer boardVertexBuffer;
	VmaAllocation boardVertexBufferAllocation;
	VkBuffer boardIndexBuffer;
	VmaAllocation boardIndexBufferAllocation;
	size_t pieceVertexCounts[6];
	size_t pieceVertexOffsets[6];
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
	VkDescriptorPool descriptorPool;
	VkDescriptorSet boardDescriptorSets[MAX_FRAMES_IN_FLIGHT];
	VkDescriptorSetLayout boardDescriptorSetLayouts[MAX_FRAMES_IN_FLIGHT];
	VkDescriptorSet piecesDescriptorSets[MAX_FRAMES_IN_FLIGHT];
	VkDescriptorSetLayout piecesDescriptorSetLayouts[MAX_FRAMES_IN_FLIGHT];
	VkBuffer boardUniformBuffers[MAX_FRAMES_IN_FLIGHT];
	VmaAllocation boardUniformBufferAllocations[MAX_FRAMES_IN_FLIGHT];
	void *boardUniformBufferMappedMemories[MAX_FRAMES_IN_FLIGHT];
	VkBuffer piecesUniformBuffers[MAX_FRAMES_IN_FLIGHT];
	VmaAllocation piecesUniformBufferAllocations[MAX_FRAMES_IN_FLIGHT];
	void *piecesUniformBufferMappedMemories[MAX_FRAMES_IN_FLIGHT];
	TransformUniform boardUniform;
	TransformUniform piecesUniforms[CHESS_SQUARE_COUNT];
	Board8x8 board;
	MoveBoard8x8 move;
	ChessSquare selected;
	LastMove lastMove;
	NormalizedPointerPosition pointerPosition;
	ChessEngine engine;
	float inverseViewProjection[mat4N * mat4N];
};

static void initializePieces(ChessBoard self);
static void initializeMove(ChessBoard self);
static void updateUniformBuffers(ChessBoard self);
static void readObj(void* ctx, const char* filename, const int is_mtl, const char* obj_filename, char** data, size_t* len);
static bool createBoardTexture(ChessBoard self, char **error);
static bool createBoardTextureSampler(ChessBoard self, char **error);
static bool createDescriptors(ChessBoard self, char **error);
static bool createBoardVertexBuffer(ChessBoard self, char **error);
static bool createBoardIndexBuffer(ChessBoard self, char **error);
static bool createBoardPipeline(ChessBoard self, char **error);
static bool createPiecesPipeline(ChessBoard self, char **error);
static bool loadPieceMeshes(ChessBoard self, char **error);
static bool createBoardUniformBuffer(ChessBoard self, char **error);
static bool createPiecesUniformBuffer(ChessBoard self, char **error);
static void updateBoardUniformBuffer(ChessBoard self);
static void updatePiecesUniformBuffer(ChessBoard self);
static void updateVertices(ChessBoard self);
static ChessSquare squareFromPointerPosition(ChessBoard self);
static void screenPositionFromPointerPosition(NormalizedPointerPosition pointerPosition, float screenPosition[mat2N]);
static float getRotationRadians(ChessBoard self);
static void parseTinyobjIntoBuffers(tinyobj_attrib_t attrib, size_t vertexOffset, size_t vertexCount, MeshVertex *vertices, uint16_t *indices);
static void basicSetMove(ChessBoard self, MoveBoard8x8 move);
static void basicSetBoard(ChessBoard self, Board8x8 board);

bool createChessBoard(ChessBoard *chessBoard, ChessEngine engine, VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, VkRenderPass renderPass, uint32_t subpass, VkSampleCountFlagBits sampleCount, const char *resourcePath, float width, float originX, float originY, Orientation orientation, bool enable3d, Projection projection, char **error)
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
	self->sampleCount = sampleCount;
	self->resourcePath = resourcePath;
	self->width = width;
	self->originX = originX;
	self->originY = originY;
	self->orientation = orientation;
	self->enable3d = enable3d;
	self->projection = projection;

	self->selected = CHESS_SQUARE_COUNT;
	self->lastMove = (LastMove) {
		.from = CHESS_SQUARE_COUNT,
		.to = CHESS_SQUARE_COUNT
	};
	initializePieces(self);
	initializeMove(self);
	updateVertices(self);
	updateUniformBuffers(self);

	if (!createBoardVertexBuffer(self, error)) {
		return false;
	}

	if (!createBoardIndexBuffer(self, error)) {
		return false;
	}

	if (!createBoardTexture(self, error)) {
		return false;
	}

	if (!createBoardTextureSampler(self, error)) {
		return false;
	}

	if (!createBoardUniformBuffer(self, error)) {
		return false;
	}

	if (!createPiecesUniformBuffer(self, error)) {
		return false;
	}

	if (!createDescriptors(self, error)) {
		return false;
	}

	if (!createBoardPipeline(self, error)) {
		return false;
	}

	if (!loadPieceMeshes(self, error)) {
		return false;
	}

	if (!createPiecesPipeline(self, error)) {
		return false;
	}

	return true;
}

static float getRotationRadians(ChessBoard self)
{
	switch (self->orientation) {
	case ROTATE_0:
		return 0;
	case ROTATE_90:
		return M_PI_2;
	case ROTATE_180:
		return M_PI;
	case ROTATE_270:
		return -M_PI_2;
	default:
		return 0;
	}
}

static void updateUniformBuffers(ChessBoard self)
{
	float model[mat4N * mat4N];
	float cameraTilt[mat4N * mat4N];
	float cameraTranslation[mat4N * mat4N];
	float view[mat4N * mat4N];
	float preRotation[mat4N * mat4N];
	float perspective[mat4N * mat4N];
	float modelView[mat4N * mat4N];
	float modelViewInverse[mat4N * mat4N];
	float normalMatrix[mat4N * mat4N];
	float projection[mat4N * mat4N];
	float viewProjection[mat4N * mat4N];
	float inverseViewProjection[mat4N * mat4N];

	float rotation = getRotationRadians(self);

	float identity[] = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};

	transformRotation(preRotation, -rotation, 0, 0, 1);
	if (self->enable3d) {
		/* View matrix */
		if (self->projection == PERSPECTIVE) {
			transformRotation(cameraTilt, M_PI / 5, -1, 0, 0);
			transformTranslation(cameraTranslation, 0, 0, 3);
		} else if (self->projection == ORTHOGRAPHIC) {
			transformRotation(cameraTilt, M_PI / 6, -1, 0, 0);
			transformTranslation(cameraTranslation, 0, 0, 2);
		}
		mat4Multiply(cameraTranslation, cameraTilt, view);

		/* Projection matrix */
		if (self->projection == PERSPECTIVE) {
			perspectiveProjection(perspective, M_PI_4, 1.0f, 0.1f, 10.0f);
		} else if (self->projection == ORTHOGRAPHIC) {
			orthographicProjection(perspective, 1.0f, -1.0f, -1.0f, 1.0f, 0.1f, 10.0f);
		}
		mat4Multiply(perspective, preRotation, projection);
	} else {
		mat4Copy(identity, view);
		mat4Copy(preRotation, projection);
	}
	
	/* Board */
	mat4Inverse(view, modelViewInverse);
	mat4Transpose(modelViewInverse, normalMatrix);
	mat4Copy(view, self->boardUniform.MV);
	mat4Copy(projection, self->boardUniform.P);
	mat4Copy(normalMatrix, self->boardUniform.normalMatrix);
	mat4Multiply(projection, view, viewProjection);
	mat4Inverse(viewProjection, inverseViewProjection);
	mat4Copy(inverseViewProjection, self->inverseViewProjection);

	/* Piece */
	for (size_t i = 0; i < CHESS_SQUARE_COUNT; ++i) {
		size_t offsetX = i % 8;
		size_t offsetY = i / 8;

		float modelScale[mat4N * mat4N];
		float modelTranslate[mat4N * mat4N];
		transformTranslation(modelTranslate, (-7 / 8.0f) + (offsetX / 4.0f), (-7 / 8.0f) + (offsetY / 4.0f), 0);
		transformScale(modelScale, 1 / 8.0f);
		mat4Multiply(modelTranslate, modelScale, model);
		mat4Multiply(view, model, modelView);
		mat4Inverse(modelView, modelViewInverse);
		mat4Transpose(modelViewInverse, normalMatrix);
		mat4Copy(modelView, self->piecesUniforms[i].MV);
		mat4Copy(projection, self->piecesUniforms[i].P);
		mat4Copy(normalMatrix, self->piecesUniforms[i].normalMatrix);
	}
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

static bool createBoardTexture(ChessBoard self, char **error)
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
	if (!createBuffer(self->device, self->allocator, piecesTextureDecodedSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, 0, &stagingBuffer, &stagingBufferAllocation, error)) {
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

	if (!createImage(self->device, self->allocator, textureExtent, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, PIECES_TEXTURE_MIP_LEVELS, VK_SAMPLE_COUNT_1_BIT, &self->textureImage, &self->textureImageAllocation, error)) {
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

	if (!createImageView(self->device, self->textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, PIECES_TEXTURE_MIP_LEVELS, &self->textureImageView, error)) {
		return false;
	}

	return true;
}

static bool createBoardTextureSampler(ChessBoard self, char **error)
{
	if (!createSampler(self->device, 0, PIECES_TEXTURE_MIP_LEVELS, &self->sampler, error)) {
		return false;
	}

	return true;
}

static bool createDescriptors(ChessBoard self, char **error)
{
	VkDescriptorBufferInfo boardBufferDescriptorInfo = {
		.buffer = self->boardUniformBuffers[0],
		.offset = 0,
		.range = sizeof(self->boardUniform)
	};

	VkDescriptorBufferInfo piecesBufferDescriptorInfo = {
		.buffer = self->piecesUniformBuffers[0],
		.offset = 0,
		.range = sizeof(self->piecesUniforms[0])
	};

	VkDescriptorImageInfo imageDescriptorInfo = {
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.imageView = self->textureImageView,
		.sampler = self->sampler
	};

	VkDescriptorSetLayoutBinding boardBufferBinding = {
		.binding = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.pImmutableSamplers = NULL
	};

	VkDescriptorSetLayoutBinding imageBinding = {
		.binding = 1,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = NULL
	};

	VkDescriptorSetLayoutBinding piecesBufferBinding = {
		.binding = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.pImmutableSamplers = NULL
	};

	void *boardDescriptorSetDescriptorInfos[] = {&boardBufferDescriptorInfo, &imageDescriptorInfo};
	VkDescriptorSetLayoutBinding boardDescriptorSetBindings[] = {boardBufferBinding, imageBinding};
	void *piecesDescriptorSetDescriptorInfos[] = {&piecesBufferDescriptorInfo};
	CreateDescriptorSetInfo createDescriptorSetInfos[] = {
		{
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT & VK_SHADER_STAGE_FRAGMENT_BIT,
			.descriptorInfos = boardDescriptorSetDescriptorInfos,
			.descriptorCount = 2,
			.bindings = boardDescriptorSetBindings,
			.bindingCount = 2
		}, {
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.descriptorInfos = piecesDescriptorSetDescriptorInfos,
			.descriptorCount = 1,
			.bindings = &piecesBufferBinding,
			.bindingCount = 1
		}
	};
	VkDescriptorSet descriptorSets[2];
	VkDescriptorSetLayout descriptorSetLayouts[2];
	if (!createDescriptorSets(self->device, createDescriptorSetInfos, 2, &self->descriptorPool, descriptorSets, descriptorSetLayouts, error)) {
		return false;
	}
	self->boardDescriptorSets[0] = descriptorSets[0];
	self->piecesDescriptorSets[0] = descriptorSets[1];
	self->boardDescriptorSetLayouts[0] = descriptorSetLayouts[0];
	self->piecesDescriptorSetLayouts[0] = descriptorSetLayouts[1];

	return true;
}

static bool createBoardVertexBuffer(ChessBoard self, char **error)
{
	if (!createMutableBufferWithStaging(self->device, self->allocator, self->commandPool, self->queue, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &self->boardStagingVertexBufferMappedMemory, &self->boardStagingVertexBuffer, &self->boardStagingVertexBufferAllocation, &self->boardVertexBuffer, &self->boardVertexBufferAllocation, self->boardVertices, CHESS_VERTEX_COUNT + 32, sizeof(*self->boardVertices), error)) {
		return false;
	}

	return true;
}

static bool createBoardUniformBuffer(ChessBoard self, char **error)
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		if (!createHostVisibleMutableBuffer(self->device, self->allocator, self->commandPool, self->queue, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &self->boardUniformBufferMappedMemories[i], &self->boardUniformBuffers[i], &self->boardUniformBufferAllocations[i], &self->boardUniform, 1, sizeof(self->boardUniform), error)) {
			return false;
		}
	}

	return true;
}

static bool createPiecesUniformBuffer(ChessBoard self, char **error)
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		if (!createHostVisibleMutableBuffer(self->device, self->allocator, self->commandPool, self->queue, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &self->piecesUniformBufferMappedMemories[i], &self->piecesUniformBuffers[i], &self->piecesUniformBufferAllocations[i], self->piecesUniforms, CHESS_SQUARE_COUNT, sizeof(self->piecesUniforms[0]), error)) {
			return false;
		}
	}

	return true;
}

static void updateBoardUniformBuffer(ChessBoard self)
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		updateHostVisibleMutableBuffer(self->device, self->boardUniformBufferMappedMemories[i], &self->boardUniform, 1, sizeof(self->boardUniform));
	}
}

static void updatePiecesUniformBuffer(ChessBoard self)
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		updateHostVisibleMutableBuffer(self->device, self->piecesUniformBufferMappedMemories[i], self->piecesUniforms, CHESS_SQUARE_COUNT, sizeof(self->piecesUniforms[0]));
	}
}

static bool createBoardIndexBuffer(ChessBoard self, char **error)
{
	uint16_t indices[CHESS_INDEX_COUNT + 48];

	for (size_t i = 0; i < CHESS_SQUARE_COUNT + 8; ++i) {
		size_t verticesOffset = i * 4;
		size_t indicesOffset = i * 6;

		indices[indicesOffset] = verticesOffset + 0;
		indices[indicesOffset + 1] = verticesOffset + 1;
		indices[indicesOffset + 2] = verticesOffset + 2;
		indices[indicesOffset + 3] = verticesOffset + 2;
		indices[indicesOffset + 4] = verticesOffset + 3;
		indices[indicesOffset + 5] = verticesOffset + 0;
	}

	if (!createStaticBuffer(self->device, self->allocator, self->commandPool, self->queue, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &self->boardIndexBuffer, &self->boardIndexBufferAllocation, indices, sizeof(indices[0]), CHESS_INDEX_COUNT + 48, error)) {
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
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(MeshVertex, pos),
		},
		{
			.binding = 0,
			.location = 1,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
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

	VkPipelineDepthStencilStateCreateInfo depthStencilState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.minDepthBounds = 0.0f,
		.maxDepthBounds = 1.0f,
		.stencilTestEnable = VK_FALSE,
		.front = {},
		.back = {}
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
		.descriptorSetLayouts = self->piecesDescriptorSetLayouts,
		.descriptorSetLayoutCount = 1,
		.pushConstantRange = pushConstantRange,
		.depthStencilState = depthStencilState,
		.sampleCount = self->sampleCount
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

static bool createBoardPipeline(ChessBoard self, char **error)
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
			.format = VK_FORMAT_R32G32B32_SFLOAT,
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

	VkPipelineDepthStencilStateCreateInfo depthStencilState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.minDepthBounds = 0.0f,
		.maxDepthBounds = 1.0f,
		.stencilTestEnable = VK_FALSE,
		.front = {},
		.back = {}
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
		.descriptorSetLayouts = self->boardDescriptorSetLayouts,
		.descriptorSetLayoutCount = 1,
		.pushConstantRange = pushConstantRange,
		.depthStencilState = depthStencilState,
		.sampleCount = self->sampleCount
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
	float shadowDark[] = {0.41f, 0.233f, 0.088f};
	srgbToLinear(shadowDark);
	float shadowLight[] = {0.641f, 0.551f, 0.41f};
	srgbToLinear(shadowLight);

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

		self->boardVertices[verticesOffset] = (BoardVertex) {{squareOriginX, squareOriginY, 0}, {color[0], color[1], color[2]}, {spriteOrigin[0], spriteOrigin[1]}, {sprite2Origin[0], sprite2Origin[1]}};
		self->boardVertices[verticesOffset + 1] = (BoardVertex) {{squareOriginX, squareOriginY + squareHeight, 0}, {color[0], color[1], color[2]}, {spriteOrigin[0], spriteOrigin[1] + 0.25f}, {sprite2Origin[0], sprite2Origin[1] + 0.25f}};
		self->boardVertices[verticesOffset + 2] = (BoardVertex) {{squareOriginX + squareWidth, squareOriginY + squareHeight, 0}, {color[0], color[1], color[2]}, {spriteOrigin[0] + 0.25f, spriteOrigin[1] + 0.25f}, {sprite2Origin[0] + 0.25f, sprite2Origin[1] + 0.25f}};
		self->boardVertices[verticesOffset + 3] = (BoardVertex) {{squareOriginX + squareWidth, squareOriginY, 0}, {color[0], color[1], color[2]}, {spriteOrigin[0] + 0.25f, spriteOrigin[1]}, {sprite2Origin[0] + 0.25f, sprite2Origin[1]}};
	}

	if (self->enable3d) {
		size_t sideOffset = CHESS_SQUARE_COUNT * 4;
		for (size_t i = 0; i < 8; ++i) {
			float *spriteOrigin = pieceSpriteOriginMap[EMPTY];
			float *sprite2Origin = iconSpriteOriginMap[ILLEGAL];
			size_t verticesOffset = sideOffset + i * 4;
			float squareWidth = VIEWPORT_WIDTH / 8.0f;
			float squareHeight = squareWidth / 2;
			float squareOriginX = (i * squareWidth) - (VIEWPORT_WIDTH / 2);

			const float *color = (i % 2) ? shadowLight : shadowDark;

			self->boardVertices[verticesOffset] = (BoardVertex) {{squareOriginX, 1.0f, 0}, {color[0], color[1], color[2]}, {spriteOrigin[0], spriteOrigin[1]}, {sprite2Origin[0], sprite2Origin[1]}};
			self->boardVertices[verticesOffset + 1] = (BoardVertex) {{squareOriginX, 1.0f, squareHeight}, {color[0], color[1], color[2]}, {spriteOrigin[0], spriteOrigin[1] + 0.25f}, {sprite2Origin[0], sprite2Origin[1] + 0.25f}};
			self->boardVertices[verticesOffset + 2] = (BoardVertex) {{squareOriginX + squareWidth, 1.0f, squareHeight}, {color[0], color[1], color[2]}, {spriteOrigin[0] + 0.25f, spriteOrigin[1] + 0.25f}, {sprite2Origin[0] + 0.25f, sprite2Origin[1] + 0.25f}};
			self->boardVertices[verticesOffset + 3] = (BoardVertex) {{squareOriginX + squareWidth, 1.0f, 0}, {color[0], color[1], color[2]}, {spriteOrigin[0] + 0.25f, spriteOrigin[1]}, {sprite2Origin[0] + 0.25f, sprite2Origin[1]}};
		}
	}
}

bool updateChessBoard(ChessBoard self, char **error)
{
	if (!updateMutableBufferWithStaging(self->device, self->allocator, self->commandPool, self->queue, self->boardStagingVertexBufferMappedMemory, &self->boardStagingVertexBuffer, &self->boardVertexBuffer, self->boardVertices, CHESS_VERTEX_COUNT, sizeof(*self->boardVertices), error)) {
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
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, self->boardPipelineLayout, 0, 1, self->boardDescriptorSets, 0, NULL);
	vkCmdDrawIndexed(commandBuffer, CHESS_INDEX_COUNT + 48, 1, 0, 0, 0);

	if (self->enable3d) {
		/* Draw Mesh */
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &self->piecesVertexBuffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, self->piecesIndexBuffer, 0, VK_INDEX_TYPE_UINT16);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, self->piecesPipeline);
		for (size_t i = 0; i < CHESS_SQUARE_COUNT; ++i) {
			if (self->board[i] == EMPTY) {
				continue;
			}

			size_t meshIndex = pieceMeshIndexMap[self->board[i]];
			uint32_t piecesUniformBufferOffset = sizeof(self->piecesUniforms[0]) * i;
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, self->piecesPipelineLayout, 0, 1, &self->piecesDescriptorSets[0], 1, &piecesUniformBufferOffset);
			vkCmdDrawIndexed(commandBuffer, self->pieceVertexCounts[meshIndex], 1, self->pieceVertexOffsets[meshIndex], self->pieceVertexOffsets[meshIndex], 0);
		}
	}

	return true;
}

static ChessSquare squareFromPointerPosition(ChessBoard self)
{
	NormalizedPointerPosition pointerPosition = self->pointerPosition;
	if (self->enable3d) {
		float screen[mat2N];
		float intersection[mat3N];
		float planePoint[] = {0.0f, 0.0f, 0.0f};
		float planeNormal[] = {0.0f, 0.0f, 1.0f};
		screenPositionFromPointerPosition(self->pointerPosition, screen);
		castScreenToPlane(intersection, screen, planePoint, planeNormal, self->inverseViewProjection);
		if (intersection[0] < -1.0f || intersection[0] > 1.0f || intersection[1] < -1.0f || intersection[1] > 1.0f) {
			return CHESS_SQUARE_COUNT;
		}
		pointerPosition = (NormalizedPointerPosition) {
			.x = (intersection[0] + 1) / 2,
			.y = (intersection[1] + 1) / 2
		};
	}

	switch (self->orientation) {
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

static void screenPositionFromPointerPosition(NormalizedPointerPosition pointerPosition, float screenPosition[mat2N])
{
	screenPosition[0] = pointerPosition.x * 2 - 1;
	screenPosition[1] = pointerPosition.y * 2 - 1;
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
		if ((square = squareFromPointerPosition(self)) >= CHESS_SQUARE_COUNT) {
			break;
		}
		chessEngineSquareSelected(self->engine, square);
		break;
	case NORMALIZED_POINTER_MOVE:
		self->pointerPosition = *(NormalizedPointerPosition *) inputEvent->data;
		break;
	}
}

static void readObj(void* ctx, const char* filename, const int is_mtl, const char* obj_filename, char** data, size_t* len)
{
#ifndef EMBED_TEXTURES
	if ((*len = readFileToString(obj_filename, data)) == -1) {
    		*data = NULL;
    		*len = 0;
	}
#else /* EMBED_MESHES */
	*data = pawnMeshBytes;
	*len = pawnMeshSize;
#endif /* EMBED_MESHES */
}

static bool loadPieceMeshes(ChessBoard self, char **error)
{
	char *pieceNames[] = {"king", "queen", "rook", "bishop", "knight", "pawn"};
	tinyobj_attrib_t attrib[6];
	tinyobj_shape_t *shapes[6];
	size_t shapeCount[6];
	tinyobj_material_t *materials[6];
	size_t materialCount[6];

	size_t totalVertexCount = 0;

	for (size_t i = 0; i < 6; ++i) {
		char *pieceMeshPath;
		asprintf(&pieceMeshPath, "%s/%s.obj", self->resourcePath, pieceNames[i]);

		if (tinyobj_parse_obj(attrib + i, shapes + i, shapeCount + i, materials + i, materialCount + i, pieceMeshPath, readObj, NULL, TINYOBJ_FLAG_TRIANGULATE) != TINYOBJ_SUCCESS) {
			free(pieceMeshPath);
			asprintf(error, "Failed to load mesh.\n");
			return false;
		}

		free(pieceMeshPath);

		self->pieceVertexCounts[i] = attrib[i].num_face_num_verts * 3;
		self->pieceVertexOffsets[i] = i == 0 ? 0 : self->pieceVertexOffsets[i - 1] + self->pieceVertexCounts[i - 1];
		totalVertexCount += self->pieceVertexCounts[i];
	}

	MeshVertex *vertices = malloc(sizeof(*vertices) * totalVertexCount);
	uint16_t *indices = malloc(sizeof(*indices) * totalVertexCount);

	for (size_t i = 0; i < 6; ++i) {
		parseTinyobjIntoBuffers(attrib[i], self->pieceVertexOffsets[i], self->pieceVertexCounts[i], vertices, indices);
	}

	if (!createStaticBuffer(self->device, self->allocator, self->commandPool, self->queue, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &self->piecesVertexBuffer, &self->piecesVertexBufferAllocation, vertices, totalVertexCount, sizeof(*vertices), error)) {
		free(vertices);
		free(indices);
		return false;
	}

	free(vertices);

	if (!createStaticBuffer(self->device, self->allocator, self->commandPool, self->queue, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &self->piecesIndexBuffer, &self->piecesIndexBufferAllocation, indices, totalVertexCount, sizeof(*indices), error)) {
		free(vertices);
		free(indices);
		return false;
	}

	free(indices);

	for (size_t i = 0; i < 6; ++i) {
		tinyobj_attrib_free(attrib + i);
		tinyobj_shapes_free(shapes[i], shapeCount[i]);
		tinyobj_materials_free(materials[i], materialCount[i]);
	}

	return true;
}

static void parseTinyobjIntoBuffers(tinyobj_attrib_t attrib, size_t vertexOffset, size_t vertexCount, MeshVertex *vertices, uint16_t *indices)
{
	size_t faceOffset = 0;

	for (size_t i = 0; i < attrib.num_face_num_verts; ++i) {
		for (size_t f = 0; f < (size_t) attrib.face_num_verts[i] / 3; ++f) {
			tinyobj_vertex_index_t idx0 = attrib.faces[faceOffset + 3 * f + 0];
			tinyobj_vertex_index_t idx1 = attrib.faces[faceOffset + 3 * f + 1];
			tinyobj_vertex_index_t idx2 = attrib.faces[faceOffset + 3 * f + 2];

			float v[3][3];

			for (size_t k = 0; k < 3; ++k) {
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
 					for (size_t k = 0; k < 3; ++k) {
						n[0][k] = attrib.normals[3 * (size_t)f0 + k];
						n[1][k] = attrib.normals[3 * (size_t)f1 + k];
						n[2][k] = attrib.normals[3 * (size_t)f2 + k];
					}
				}
			}

			for (size_t j = 0; j < 3; ++j) {
				vertices[vertexOffset + faceOffset + (f * 3) + j] = (MeshVertex) {
					.pos = {v[j][0], v[j][1], v[j][2]},
					.normal = {n[j][0], n[j][1], n[j][2]}
				};
			}
		}

		faceOffset += (size_t) attrib.face_num_verts[i];
	}

	for (size_t i = 0; i < vertexCount; ++i) {
		indices[vertexOffset + i] = i;
	}
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
	destroyDescriptorPool(self->device, self->descriptorPool);
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		destroyDescriptorSetLayout(self->device, self->boardDescriptorSetLayouts[i]);
		destroyDescriptorSetLayout(self->device, self->piecesDescriptorSetLayouts[i]);
		vmaUnmapMemory(self->allocator, self->boardUniformBufferAllocations[i]);
		destroyBuffer(self->allocator, self->boardUniformBuffers[i], self->boardUniformBufferAllocations[i]);
		vmaUnmapMemory(self->allocator, self->piecesUniformBufferAllocations[i]);
		destroyBuffer(self->allocator, self->piecesUniformBuffers[i], self->piecesUniformBufferAllocations[i]);
	}
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

void chessBoardSetDimensions(ChessBoard self, float width, float originX, float originY, Orientation orientation)
{
	self->width = width;
	self->originX = originX;
	self->originY = originY;
	self->orientation = orientation;

	updateUniformBuffers(self);
	updateBoardUniformBuffer(self);
	updatePiecesUniformBuffer(self);
	updateVertices(self);
}

void basicSetBoard(ChessBoard self, Board8x8 board)
{
	for (size_t i = 0; i < CHESS_SQUARE_COUNT; ++i) {
		self->board[i] = board[i];
	}
}

void chessBoardSetBoard(ChessBoard self, Board8x8 board)
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

void chessBoardSetMove(ChessBoard self, MoveBoard8x8 move)
{
	basicSetMove(self, move);
	updateVertices(self);
}

void chessBoardSetSelected(ChessBoard self, ChessSquare selected)
{
	self->selected = selected;

	updateVertices(self);
}

void chessBoardSetLastMove(ChessBoard self, LastMove lastMove)
{
	self->lastMove = lastMove;

	updateVertices(self);
}

bool chessBoardGetEnable3d(ChessBoard self)
{
	return self->enable3d;
}

void chessBoardSetEnable3d(ChessBoard self, bool enable3d)
{
	if (self->enable3d == enable3d) {
		return;
	}

	self->enable3d = enable3d;

	updateVertices(self);
	updateUniformBuffers(self);
	updateBoardUniformBuffer(self);
	updatePiecesUniformBuffer(self);
}
