#include <stdlib.h>

#include "swapchain.h"
#include "utils.h"
#include "vulkan_utils.h"

#include "pipeline.h"

bool createPipeline(VkDevice device, VkRenderPass renderPass, uint32_t subpassIndex, const char *vertexShaderBytes, long vertexShaderSize, const char *fragmentShaderBytes, long fragmentShaderSize, VkExtent2D extent, VkDescriptorSetLayout *descriptorSetLayouts, uint32_t descriptorSetLayoutCount, VkPipelineLayout *pipelineLayout, VkPipeline *pipeline, char **error)
{
	VkResult result;

	VkShaderModuleCreateInfo vertexShaderModuleCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.codeSize = vertexShaderSize,
		.pCode = (const uint32_t *) vertexShaderBytes
	};

	VkShaderModuleCreateInfo fragmentShaderModuleCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.codeSize = fragmentShaderSize,
		.pCode = (const uint32_t *) fragmentShaderBytes
	};

	VkShaderModule vertexShaderModule;
	VkShaderModule fragmentShaderModule;
	vkCreateShaderModule(device, &vertexShaderModuleCreateInfo, NULL, &vertexShaderModule);
	vkCreateShaderModule(device, &fragmentShaderModuleCreateInfo, NULL, &fragmentShaderModule);

	VkPipelineShaderStageCreateInfo vertexShaderStageCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,	
		.module = vertexShaderModule,
		.pName = "main",
		.pSpecializationInfo = NULL
	};

	VkPipelineShaderStageCreateInfo fragmentShaderStageCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = fragmentShaderModule,
		.pName = "main",
		.pSpecializationInfo = NULL
	};

	VkPipelineShaderStageCreateInfo shaderStageCreateInfos[] = {
		vertexShaderStageCreateInfo,
		fragmentShaderStageCreateInfo
	};

	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.vertexBindingDescriptionCount = 0,
		.pVertexBindingDescriptions = NULL,
		.vertexAttributeDescriptionCount = 0,
		.pVertexAttributeDescriptions = NULL
	};

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
	};

	VkViewport viewport = {
		.x = 0.0f,
		.y = 0.0f,
		.width = extent.width,
		.height = extent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	VkOffset2D scissorOffset = {
		.x = 0,
		.y = 0
	};

	VkRect2D scissor = {
		.offset = scissorOffset,
		.extent = extent
	};

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor
	};

	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0.0f,
		.depthBiasClamp = 0.0f,
		.depthBiasSlopeFactor = 0.0f,
		.lineWidth = 1.0f
	};

	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 1.0f,
		.pSampleMask = NULL,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE
	};

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {
		.blendEnable = VK_FALSE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};

	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachmentState,
	};
	for (uint32_t i = 0; i < 4; i++) {
		colorBlendStateCreateInfo.blendConstants[i] = 0.0f;
	}

	VkPushConstantRange pushConstantRange = {
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.offset = 0,
		.size = sizeof(PushConstants)
	};

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.setLayoutCount = descriptorSetLayoutCount,
		.pSetLayouts = descriptorSetLayouts,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &pushConstantRange
	};

	if ((result = vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, NULL, pipelineLayout)) != VK_SUCCESS) {
		asprintf(error, "Failed to create pipeline layout: %s", string_VkResult(result));
		return false;
	}

	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.dynamicStateCount = 2,
		.pDynamicStates = dynamicStates
	};

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.stageCount = 2,
		.pStages = shaderStageCreateInfos,
		.pVertexInputState = &vertexInputStateCreateInfo,
		.pInputAssemblyState = &inputAssemblyStateCreateInfo,
		.pTessellationState = NULL,
		.pViewportState = &viewportStateCreateInfo,
		.pRasterizationState = &rasterizationStateCreateInfo,
		.pMultisampleState = &multisampleStateCreateInfo,
		.pDepthStencilState = NULL,
		.pColorBlendState = &colorBlendStateCreateInfo,
		.pDynamicState = &pipelineDynamicStateCreateInfo,
		.layout = *pipelineLayout,
		.renderPass = renderPass,
		.subpass = subpassIndex,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1
	};

	if ((result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, NULL, pipeline)) != VK_SUCCESS) {
		asprintf(error, "Failed to create pipeline: %s", string_VkResult(result));
		return false;
	}

	vkDestroyShaderModule(device, fragmentShaderModule, NULL);
	vkDestroyShaderModule(device, vertexShaderModule, NULL);

	return true;
}

void destroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout)
{
	vkDestroyPipelineLayout(device, pipelineLayout, NULL);
}

void destroyPipeline(VkDevice device, VkPipeline pipeline)
{
	vkDestroyPipeline(device, pipeline, NULL);
}