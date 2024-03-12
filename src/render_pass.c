 #include <stdio.h>

 #include "utils.h"
 #include "vulkan_utils.h"

 #include "render_pass.h"

bool createRenderPass(VkDevice device, SwapchainInfo swapchainInfo, VkRenderPass *renderPass, char **error)
{
	VkAttachmentDescription attachmentDescription = {
		.flags = 0,
		.format = swapchainInfo.surfaceFormat.format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkAttachmentReference attachmentReference = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpassDescription = {
		.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 0,
		.pInputAttachments = NULL,
		.colorAttachmentCount = 1,
		.pColorAttachments = &attachmentReference,
		.pResolveAttachments = NULL,
		.pDepthStencilAttachment = NULL,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments = NULL
	};

	VkSubpassDependency subpassDependency = {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.dependencyFlags = 0
	};

	VkAttachmentDescription secondAttachmentDescription = {
		.flags = 0,
		.format = swapchainInfo.surfaceFormat.format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};

	VkAttachmentReference secondAttachmentInputReference = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};

	VkAttachmentReference secondAttachmentReference = {
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription secondSubpassDescription = {
		.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 1,
		.pInputAttachments = &secondAttachmentInputReference,
		.colorAttachmentCount = 1,
		.pColorAttachments = &secondAttachmentReference,
		.pResolveAttachments = NULL,
		.pDepthStencilAttachment = NULL,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments = NULL
	};

	VkSubpassDependency secondSubpassDependency = {
		.srcSubpass = 0,
		.dstSubpass = 1,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
		.dependencyFlags = 0
	};

	VkAttachmentDescription attachmentDescriptions[] = {attachmentDescription, secondAttachmentDescription};
	VkSubpassDescription subpassDescriptions[] = {subpassDescription, secondSubpassDescription};
	VkSubpassDependency subpassDependencies[] = {subpassDependency, secondSubpassDependency};

	VkRenderPassCreateInfo renderPassCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.attachmentCount = 2,
		.pAttachments = attachmentDescriptions,
		.subpassCount = 2,
		.pSubpasses = subpassDescriptions,
		.dependencyCount = 2,
		.pDependencies = subpassDependencies
	};

	VkResult result;
	if ((result = vkCreateRenderPass(device, &renderPassCreateInfo, NULL, renderPass)) != VK_SUCCESS) {
		asprintf(error, "Failed to create render pass: %s", string_VkResult(result));
		return false;
	}

	return true;
}

void destroyRenderPass(VkDevice device, VkRenderPass renderPass) {
	vkDestroyRenderPass(device, renderPass, NULL);
}
