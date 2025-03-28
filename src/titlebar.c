#include <stdlib.h>

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
	float minimizeColor[4];
	float maximizeColor[4];
	float closeColor[4];
	float height;
} TitlebarPushConstants;

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

static bool createTitlebarTexture(Titlebar self, char **error);
static bool createTitlebarTextureSampler(Titlebar self, char **error);
static bool createTitlebarDescriptors(Titlebar self, char **error);
static bool createTitlebarPipeline(Titlebar self, char **error);

bool createTitlebar(Titlebar *titlebar, VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, VkRenderPass renderPass, uint32_t subpass, VkSampleCountFlagBits sampleCount, const char *resourcePath, char **error)
{
	*titlebar = malloc(sizeof(**titlebar));

	Titlebar self = *titlebar;

	self->device = device;
	self->allocator = allocator;
	self->commandPool = commandPool;
	self->queue = queue;
	self->renderPass = renderPass;
	self->subpass = subpass;
	self->sampleCount = sampleCount;
	self->resourcePath = resourcePath;

	if (!createTitlebarTexture(self, error)) {
		return false;
	}

	if (!createTitlebarTextureSampler(self, error)) {
		return false;
	}

	if (!createTitlebarDescriptors(self, error)) {
		return false;
	}

	if (!createTitlebarPipeline(self, error)) {
		return false;
	}

	if (!createTitlebarPipeline(self, error)) {
		return false;
	}

	return true;
}

static bool createTitlebarTexture(Titlebar self, char **error)
{
#if 0
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

#endif
	return true;
}

static bool createTitlebarTextureSampler(Titlebar self, char **error)
{
#if 0
	if (!createSampler(self->device, 0, PIECES_TEXTURE_MIP_LEVELS, &self->sampler, error)) {
		return false;
	}

#endif
	return true;
}

static bool createTitlebarDescriptors(Titlebar self, char **error)
{
#if 0
	VkDescriptorImageInfo imageDescriptorInfo = {
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.imageView = self->textureImageView,
		.sampler = self->sampler
	};

	VkDescriptorSetLayoutBinding imageBinding = {
		.binding = 1,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = NULL
	};

	void *boardDescriptorSetDescriptorInfos[] = {&imageDescriptorInfo};
	VkDescriptorSetLayoutBinding boardDescriptorSetBindings[] = {imageBinding};
	CreateDescriptorSetInfo createDescriptorSetInfos[] = {
		{
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.descriptorInfos = boardDescriptorSetDescriptorInfos,
			.descriptorCount = 1,
			.bindings = boardDescriptorSetBindings,
			.bindingCount = 1
		}
	};
	VkDescriptorSet descriptorSet;
	VkDescriptorSetLayout descriptorSetLayout;
	if (!createDescriptorSets(self->device, createDescriptorSetInfos, 2, &self->descriptorPool, &descriptorSet, &descriptorSetLayout, error)) {
		return false;
	}
	self->descriptorSets[0] = descriptorSet;
	self->descriptorSetLayouts[0] = descriptorSetLayout;

#endif
	return true;
}

static bool createTitlebarPipeline(Titlebar self, char **error)
{
#ifndef EMBED_SHADERS
	char *titlebarVertShaderPath;
	char *titlebarFragShaderPath;
	asprintf(&titlebarVertShaderPath, "%s/%s", self->resourcePath, "titlebar.vert.spv");
	asprintf(&titlebarFragShaderPath, "%s/%s", self->resourcePath, "titlebar.frag.spv");
	char *titlebarVertShaderBytes;
	char *titlebarFragShaderBytes;
	uint32_t titlebarVertShaderSize = 0;
	uint32_t titlebarFragShaderSize = 0;

	if ((titlebarVertShaderSize = readFileToString(titlebarVertShaderPath, &titlebarVertShaderBytes)) == -1) {
		asprintf(error, "Failed to open titlebar vertex shader for reading.\n");
		return false;
	}
	if ((titlebarFragShaderSize = readFileToString(titlebarFragShaderPath, &titlebarFragShaderBytes)) == -1) {
		asprintf(error, "Failed to open titlebar fragment shader for reading.\n");
		return false;
	}
#endif /* EMBED_SHADERS */

	VkPushConstantRange pushConstantRange = {
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.offset = 0,
		.size = sizeof(TitlebarPushConstants)
	};

	VkPipelineDepthStencilStateCreateInfo depthStencilState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_FALSE,
		.depthWriteEnable = VK_FALSE,
		.depthCompareOp = VK_COMPARE_OP_NEVER,
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
		.vertexShaderBytes = titlebarVertShaderBytes,
		.vertexShaderSize = titlebarVertShaderSize,
		.fragmentShaderBytes = titlebarFragShaderBytes,
		.fragmentShaderSize = titlebarFragShaderSize,
		.vertexBindingDescriptionCount = 0,
		.vertexBindingDescriptions = NULL,
		.vertexAttributeDescriptionCount = 0,
		.VertexAttributeDescriptions = NULL,
		// .descriptorSetLayouts = self->descriptorSetLayouts,
		// .descriptorSetLayoutCount = 1,
		.descriptorSetLayouts = NULL,
		.descriptorSetLayoutCount = 0,
		.pushConstantRangeCount = 1,
		.pushConstantRanges = &pushConstantRange,
		.depthStencilState = depthStencilState,
		.sampleCount = self->sampleCount
	};
	bool pipelineCreateSuccess = createPipeline(pipelineCreateInfo, &self->pipelineLayout, &self->pipeline, true, error);
#ifndef EMBED_SHADERS
	free(titlebarFragShaderBytes);
	free(titlebarVertShaderBytes);
#endif /* EMBED_SHADERS */
	if (!pipelineCreateSuccess) {
		return false;
	}

	return true;
}

bool drawTitlebar(Titlebar self, VkCommandBuffer commandBuffer, char **error)
{
	TitlebarPushConstants pushConstants = {
		.minimizeColor = {1.0f, 0.0f, 0.0f, 0.0f},
		.maximizeColor = {1.0f, 0.0f, 0.0f, 0.0f},
		.closeColor = {1.0f, 0.0f, 0.0f, 0.0f},
		.height = 0.1f
	};

	VkDeviceSize offsets[] = {0};
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, self->pipeline);
	//vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, self->pipelineLayout, 0, 1, self->descriptorSets, 0, NULL);
	vkCmdPushConstants(commandBuffer, self->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);
	vkCmdDraw(commandBuffer, 24, 1, 0, 0);

	return true;
}

void titlebarHandleInputEvent(void *titlebar, InputEvent *inputEvent)
{
	Titlebar self = (Titlebar) titlebar;

	switch(inputEvent->type) {
	case POINTER_LEAVE:
		break;
	case BUTTON_DOWN:
		break;
	case BUTTON_UP:
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
	destroyPipeline(self->device, self->pipeline);
	destroyPipelineLayout(self->device, self->pipelineLayout);
	destroyDescriptorPool(self->device, self->descriptorPool);
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		destroyDescriptorSetLayout(self->device, self->descriptorSetLayouts[i]);
	}
	destroySampler(self->device, self->sampler);
	destroyImageView(self->device, self->textureImageView);
	destroyImage(self->allocator, self->textureImage, self->textureImageAllocation);
	free(self);
}
