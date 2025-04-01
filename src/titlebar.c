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
#include "matrix_utils.h"

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
	float aspectRatio;
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
	float aspectRatio;
	float height;
	bool hoveringMinimize;
	bool hoveringMaximize;
	bool hoveringClose;
	bool pressedMinimize;
	bool pressedMaximize;
	bool pressedClose;
	void (*close)(void *);
	void *closeArg;
};

static bool createTitlebarTexture(Titlebar self, char **error);
static bool createTitlebarTextureSampler(Titlebar self, char **error);
static bool createTitlebarDescriptors(Titlebar self, char **error);
static bool createTitlebarPipeline(Titlebar self, char **error);
static void updateHovering(Titlebar self);
static void updatePressed(Titlebar self);

bool createTitlebar(Titlebar *titlebar, VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, VkRenderPass renderPass, uint32_t subpass, VkSampleCountFlagBits sampleCount, const char *resourcePath, float aspectRatio, float height, void (*close)(void *), void *closeArg, char **error)
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
	self->aspectRatio = aspectRatio;
	self->height = height;
	self->hoveringMinimize = false;
	self->hoveringMaximize = false;
	self->hoveringClose = false;
	self->close = close;
	self->closeArg = closeArg;

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
#ifndef EMBED_TEXTURES
	char *texturePath;
	asprintf(&texturePath, "%s/%s", self->resourcePath, "titlebar.png");
	char *textureBytes;
	uint32_t textureSize = 0;

	if ((textureSize = readFileToString(texturePath, &textureBytes)) == -1) {
		asprintf(error, "Failed to open texture for reading.\n");
		return false;
	}
#endif /* EMBED_TEXTURES */

	unsigned lodepngResult;
	unsigned char *textureDecodedBytes;
	unsigned textureDecodedWidth;
	unsigned textureDecodedHeight;

	if (lodepngResult = lodepng_decode32(&textureDecodedBytes, &textureDecodedWidth, &textureDecodedHeight, textureBytes, textureSize)) {
		asprintf(error, "Failed to decode PNG: %s\n", lodepng_error_text(lodepngResult));
		return false;
	}

	VkExtent2D textureExtent = {
		.width = textureDecodedHeight,
		.height = textureDecodedHeight
	};
	unsigned textureDecodedSize = (textureDecodedWidth * textureDecodedHeight) * (32 / sizeof(textureDecodedBytes));

#ifndef EMBED_TEXTURES
	free(textureBytes);
#endif /* EMBED_TEXTURES */

	VkBuffer stagingBuffer;
	VmaAllocation stagingBufferAllocation;
	if (!createBuffer(self->device, self->allocator, textureDecodedSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, 0, &stagingBuffer, &stagingBufferAllocation, error)) {
		return false;
	}

	VkResult result;
	void *data;
	if ((result = vmaMapMemory(self->allocator, stagingBufferAllocation, &data)) != VK_SUCCESS) {
		asprintf(error, "Failed to map memory: %s", string_VkResult(result));
		return false;
	}
	memcpy(data, textureDecodedBytes, textureDecodedSize);
	free(textureDecodedBytes);
	vmaUnmapMemory(self->allocator, stagingBufferAllocation);

	if (!createImage(self->device, self->allocator, textureExtent, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 1, VK_SAMPLE_COUNT_1_BIT, &self->textureImage, &self->textureImageAllocation, error)) {
		return false;
	}

	if (!transitionImageLayout(self->device, self->commandPool, self->queue, self->textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, error)) {
		return false;
	}

	if (!copyBufferToImage(self->device, self->commandPool, self->queue, stagingBuffer, self->textureImage, textureDecodedWidth, textureDecodedHeight, 1, error)) {
		return false;
	}

	if (!transitionImageLayout(self->device, self->commandPool, self->queue, self->textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, error)) {
		return false;
	}

	destroyBuffer(self->allocator, stagingBuffer, stagingBufferAllocation);

	if (!createImageView(self->device, self->textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, 1, &self->textureImageView, error)) {
		return false;
	}

	return true;
}

static bool createTitlebarTextureSampler(Titlebar self, char **error)
{
	if (!createSampler(self->device, 0, 1, &self->sampler, error)) {
		return false;
	}

	return true;
}

static bool createTitlebarDescriptors(Titlebar self, char **error)
{
	VkDescriptorImageInfo imageDescriptorInfo = {
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.imageView = self->textureImageView,
		.sampler = self->sampler
	};

	VkDescriptorSetLayoutBinding imageBinding = {
		.binding = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = NULL
	};

	void *descriptorSetDescriptorInfos[] = {&imageDescriptorInfo};
	CreateDescriptorSetInfo createDescriptorSetInfos[] = {
		{
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.descriptorInfos = descriptorSetDescriptorInfos,
			.descriptorCount = 1,
			.bindings = &imageBinding,
			.bindingCount = 1
		}
	};
	VkDescriptorSet descriptorSet;
	VkDescriptorSetLayout descriptorSetLayout;
	if (!createDescriptorSets(self->device, createDescriptorSetInfos, 1, &self->descriptorPool, &descriptorSet, &descriptorSetLayout, error)) {
		return false;
	}
	self->descriptorSets[0] = descriptorSet;
	self->descriptorSetLayouts[0] = descriptorSetLayout;

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
		.descriptorSetLayouts = self->descriptorSetLayouts,
		.descriptorSetLayoutCount = 1,
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

static void updateHovering(Titlebar self)
{
	self->hoveringClose = false;
	self->hoveringMaximize = false;
	self->hoveringMinimize = false;

	if (self->pointerPosition.y < self->height) {
		if (self->pointerPosition.x >= 1.0f - (self->height / self->aspectRatio)) {
			self->hoveringClose = true;
		} else if (self->pointerPosition.x >= 1.0f - (self->height * 2 / self->aspectRatio)) {
			self->hoveringMaximize = true;
		} else if (self->pointerPosition.x >= 1.0f - (self->height * 3 / self->aspectRatio)) {
			self->hoveringMinimize = true;
		}
	}
}

static void handleButtonDown(Titlebar self)
{
	self->pressedClose = false;
	self->pressedMaximize = false;
	self->pressedMinimize = false;

	if (self->hoveringClose) {
		self->pressedClose = true;
	} else if (self->hoveringMaximize) {
		self->pressedMaximize = true;
	} else if (self->hoveringMinimize) {
		self->pressedMinimize = true;
	}
}

static void handleButtonUp(Titlebar self)
{
	if (self->pressedClose) {
		self->close(self->closeArg);
	} else if (self->pressedMaximize) {
	} else if (self->pressedMinimize) {
	}

	self->pressedClose = false;
	self->pressedMaximize = false;
	self->pressedMinimize = false;
}

bool drawTitlebar(Titlebar self, VkCommandBuffer commandBuffer, char **error)
{
	float hoverColor[] = {1.0f, 1.0f, 1.0f, 0.01f};
	float pressedColor[] = {1.0f, 1.0f, 1.0f, 0.05f};

	TitlebarPushConstants pushConstants = {
		.minimizeColor = {0.0f, 0.0f, 0.0f, 0.0f},
		.maximizeColor = {0.0f, 0.0f, 0.0f, 0.0f},
		.closeColor = {0.0f, 0.0f, 0.0f, 0.0f},
		.aspectRatio = self->aspectRatio,
		.height = self->height * VIEWPORT_HEIGHT
	};

	if (self->pressedClose) {
		vec4Copy(pressedColor, pushConstants.closeColor);
	} else if (self->pressedMaximize) {
		vec4Copy(pressedColor, pushConstants.maximizeColor);
	} else if (self->pressedMinimize) {
		vec4Copy(pressedColor, pushConstants.minimizeColor);
	} else if (self->hoveringClose) {
		vec4Copy(hoverColor, pushConstants.closeColor);
	} else if (self->hoveringMaximize) {
		vec4Copy(hoverColor, pushConstants.maximizeColor);
	} else if (self->hoveringMinimize) {
		vec4Copy(hoverColor, pushConstants.minimizeColor);
	}

	VkDeviceSize offsets[] = {0};
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, self->pipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, self->pipelineLayout, 0, 1, self->descriptorSets, 0, NULL);
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
		handleButtonDown(self);
		break;
	case BUTTON_UP:
		handleButtonUp(self);
		break;
	case NORMALIZED_POINTER_MOVE:
		self->pointerPosition = *(NormalizedPointerPosition *) inputEvent->data;
		updateHovering(self);
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

void titlebarSetAspectRatio(Titlebar self, float aspectRatio)
{
	self->aspectRatio = aspectRatio;
}

void titlebarSetHeight(Titlebar self, float height)
{
	self->height = height;
}
