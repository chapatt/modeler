#include "lodepng.h"

#include "titlebar.h"
#include "descriptor.h"
#include "image.h"
#include "image_view.h"
#include "pipeline.h"
#include "sampler.h"
#include "synchronization.h"
#include "utils.h"

#ifdef EMBED_SHADERS
#include "../shader_titlebar.vert.h"
#include "../shader_titlebar.frag.h"
#endif /* EMBED_SHADERS */

#ifdef EMBED_TEXTURES
#include "../texture_titlebar.h"
#endif /* EMBED_TEXTURES */

static const float VIEWPORT_WIDTH = 2.0f;
static const float VIEWPORT_HEIGHT = 2.0f;

typedef struct titlebar_push_constants_t {
	float ambientColor[4];
} TitlebarPushConstants;

float iconSpriteOriginMap[3][2] = {
	{0.75f, 0.75f},
	{0.5f, 0.25f},
	{0.75f, 0.25f}
};

struct titlebar_t {
	VkDevice device;
	VmaAllocator allocator;
	VkCommandPool commandPool;
	VkQueue queue;
	VkRenderPass renderPass;
	uint32_t subpass;
	VkSampleCountFlagBits sampleCount;
	const char *resourcePath;
	Orientation orientation;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	VkImage textureImage;
	VmaAllocation textureImageAllocation;
	VkImageView textureImageView;
	VkSampler sampler;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSets[MAX_FRAMES_IN_FLIGHT];
	VkDescriptorSetLayout descriptorSetLayouts[MAX_FRAMES_IN_FLIGHT];
	NormalizedPointerPosition pointerPosition;
};

static bool createTexture(Titlebar self, char **error);
static bool createTextureSampler(Titlebar self, char **error);
static bool createDescriptors(Titlebar self, char **error);
static bool createPipeline(Titlebar self, char **error);

bool createTitlebar(Titlebar *titlebar, VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, VkRenderPass renderPass, uint32_t subpass, VkSampleCountFlagBits sampleCount, const char *resourcePath, char **error)
{
	*chessBoard = malloc(sizeof(**chessBoard));

	ChessBoard self = *chessBoard;

	self->device = device;
	self->allocator = allocator;
	self->commandPool = commandPool;
	self->queue = queue;
	self->renderPass = renderPass;
	self->subpass = subpass;
	self->sampleCount = sampleCount;
	self->resourcePath = resourcePath;

	if (!createTexture(self, error)) {
		return false;
	}

	if (!createTextureSampler(self, error)) {
		return false;
	}

	if (!createDescriptors(self, error)) {
		return false;
	}

	if (!createPipeline(self, error)) {
		return false;
	}

	if (!createPipeline(self, error)) {
		return false;
	}

	return true;
}

static bool createTexture(Titlebar self, char **error)
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

static bool createTextureSampler(Titlebar self, char **error)
{
	if (!createSampler(self->device, 0, PIECES_TEXTURE_MIP_LEVELS, &self->sampler, error)) {
		return false;
	}

	return true;
}

static bool createDescriptors(Titlebar self, char **error)
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

static bool createPipeline(Titlebar self, char **error)
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
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.offset = 0,
		.size = sizeof(ChessPiecePushConstants)
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
		.pushConstantRangeCount = 1,
		.pushConstantRanges = &pushConstantRange,
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

bool drawTitlebar(Titlebar self, VkCommandBuffer commandBuffer, char **error)
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

			float diffuseColor[3];
			float ambientColor[3];
			if (self->board[i] >= BLACK_PAWN && self->board[i] <= BLACK_KING) {
				diffuseColor[0] = 0.3f;
				diffuseColor[1] = 0.3f;
				diffuseColor[2] = 0.3f;
				ambientColor[0] = 0.03f;
				ambientColor[1] = 0.03f;
				ambientColor[2] = 0.03f;
			} else if (self->board[i] >= WHITE_PAWN && self->board[i] <= WHITE_KING) {
				diffuseColor[0] = 0.9f;
				diffuseColor[1] = 0.9f;
				diffuseColor[2] = 0.9f;
				ambientColor[0] = 0.09f;
				ambientColor[1] = 0.09f;
				ambientColor[2] = 0.09f;
			}
			srgbToLinear(diffuseColor);
			srgbToLinear(ambientColor);

			ChessPiecePushConstants pushConstants = {
				.diffuseColor = {diffuseColor[0], diffuseColor[1], diffuseColor[2], 0.0f},
				.ambientColor = {ambientColor[0], ambientColor[1], ambientColor[2], 0.0f}
			};

			size_t meshIndex = pieceMeshIndexMap[self->board[i]];
			uint32_t piecesUniformBufferOffset = sizeof(self->piecesUniforms[0]) * i;

			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, self->piecesPipelineLayout, 0, 1, &self->piecesDescriptorSets[0], 1, &piecesUniformBufferOffset);
			vkCmdPushConstants(commandBuffer, self->piecesPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);
			vkCmdDrawIndexed(commandBuffer, self->pieceVertexCounts[meshIndex], 1, self->pieceVertexOffsets[meshIndex], self->pieceVertexOffsets[meshIndex], 0);
		}
	}

	return true;
}

void titlebarHandleInputEvent(void *titlebar, InputEvent *inputEvent)
{
	Titlebar self = (Titlebar) titlebar;
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

void destroyTitlebar(Titlebar self)
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
