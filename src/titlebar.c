#include <stdlib.h>

#include "lodepng.h"

#include "titlebar.h"
#include "buffer.h"
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

#define TITLEBAR_VERTEX_COUNT 5 * 4
#define TITLEBAR_INDEX_COUNT 5 * 6

static const float VIEWPORT_WIDTH = 2.0f;
static const float VIEWPORT_HEIGHT = 2.0f;

typedef struct titlebar_push_constants_t {
	float aspectRatio;
} TitlebarPushConstants;

typedef enum action_t {
	TITLEBAR_ACTION_CLOSE,
	TITLEBAR_ACTION_MAXIMIZE,
	TITLEBAR_ACTION_MINIMIZE,
	TITLEBAR_ACTION_MENU
} TITLEBAR_ACTION;

float iconSpriteOriginMap[4][2] = {
	{0.0f, 0.0f},
	{0.5f, 0.0f},
	{0.0f, 0.5f},
	{0.5f, 0.5f}
};

typedef struct board_vertex_t {
	float pos[2];
	float color[3];
	float texCoord[2];
} TitlebarVertex;

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
	bool hoveringMenu;
	bool hoveringMinimize;
	bool hoveringMaximize;
	bool hoveringClose;
	bool pressedMenu;
	bool pressedMinimize;
	bool pressedMaximize;
	bool pressedClose;
	void (*close)(void *);
	void *closeArg;
	void (*maximize)(void *);
	void *maximizeArg;
	void (*minimize)(void *);
	void *minimizeArg;
	VkRect2D buttonRectangles[4];
	TitlebarVertex vertices[TITLEBAR_VERTEX_COUNT];
	VkBuffer vertexBuffer;
	VmaAllocation vertexBufferAllocation;
	void *vertexBufferMappedMemory;
	uint16_t indices[TITLEBAR_INDEX_COUNT];
	VkBuffer indexBuffer;
	VmaAllocation indexBufferAllocation;
	void *indexBufferMappedMemory;
};

static bool createTitlebarTexture(Titlebar self, char **error);
static bool createTitlebarTextureSampler(Titlebar self, char **error);
static bool createTitlebarDescriptors(Titlebar self, char **error);
static void updateMesh(Titlebar self);
static bool createVertexBuffer(Titlebar self, char **error);
static void updateVertexBuffer(Titlebar self);
static void updateIndices(Titlebar self);
static bool createIndexBuffer(Titlebar self, char **error);
static void updateIndexBuffer(Titlebar self);
static bool createTitlebarPipeline(Titlebar self, char **error);
static void updateHovering(Titlebar self);
static void updatePressed(Titlebar self);

bool createTitlebar(Titlebar *titlebar, VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, VkRenderPass renderPass, uint32_t subpass, VkSampleCountFlagBits sampleCount, const char *resourcePath, float aspectRatio, void (*close)(void *), void *closeArg, void (*maximize)(void *), void *maximizeArg, void (*minimize)(void *), void *minimizeArg, char **error)
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
	self->hoveringMenu = false;
	self->hoveringMinimize = false;
	self->hoveringMaximize = false;
	self->hoveringClose = false;
	self->pressedMenu = false;
	self->pressedMinimize = false;
	self->pressedMaximize = false;
	self->hoveringClose = false;
	self->close = close;
	self->closeArg = closeArg;
	self->maximize = maximize;
	self->maximizeArg = maximizeArg;
	self->minimize = minimize;
	self->minimizeArg = minimizeArg;

	updateMesh(self);
	updateIndices(self);

	if (!createVertexBuffer(self, error)) {
		return false;
	}

	if (!createIndexBuffer(self, error)) {
		return false;
	}

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

	return true;
}

static bool createTitlebarTexture(Titlebar self, char **error)
{
#ifndef EMBED_TEXTURES
	char *texturePath;
	asprintf(&texturePath, "%s/%s", self->resourcePath, "titlebar.png");
	char *titlebarTextureBytes;
	uint32_t titlebarTextureSize = 0;

	if ((titlebarTextureSize = readFileToString(texturePath, &titlebarTextureBytes)) == -1) {
		asprintf(error, "Failed to open texture for reading.\n");
		return false;
	}
#endif /* EMBED_TEXTURES */

	unsigned lodepngResult;
	unsigned char *textureDecodedBytes;
	unsigned textureDecodedWidth;
	unsigned textureDecodedHeight;

	if (lodepngResult = lodepng_decode32(&textureDecodedBytes, &textureDecodedWidth, &textureDecodedHeight, titlebarTextureBytes, titlebarTextureSize)) {
		asprintf(error, "Failed to decode PNG: %s\n", lodepng_error_text(lodepngResult));
		return false;
	}

	VkExtent2D textureExtent = {
		.width = textureDecodedHeight,
		.height = textureDecodedHeight
	};
	unsigned textureDecodedSize = (textureDecodedWidth * textureDecodedHeight) * (32 / sizeof(textureDecodedBytes));

#ifndef EMBED_TEXTURES
	free(titlebarTextureBytes);
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

static void updateButtonRectangles(Titlebar self)
{
	float buttonHeight = 2.0f;
	float buttonWidth = 2.0f;

	self->buttonRectangles[TITLEBAR_ACTION_CLOSE] = (VkRect2D) {
		.offset = {
			.x = 1.0 - buttonWidth,
			.y = 1.0 - buttonHeight
		},
		.extent = {
			.width = buttonWidth,
			.height = buttonHeight
		}
	};

	self->buttonRectangles[TITLEBAR_ACTION_MAXIMIZE] = (VkRect2D) {
		.offset = {
			.x = 1.0 - buttonWidth * 2,
			.y = 1.0 - buttonHeight
		},
		.extent = {
			.width = buttonWidth,
			.height = buttonHeight
		}
	};

	self->buttonRectangles[TITLEBAR_ACTION_MINIMIZE] = (VkRect2D) {
		.offset = {
			.x = 1.0 - buttonWidth * 3,
			.y = 1.0 - buttonHeight
		},
		.extent = {
			.width = buttonWidth,
			.height = buttonHeight
		}
	};

	self->buttonRectangles[TITLEBAR_ACTION_MENU] = (VkRect2D) {
		.offset = {
			.x = 1.0 - buttonWidth * 4,
			.y = 1.0 - buttonHeight
		},
		.extent = {
			.width = buttonWidth,
			.height = buttonHeight
		}
	};
}

static void updateMesh(Titlebar self)
{
	float hoverColor[] = {1.0f, 1.0f, 1.0f, 0.01f};
	float pressedColor[] = {1.0f, 1.0f, 1.0f, 0.05f};

	float buttonHeight = 2.0f;
	float buttonWidth = 2.0f;

	float barColor[] = {0.0f, 0.5f, 0.5f};
	srgbToLinear(barColor);
	float defaultBackground[] = {0.71f, 0.533f, 0.388f};
	srgbToLinear(defaultBackground);
	float hoverBackground[] = {0.71f, 0.533f, 0.388f};
	srgbToLinear(hoverBackground);

	bool hovering[] = {self->hoveringClose, self->hoveringMaximize, self->hoveringMinimize, self->hoveringMenu};
	float buttonColors[4][3];
	for (size_t i = 0; i <= TITLEBAR_ACTION_MENU; ++i) {
		if (hovering[i]) {
			buttonColors[i][0] = hoverBackground[0];
			buttonColors[i][1] = hoverBackground[1];
			buttonColors[i][2] = hoverBackground[2];
		} else {
			buttonColors[i][0] = defaultBackground[0];
			buttonColors[i][0] = defaultBackground[1];
			buttonColors[i][0] = defaultBackground[2];
		}
	}

	self->vertices[0] = (TitlebarVertex) {{-1.0f, -1.0f}, {barColor[0], barColor[1], barColor[2]}, {0.0f, 0.0f}};
	self->vertices[1] = (TitlebarVertex) {{-1.0f, 1.0f}, {barColor[0], barColor[1], barColor[2]}, {0.0f, 0.0f}};
	self->vertices[2] = (TitlebarVertex) {{1.0f, 1.0f}, {barColor[0], barColor[1], barColor[2]}, {0.0f, 0.0f}};
	self->vertices[3] = (TitlebarVertex) {{1.0f, -1.0f}, {barColor[0], barColor[1], barColor[2]}, {0.0f, 0.0f}};
	for (size_t i = 0; i <= TITLEBAR_ACTION_MENU; ++i) {
		self->vertices[(i + 1) * 4] = (TitlebarVertex) {
			{self->buttonRectangles[i].offset.x, self->buttonRectangles[i].offset.y},
			{buttonColors[i][0], buttonColors[i][1], buttonColors[i][2]},
			{iconSpriteOriginMap[i][0], iconSpriteOriginMap[i][1]}
		};
		self->vertices[(i + 1) * 4 + 1] = (TitlebarVertex) {
			{self->buttonRectangles[i].offset.x, self->buttonRectangles[i].offset.y + buttonHeight},
			{buttonColors[i][0], buttonColors[i][1], buttonColors[i][2]},
			{iconSpriteOriginMap[i][0], iconSpriteOriginMap[i][1] + 0.5f}
		};
		self->vertices[(i + 1) * 4 + 2] = (TitlebarVertex) {
			{self->buttonRectangles[i].offset.x + buttonWidth, self->buttonRectangles[i].offset.y + buttonHeight},
			{buttonColors[i][0], buttonColors[i][1], buttonColors[i][2]},
			{iconSpriteOriginMap[i][0] + 0.5f, iconSpriteOriginMap[i][1] + 0.5f}
		};
		self->vertices[(i + 1) * 4 + 3] = (TitlebarVertex) {
			{self->buttonRectangles[i].offset.x + buttonWidth, self->buttonRectangles[i].offset.y},
			{buttonColors[i][0], buttonColors[i][1], buttonColors[i][2]},
			{iconSpriteOriginMap[i][0] + 0.5f, iconSpriteOriginMap[i][1]}
		};
	}
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

static bool createVertexBuffer(Titlebar self, char **error)
{
	if (!createHostVisibleMutableBuffer(self->device, self->allocator, self->commandPool, self->queue, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &self->vertexBufferMappedMemory, &self->vertexBuffer, &self->vertexBufferAllocation, self->vertices, TITLEBAR_VERTEX_COUNT, sizeof(*self->vertices), error)) {
		return false;
	}

	return true;
}

static void updateVertexBuffer(Titlebar self)
{
	updateHostVisibleMutableBuffer(self->device, self->vertexBufferMappedMemory, &self->vertexBuffer, TITLEBAR_VERTEX_COUNT, sizeof(*self->vertices));
}

static void updateIndices(Titlebar self)
{
	for (size_t i = 0; i < 5; ++i) {
		size_t verticesOffset = i * 4;
		size_t indicesOffset = i * 6;

		self->indices[indicesOffset] = verticesOffset + 0;
		self->indices[indicesOffset + 1] = verticesOffset + 1;
		self->indices[indicesOffset + 2] = verticesOffset + 2;
		self->indices[indicesOffset + 3] = verticesOffset + 0;
		self->indices[indicesOffset + 4] = verticesOffset + 2;
		self->indices[indicesOffset + 5] = verticesOffset + 3;
	}
}

static bool createIndexBuffer(Titlebar self, char **error)
{
	if (!createHostVisibleMutableBuffer(self->device, self->allocator, self->commandPool, self->queue, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &self->indexBufferMappedMemory, &self->indexBuffer, &self->indexBufferAllocation, self->indices, TITLEBAR_INDEX_COUNT, sizeof(*self->indices), error)) {
		return false;
	}

	return true;
}

static void updateIndexBuffer(Titlebar self)
{
	updateHostVisibleMutableBuffer(self->device, self->indexBufferMappedMemory, &self->indexBuffer, TITLEBAR_INDEX_COUNT, sizeof(*self->indices));
}

static bool createTitlebarPipeline(Titlebar self, char **error)
{
	VkVertexInputBindingDescription vertexBindingDescription = {
		.binding = 0,
		.stride = sizeof(TitlebarVertex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	};

	VkVertexInputAttributeDescription vertexAttributeDescriptions[] = {
		{
			.binding = 0,
			.location = 0,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = offsetof(TitlebarVertex, pos),
		}, {
			.binding = 0,
			.location = 1,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(TitlebarVertex, color),
		}, {
			.binding = 0,
			.location = 2,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = offsetof(TitlebarVertex, texCoord),
		}
	};

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
		.vertexBindingDescriptionCount = 1,
		.vertexBindingDescriptions = &vertexBindingDescription,
		.vertexAttributeDescriptionCount = sizeof(vertexAttributeDescriptions) / sizeof(*vertexAttributeDescriptions),
		.VertexAttributeDescriptions = vertexAttributeDescriptions,
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
	self->hoveringMenu = false;

	if (self->pointerPosition.x >= 1.0f - (1.0f / self->aspectRatio)) {
		self->hoveringClose = true;
	} else if (self->pointerPosition.x >= 1.0f - (2.0f / self->aspectRatio)) {
		self->hoveringMaximize = true;
	} else if (self->pointerPosition.x >= 1.0f - (3.0f / self->aspectRatio)) {
		self->hoveringMinimize = true;
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
		self->maximize(self->maximizeArg);
	} else if (self->pressedMinimize) {
		self->minimize(self->minimizeArg);
	}

	self->pressedClose = false;
	self->pressedMaximize = false;
	self->pressedMinimize = false;
}

bool drawTitlebar(Titlebar self, VkCommandBuffer commandBuffer, char **error)
{
	TitlebarPushConstants pushConstants = {
		.aspectRatio = self->aspectRatio
	};

	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &self->vertexBuffer, offsets);
	vkCmdBindIndexBuffer(commandBuffer, self->indexBuffer, 0, VK_INDEX_TYPE_UINT16);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, self->pipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, self->pipelineLayout, 0, 1, self->descriptorSets, 0, NULL);
	vkCmdPushConstants(commandBuffer, self->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);
	vkCmdDrawIndexed(commandBuffer, TITLEBAR_INDEX_COUNT, 1, 0, 0, 0);

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
	updateVertexBuffer(self);
}
