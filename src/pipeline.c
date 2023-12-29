#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "swapchain.h"
#include "utils.h"
#include "vulkan_utils.h"

#include "pipeline.h"

bool createPipeline(VkDevice device, VkRenderPass renderPass, char *resourcePath, SwapchainInfo swapchainInfo, VkPipelineLayout *pipelineLayout, VkPipeline *pipeline, char **error)
{
	VkResult result;

#ifndef EMBED_SHADERS
	char *vertexShaderPath;
	char *fragmentShaderPath;
	asprintf(&vertexShaderPath, "%s/%s", resourcePath, "vert.spv");
	asprintf(&fragmentShaderPath, "%s/%s", resourcePath, "frag.spv");
	char *vertexShaderBytes;
	char *fragmentShaderBytes;
	uint32_t vertexShaderSize = 0;
	uint32_t fragmentShaderSize = 0;

	if ((vertexShaderSize = readFileToString(vertexShaderPath, &vertexShaderBytes)) == -1) {
		asprintf(error, "Failed to open vertex shader for reading.\n");
		return false;
	}
	if ((fragmentShaderSize = readFileToString(fragmentShaderPath, &fragmentShaderBytes)) == -1) {
		asprintf(error, "Failed to open fragment shader for reading.\n");
		return false;
	}
#endif /* EMBED_SHADERS */

	VkShaderModuleCreateInfo vert_shad_mod_cre_info;
	vert_shad_mod_cre_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	vert_shad_mod_cre_info.pNext = NULL;
	vert_shad_mod_cre_info.flags = 0;
	vert_shad_mod_cre_info.codeSize = vertexShaderSize;
	vert_shad_mod_cre_info.pCode = (const uint32_t *) vertexShaderBytes;

	VkShaderModuleCreateInfo frag_shad_mod_cre_info;
	frag_shad_mod_cre_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	frag_shad_mod_cre_info.pNext = NULL;
	frag_shad_mod_cre_info.flags = 0;
	frag_shad_mod_cre_info.codeSize = fragmentShaderSize;
	frag_shad_mod_cre_info.pCode = (const uint32_t *) fragmentShaderBytes;

	VkShaderModule vert_shad_mod;
	VkShaderModule frag_shad_mod;
	vkCreateShaderModule(device, &vert_shad_mod_cre_info, NULL, &vert_shad_mod);
	vkCreateShaderModule(device, &frag_shad_mod_cre_info, NULL, &frag_shad_mod);

	VkPipelineShaderStageCreateInfo vert_shad_stage_cre_info, frag_shad_stage_cre_info, shad_stage_cre_infos[2];

	vert_shad_stage_cre_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_shad_stage_cre_info.pNext = NULL;
	vert_shad_stage_cre_info.flags = 0;
	vert_shad_stage_cre_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_shad_stage_cre_info.module = vert_shad_mod;
	char vert_entry
	[VK_MAX_EXTENSION_NAME_SIZE];
	strcpy(vert_entry, "main");
	vert_shad_stage_cre_info.pName = vert_entry;
	vert_shad_stage_cre_info.pSpecializationInfo = NULL;

	frag_shad_stage_cre_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_shad_stage_cre_info.pNext = NULL;
	frag_shad_stage_cre_info.flags = 0;
	frag_shad_stage_cre_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_shad_stage_cre_info.module = frag_shad_mod;
	char frag_entry
	[VK_MAX_EXTENSION_NAME_SIZE];
	strcpy(frag_entry, "main");
	frag_shad_stage_cre_info.pName = frag_entry;
	frag_shad_stage_cre_info.pSpecializationInfo = NULL;

	shad_stage_cre_infos[0] = vert_shad_stage_cre_info;
	shad_stage_cre_infos[1] = frag_shad_stage_cre_info;

	VkPipelineVertexInputStateCreateInfo vert_input_info;
	vert_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vert_input_info.pNext = NULL;
	vert_input_info.flags = 0;
	vert_input_info.vertexBindingDescriptionCount = 0;
	vert_input_info.pVertexBindingDescriptions = NULL;
	vert_input_info.vertexAttributeDescriptionCount = 0;
	vert_input_info.pVertexAttributeDescriptions = NULL;

	VkPipelineInputAssemblyStateCreateInfo input_asm_info;
	input_asm_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_asm_info.pNext = NULL;
	input_asm_info.flags = 0;
	input_asm_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_asm_info.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport;
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = swapchainInfo.extent.width;
	viewport.height = swapchainInfo.extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor;
	VkOffset2D sci_offset;
	sci_offset.x = 0;
	sci_offset.y = 0;
	scissor.offset = sci_offset;
	scissor.extent = swapchainInfo.extent;

	VkPipelineViewportStateCreateInfo vwp_state_cre_info;
	vwp_state_cre_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	vwp_state_cre_info.pNext = NULL;
	vwp_state_cre_info.flags = 0;
	vwp_state_cre_info.viewportCount = 1;
	vwp_state_cre_info.pViewports = &viewport;
	vwp_state_cre_info.scissorCount = 1;
	vwp_state_cre_info.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rast_cre_info;
	rast_cre_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rast_cre_info.pNext = NULL;
	rast_cre_info.flags = 0;
	rast_cre_info.depthClampEnable = VK_FALSE;
	rast_cre_info.rasterizerDiscardEnable = VK_FALSE;
	rast_cre_info.polygonMode = VK_POLYGON_MODE_FILL;
	rast_cre_info.cullMode = VK_CULL_MODE_BACK_BIT;
	rast_cre_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rast_cre_info.depthBiasEnable = VK_FALSE;
	rast_cre_info.depthBiasConstantFactor = 0.0f;
	rast_cre_info.depthBiasClamp = 0.0f;
	rast_cre_info.depthBiasSlopeFactor = 0.0f;
	rast_cre_info.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo mul_sam_cre_info;
	mul_sam_cre_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	mul_sam_cre_info.pNext = NULL;
	mul_sam_cre_info.flags = 0;
	mul_sam_cre_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	mul_sam_cre_info.sampleShadingEnable = VK_FALSE;
	mul_sam_cre_info.minSampleShading = 1.0f;
	mul_sam_cre_info.pSampleMask = NULL;
	mul_sam_cre_info.alphaToCoverageEnable = VK_FALSE;
	mul_sam_cre_info.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState color_blend_attach;
	color_blend_attach.blendEnable = VK_FALSE;
	color_blend_attach.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attach.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_blend_attach.colorBlendOp = VK_BLEND_OP_ADD;
	color_blend_attach.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attach.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_blend_attach.alphaBlendOp = VK_BLEND_OP_ADD;
	color_blend_attach.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo color_blend_cre_info;
	color_blend_cre_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend_cre_info.pNext = NULL;
	color_blend_cre_info.flags = 0;
	color_blend_cre_info.logicOpEnable = VK_FALSE;
	color_blend_cre_info.logicOp = VK_LOGIC_OP_COPY;
	color_blend_cre_info.attachmentCount = 1;
	color_blend_cre_info.pAttachments = &color_blend_attach;
	for (uint32_t i = 0; i < 4; i++) {
		color_blend_cre_info.blendConstants[i] = 0.0f;
	}

	VkPipelineLayoutCreateInfo pipe_lay_cre_info;
	pipe_lay_cre_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipe_lay_cre_info.pNext = NULL;
	pipe_lay_cre_info.flags = 0;
	pipe_lay_cre_info.setLayoutCount = 0;
	pipe_lay_cre_info.pSetLayouts = NULL;
	pipe_lay_cre_info.pushConstantRangeCount = 0;
	pipe_lay_cre_info.pPushConstantRanges = NULL;

	if ((result = vkCreatePipelineLayout(device, &pipe_lay_cre_info, NULL, pipelineLayout)) != VK_SUCCESS) {
		asprintf(error, "Failed to create pipeline layout: %s", string_VkResult(result));
		return false;
	}

	VkGraphicsPipelineCreateInfo pipe_cre_info;
	pipe_cre_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipe_cre_info.pNext = NULL;
	pipe_cre_info.flags = 0;
	pipe_cre_info.stageCount = 2;
	pipe_cre_info.pStages = shad_stage_cre_infos;
	pipe_cre_info.pVertexInputState = &vert_input_info;
	pipe_cre_info.pInputAssemblyState = &input_asm_info;
	pipe_cre_info.pTessellationState = NULL;
	pipe_cre_info.pViewportState = &vwp_state_cre_info;
	pipe_cre_info.pRasterizationState = &rast_cre_info;
	pipe_cre_info.pMultisampleState = &mul_sam_cre_info;
	pipe_cre_info.pDepthStencilState = NULL;
	pipe_cre_info.pColorBlendState = &color_blend_cre_info;
	pipe_cre_info.pDynamicState = NULL;

	pipe_cre_info.layout = *pipelineLayout;
	pipe_cre_info.renderPass = renderPass;
	pipe_cre_info.subpass = 0;
	pipe_cre_info.basePipelineHandle = VK_NULL_HANDLE;
	pipe_cre_info.basePipelineIndex = -1;

	if ((result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipe_cre_info, NULL, pipeline)) != VK_SUCCESS) {
		asprintf(error, "Failed to create pipeline: %s", string_VkResult(result));
		return false;
	}

	vkDestroyShaderModule(device, frag_shad_mod, NULL);
	vkDestroyShaderModule(device, vert_shad_mod, NULL);
#ifndef EMBED_SHADERS
	free(fragmentShaderBytes);
	free(vertexShaderBytes);
#endif /* EMBED_SHADERS */

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