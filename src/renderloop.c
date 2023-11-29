#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __APPLE__
#elif defined _WIN32 || defined _WIN64
#include <windows.h>
#endif

#include "utils.h"

#include "renderloop.h"

void draw(VkDevice dev, VkSwapchainKHR swap, VkExtent2D windowExtent, VkQueue graphicsQueue, VkQueue presentationQueue, uint32_t graphicsQueueFamilyIndex, const char *resourcePath, Queue *inputQueue) {
//
//fetch image from swapchain
//
	uint32_t swap_image_count = 0;
	vkGetSwapchainImagesKHR(dev, swap, &swap_image_count, NULL);
	VkImage swap_images[swap_image_count];
	vkGetSwapchainImagesKHR(dev, swap, &swap_image_count, swap_images);
	printf("%d images fetched from swapchain.\n", swap_image_count);
//
//create image view
//
	VkImageView image_views[swap_image_count];
	VkImageViewCreateInfo image_view_cre_infos[swap_image_count];

	VkComponentMapping image_view_rgba_comp;
	image_view_rgba_comp.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_rgba_comp.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_rgba_comp.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_rgba_comp.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	VkImageSubresourceRange image_view_subres;
	image_view_subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	image_view_subres.baseMipLevel = 0;
	image_view_subres.levelCount = 1;
	image_view_subres.baseArrayLayer = 0;
	image_view_subres.layerCount = 1;

	for (uint32_t i = 0; i < swap_image_count; i++) {
		image_view_cre_infos[i].sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_cre_infos[i].pNext = NULL;
		image_view_cre_infos[i].flags = 0;
		image_view_cre_infos[i].image = swap_images[i];
		image_view_cre_infos[i].viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_cre_infos[i].format = VK_FORMAT_B8G8R8A8_SRGB;
		image_view_cre_infos[i].components = image_view_rgba_comp;
		image_view_cre_infos[i].subresourceRange = image_view_subres;
		vkCreateImageView(dev, &image_view_cre_infos[i], NULL, &image_views[i]);
		printf("image view %d created.\n", i);
	}
//
//
//render pass creation part		line_477 to line_574
//
//fill attachment description
//
	VkAttachmentDescription attach_descp;
	attach_descp.flags = 0;
	attach_descp.format = VK_FORMAT_B8G8R8A8_SRGB;
	attach_descp.samples = VK_SAMPLE_COUNT_1_BIT;
	attach_descp.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attach_descp.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attach_descp.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attach_descp.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attach_descp.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attach_descp.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	printf("attachment description filled.\n");
//
//fill attachment reference
//
	VkAttachmentReference attach_ref;
	attach_ref.attachment = 0;
	attach_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	printf("attachment reference filled.\n");
//
//fill subpass description
//
	VkSubpassDescription subp_descp;
	subp_descp.flags = 0;
	subp_descp.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subp_descp.inputAttachmentCount = 0;
	subp_descp.pInputAttachments = NULL;
	subp_descp.colorAttachmentCount = 1;
	subp_descp.pColorAttachments = &attach_ref;
	subp_descp.pResolveAttachments = NULL;
	subp_descp.pDepthStencilAttachment = NULL;
	subp_descp.preserveAttachmentCount = 0;
	subp_descp.pPreserveAttachments = NULL;
	printf("subpass description filled.\n");
//
//fill subpass dependency
//
	VkSubpassDependency subp_dep;
	subp_dep.srcSubpass = VK_SUBPASS_EXTERNAL;
	subp_dep.dstSubpass = 0;
	subp_dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subp_dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subp_dep.srcAccessMask = 0;
	subp_dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subp_dep.dependencyFlags = 0;
	printf("subpass dependency created.\n");
//
//create render pass
//
	VkRenderPassCreateInfo rendp_cre_info;
	rendp_cre_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	rendp_cre_info.pNext = NULL;
	rendp_cre_info.flags = 0;
	rendp_cre_info.attachmentCount = 1;
	rendp_cre_info.pAttachments = &attach_descp;
	rendp_cre_info.subpassCount = 1;
	rendp_cre_info.pSubpasses = &subp_descp;
	rendp_cre_info.dependencyCount = 1;
	rendp_cre_info.pDependencies = &subp_dep;

	VkRenderPass rendp;
	vkCreateRenderPass(dev, &rendp_cre_info, NULL, &rendp);
	printf("render pass created.\n");
//
//
//pipeline creation part		line_575 to line_935
//
//load shader
//
	FILE *fp_vert = NULL;
	FILE *fp_frag = NULL;

	char *vertPath;
	char *fragPath;
	asprintf(&vertPath, "%s/%s", resourcePath, "vert.spv");
	asprintf(&fragPath, "%s/%s", resourcePath, "frag.spv");
	fp_vert = fopen(vertPath, "rb");
	fp_frag = fopen(fragPath, "rb");
	char shader_loaded = 1;
	if (fp_vert == NULL || fp_frag == NULL) {
		shader_loaded = 0;
		printf("can't find SPIR-V binaries.\n");
	}
	fseek(fp_vert, 0, SEEK_END);
	fseek(fp_frag, 0, SEEK_END);
	uint32_t vert_size = ftell(fp_vert);
	uint32_t frag_size = ftell(fp_frag);

	char *p_vert_code = (char *) malloc(vert_size * sizeof(char));
	char *p_frag_code = (char *) malloc(frag_size * sizeof(char));

	rewind(fp_vert);
	rewind(fp_frag);
	fread(p_vert_code, 1, vert_size, fp_vert);
	printf("vertex shader binaries loaded.\n");
	fread(p_frag_code, 1, frag_size, fp_frag);
	printf("fragment shader binaries loaded.\n");

	fclose(fp_vert);
	fclose(fp_frag);
//
//create shader modules
//
	VkShaderModuleCreateInfo vert_shad_mod_cre_info;
	vert_shad_mod_cre_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	vert_shad_mod_cre_info.pNext = NULL;
	vert_shad_mod_cre_info.flags = 0;
	vert_shad_mod_cre_info.codeSize = shader_loaded ? vert_size : 0;
	vert_shad_mod_cre_info.pCode = shader_loaded ? (const uint32_t *) p_vert_code : NULL;

	VkShaderModuleCreateInfo frag_shad_mod_cre_info;
	frag_shad_mod_cre_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	frag_shad_mod_cre_info.pNext = NULL;
	frag_shad_mod_cre_info.flags = 0;
	frag_shad_mod_cre_info.codeSize = shader_loaded ? frag_size : 0;
	frag_shad_mod_cre_info.pCode = shader_loaded ? (const uint32_t *) p_frag_code : NULL;

	VkShaderModule vert_shad_mod;
	VkShaderModule frag_shad_mod;
	vkCreateShaderModule(dev, &vert_shad_mod_cre_info, NULL, &vert_shad_mod);
	printf("vertex shader module created.\n");
	vkCreateShaderModule(dev, &frag_shad_mod_cre_info, NULL, &frag_shad_mod);
	printf("fragment shader module created.\n");
//
//fill shader stage info
//
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
	printf("vertex shader stage info filled.\n");

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
	printf("fragment shader stage info filled.\n");

	shad_stage_cre_infos[0] = vert_shad_stage_cre_info;
	shad_stage_cre_infos[1] = frag_shad_stage_cre_info;
//
//fill vertex input state info
//
	VkPipelineVertexInputStateCreateInfo vert_input_info;
	vert_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vert_input_info.pNext = NULL;
	vert_input_info.flags = 0;
	vert_input_info.vertexBindingDescriptionCount = 0;
	vert_input_info.pVertexBindingDescriptions = NULL;
	vert_input_info.vertexAttributeDescriptionCount = 0;
	vert_input_info.pVertexAttributeDescriptions = NULL;
	printf("vertex input state info filled.\n");
//
//fill input assembly state info
//
	VkPipelineInputAssemblyStateCreateInfo input_asm_info;
	input_asm_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_asm_info.pNext = NULL;
	input_asm_info.flags = 0;
	input_asm_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_asm_info.primitiveRestartEnable = VK_FALSE;
	printf("input assembly info filled.\n");
//
//fill viewport
//
	VkViewport viewport;
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = windowExtent.width;
	viewport.height = windowExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	printf("viewport filled.\n");
//
//fill scissor
//
	VkRect2D scissor;
	VkOffset2D sci_offset;
	sci_offset.x = 0;
	sci_offset.y = 0;
	scissor.offset = sci_offset;
	scissor.extent = windowExtent;
	printf("scissor filled.\n");
//
//fill viewport state info
//
	VkPipelineViewportStateCreateInfo vwp_state_cre_info;
	vwp_state_cre_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	vwp_state_cre_info.pNext = NULL;
	vwp_state_cre_info.flags = 0;
	vwp_state_cre_info.viewportCount = 1;
	vwp_state_cre_info.pViewports = &viewport;
	vwp_state_cre_info.scissorCount = 1;
	vwp_state_cre_info.pScissors = &scissor;
	printf("viewport state filled.\n");
//
//fill rasterizer state info
//
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
	printf("rasterization info filled.\n");
//
//fill multisampling state info
//
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
	printf("multisample info filled.\n");
//
//fill color blend attachment state
//
	VkPipelineColorBlendAttachmentState color_blend_attach;
	color_blend_attach.blendEnable = VK_FALSE;
	color_blend_attach.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attach.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_blend_attach.colorBlendOp = VK_BLEND_OP_ADD;
	color_blend_attach.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attach.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_blend_attach.alphaBlendOp = VK_BLEND_OP_ADD;
	color_blend_attach.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	printf("color blend attachment state filled.\n");
//
//fill color blend state info
//
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
	printf("color blend state info filled.\n");
//
//create pipeline layout
//
	VkPipelineLayoutCreateInfo pipe_lay_cre_info;
	pipe_lay_cre_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipe_lay_cre_info.pNext = NULL;
	pipe_lay_cre_info.flags = 0;
	pipe_lay_cre_info.setLayoutCount = 0;
	pipe_lay_cre_info.pSetLayouts = NULL;
	pipe_lay_cre_info.pushConstantRangeCount = 0;
	pipe_lay_cre_info.pPushConstantRanges = NULL;

	VkPipelineLayout pipe_layout;
	vkCreatePipelineLayout(dev, &pipe_lay_cre_info, NULL, &pipe_layout);
	printf("pipeline layout created.\n");
//
//create pipeline
//
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

	pipe_cre_info.layout = pipe_layout;
	pipe_cre_info.renderPass = rendp;
	pipe_cre_info.subpass = 0;
	pipe_cre_info.basePipelineHandle = VK_NULL_HANDLE;
	pipe_cre_info.basePipelineIndex = -1;

	VkPipeline pipe;
	vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &pipe_cre_info, NULL, &pipe);
	printf("graphics pipeline created.\n");
//
//destroy shader module
//
	vkDestroyShaderModule(dev, frag_shad_mod, NULL);
	printf("fragment shader module destroyed.\n");
	vkDestroyShaderModule(dev, vert_shad_mod, NULL);
	printf("vertex shader module destroyed.\n");
	free(p_frag_code);
	printf("fragment shader binaries released.\n");
	free(p_vert_code);
	printf("vertex shader binaries released.\n");
//
//
//framebuffer creation		line_936 to line_967
//
//create framebuffer
//
	VkFramebufferCreateInfo frame_buff_cre_infos[swap_image_count];
	VkFramebuffer frame_buffs[swap_image_count];
	VkImageView image_attachs[swap_image_count];
	for (uint32_t i = 0; i < swap_image_count; i++) {
		image_attachs[i] = image_views[i];
		frame_buff_cre_infos[i].sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frame_buff_cre_infos[i].pNext = NULL;
		frame_buff_cre_infos[i].flags = 0;
		frame_buff_cre_infos[i].renderPass = rendp;
		frame_buff_cre_infos[i].attachmentCount = 1;
		frame_buff_cre_infos[i].pAttachments = &(image_attachs[i]);
		frame_buff_cre_infos[i].width = windowExtent.width;
		frame_buff_cre_infos[i].height = windowExtent.height;
		frame_buff_cre_infos[i].layers = 1;

		vkCreateFramebuffer(dev, &(frame_buff_cre_infos[i]), NULL, &(frame_buffs[i]));
		printf("framebuffer %d created.\n", i);
	}
//
//
//command buffer creation		line_968 to line_1001
//
//create command pool
//
	VkCommandPoolCreateInfo cmd_pool_cre_info;
	cmd_pool_cre_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmd_pool_cre_info.pNext = NULL;
	cmd_pool_cre_info.flags = 0;
	cmd_pool_cre_info.queueFamilyIndex = graphicsQueueFamilyIndex;

	VkCommandPool cmd_pool;
	vkCreateCommandPool(dev, &cmd_pool_cre_info, NULL, &cmd_pool);
	printf("command pool created.\n");
//
//allocate command buffers
//
	VkCommandBufferAllocateInfo cmd_buff_alloc_info;
	cmd_buff_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmd_buff_alloc_info.pNext = NULL;
	cmd_buff_alloc_info.commandPool = cmd_pool;
	cmd_buff_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmd_buff_alloc_info.commandBufferCount = swap_image_count;

	VkCommandBuffer cmd_buffers[swap_image_count];
	vkAllocateCommandBuffers(dev, &cmd_buff_alloc_info, cmd_buffers);
	printf("command buffers allocated.\n");
//
//
//render preparation		line1002 to line1062
//
	VkCommandBufferBeginInfo cmd_buff_begin_infos[swap_image_count];
	VkRenderPassBeginInfo rendp_begin_infos[swap_image_count];
	VkRect2D rendp_area;
	rendp_area.offset.x = 0;
	rendp_area.offset.y = 0;
	rendp_area.extent = windowExtent;
	VkClearValue clear_val = {0.0f, 0.0f, 0.0f, 0.5f};
	for (uint32_t i = 0; i < swap_image_count; i++) {


		cmd_buff_begin_infos[i].sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmd_buff_begin_infos[i].pNext = NULL;
		cmd_buff_begin_infos[i].flags = 0;
		cmd_buff_begin_infos[i].pInheritanceInfo = NULL;
		printf("command buffer begin info %d filled.\n", i);

		rendp_begin_infos[i].sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rendp_begin_infos[i].pNext = NULL;
		rendp_begin_infos[i].renderPass = rendp;
		rendp_begin_infos[i].framebuffer = frame_buffs[i];
		rendp_begin_infos[i].renderArea = rendp_area;
		rendp_begin_infos[i].clearValueCount = 1;
		rendp_begin_infos[i].pClearValues = &clear_val;
		printf("render pass begin info %d filled.\n", i);

		vkBeginCommandBuffer(cmd_buffers[i], &cmd_buff_begin_infos[i]);

		vkCmdBeginRenderPass(cmd_buffers[i], &(rendp_begin_infos[i]), VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);

		vkCmdDraw(cmd_buffers[i], 3, 1, 0, 0);

		vkCmdEndRenderPass(cmd_buffers[i]);

		vkEndCommandBuffer(cmd_buffers[i]);

		printf("command buffer drawing recorded.\n");
	}
//
//
//semaphores and fences creation part		line_1063 to line_1103
//
	uint32_t max_frames = 2;
	VkSemaphore semps_img_avl[max_frames];
	VkSemaphore semps_rend_fin[max_frames];
	VkFence fens[max_frames];

	VkSemaphoreCreateInfo semp_cre_info;
	semp_cre_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semp_cre_info.pNext = NULL;
	semp_cre_info.flags = 0;

	VkFenceCreateInfo fen_cre_info;
	fen_cre_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fen_cre_info.pNext = NULL;
	fen_cre_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (uint32_t i = 0; i < max_frames; i++) {
		vkCreateSemaphore(dev, &semp_cre_info, NULL, &(semps_img_avl[i]));
		vkCreateSemaphore(dev, &semp_cre_info, NULL, &(semps_rend_fin[i]));
		vkCreateFence(dev, &fen_cre_info, NULL, &(fens[i]));
	}
	printf("semaphores and fences created.\n");

	uint32_t cur_frame = 0;
	VkFence fens_img[swap_image_count];
	for (uint32_t i = 0; i < swap_image_count; i++) {
		fens_img[i] = VK_NULL_HANDLE;
	}
//
//
//main present part		line_1104 to line_1197
//
	printf("\n");
	for (;;) {
#ifdef __APPLE__
		InputEvent *inputEvent;
		while (dequeue(inputQueue, (void **) &inputEvent)) {
			printf("thread: %d\n", inputEvent->type);
			free(inputEvent);
		}
#elif defined _WIN32 || defined _WIN64
		MSG msg = {};
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
#endif
//
//submit
//
		vkWaitForFences(dev, 1, &(fens[cur_frame]), VK_TRUE, UINT64_MAX);

		uint32_t img_index = 0;
		vkAcquireNextImageKHR(dev, swap, UINT64_MAX, semps_img_avl[cur_frame], VK_NULL_HANDLE, &img_index);

		if (fens_img[img_index] != VK_NULL_HANDLE) {
			vkWaitForFences(dev, 1, &(fens_img[img_index]), VK_TRUE, UINT64_MAX);
		}

		fens_img[img_index] = fens[cur_frame];

		VkSubmitInfo sub_info;
		sub_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		sub_info.pNext = NULL;

		VkSemaphore semps_wait[1];
		semps_wait[0] = semps_img_avl[cur_frame];
		VkPipelineStageFlags wait_stages[1];
		wait_stages[0] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		sub_info.waitSemaphoreCount = 1;
		sub_info.pWaitSemaphores = &(semps_wait[0]);
		sub_info.pWaitDstStageMask = &(wait_stages[0]);
		sub_info.commandBufferCount = 1;
		sub_info.pCommandBuffers = &(cmd_buffers[img_index]);

		VkSemaphore semps_sig[1];
		semps_sig[0] = semps_rend_fin[cur_frame];

		sub_info.signalSemaphoreCount = 1;
		sub_info.pSignalSemaphores = &(semps_sig[0]);

		vkResetFences(dev, 1, &(fens[cur_frame]));

		vkQueueSubmit(graphicsQueue, 1, &sub_info, fens[cur_frame]);
//
//present
//
		VkPresentInfoKHR pres_info;

		pres_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		pres_info.pNext = NULL;
		pres_info.waitSemaphoreCount = 1;
		pres_info.pWaitSemaphores = &(semps_sig[0]);

		VkSwapchainKHR swaps[1];
		swaps[0] = swap;
		pres_info.swapchainCount = 1;
		pres_info.pSwapchains = &(swaps[0]);
		pres_info.pImageIndices = &img_index;
		pres_info.pResults = NULL;

		vkQueuePresentKHR(presentationQueue, &pres_info);

		cur_frame = (cur_frame + 1) % max_frames;
	}
	vkDeviceWaitIdle(dev);
}
