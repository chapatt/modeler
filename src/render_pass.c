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
#ifdef DRAW_WINDOW_DECORATION
		.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
#else
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
#endif
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

#ifdef ENABLE_IMGUI
 	VkAttachmentDescription imAttachmentDescription = {
		.flags = 0,
		.format = swapchainInfo.surfaceFormat.format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
#ifdef DRAW_WINDOW_DECORATION
		.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
#else
		.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
#endif
#ifdef DRAW_WINDOW_DECORATION
		.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
#else
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
#endif
	};

	VkAttachmentReference imAttachmentReference = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription imSubpassDescription = {
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

	VkSubpassDependency imSubpassDependency = {
		.srcSubpass = 0,
		.dstSubpass = 1,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.dependencyFlags = 0
	};
#endif /* ENABLE_IMGUI */

#if DRAW_WINDOW_DECORATION
	VkAttachmentDescription windowDecorationAttachmentDescription = {
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

	VkAttachmentReference windowDecorationAttachmentInputReference = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};

	VkAttachmentReference windowDecorationAttachmentReference = {
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription windowDecorationSubpassDescription = {
		.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 1,
		.pInputAttachments = &windowDecorationAttachmentInputReference,
		.colorAttachmentCount = 1,
		.pColorAttachments = &windowDecorationAttachmentReference,
		.pResolveAttachments = NULL,
		.pDepthStencilAttachment = NULL,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments = NULL
	};

	VkSubpassDependency windowDecorationSubpassDependency = {
#if ENABLE_IMGUI
		.srcSubpass = 1,
		.dstSubpass = 2,
#else /* ENABLE_IMGUI */
		.srcSubpass = 0,
		.dstSubpass = 1,
#endif /* ENABLE_IMGUI */
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
		.dependencyFlags = 0
	};

#if ENABLE_IMGUI
	VkAttachmentDescription attachmentDescriptions[] = {attachmentDescription, imAttachmentDescription, windowDecorationAttachmentDescription};
	VkSubpassDescription subpassDescriptions[] = {subpassDescription, imSubpassDescription, windowDecorationSubpassDescription};
	VkSubpassDependency subpassDependencies[] = {subpassDependency, imSubpassDependency, windowDecorationSubpassDependency};
#else /* ENABLE_IMGUI */
	VkAttachmentDescription attachmentDescriptions[] = {attachmentDescription, windowDecorationAttachmentDescription};
	VkSubpassDescription subpassDescriptions[] = {subpassDescription, windowDecorationSubpassDescription};
	VkSubpassDependency subpassDependencies[] = {subpassDependency, windowDecorationSubpassDependency};
#endif /* ENABLE_IMGUI */
#else /* DRAW_WINDOW_DECORATION */
#if ENABLE_IMGUI
	VkAttachmentDescription attachmentDescriptions[] = {attachmentDescription, imAttachmentDescription};
	VkSubpassDescription subpassDescriptions[] = {subpassDescription, imSubpassDescription};
	VkSubpassDependency subpassDependencies[] = {subpassDependency, imSubpassDependency};
#else
	VkAttachmentDescription attachmentDescriptions[] = {attachmentDescription};
	VkSubpassDescription subpassDescriptions[] = {subpassDescription};
	VkSubpassDependency subpassDependencies[] = {subpassDependency};
#endif /* ENABLE_IMGUI */
#endif /* DRAW_WINDOW_DECORATION */

	VkRenderPassCreateInfo renderPassCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
#if DRAW_WINDOW_DECORATION
		.attachmentCount = 2,
#else /* DRAW_WINDOW_DECORATION */
		.attachmentCount = 1,
#endif /* DRAW_WINDOW_DECORATION */
		.pAttachments = attachmentDescriptions,
#if DRAW_WINDOW_DECORATION
#if ENABLE_IMGUI
		.subpassCount = 3,
		.dependencyCount = 3,
#else /* ENABLE_IMGUI */
		.subpassCount = 2,
		.dependencyCount = 2,
#endif /* ENABLE_IMGUI */
#else /* DRAW_WINDOW_DECORATION */
#if ENABLE_IMGUI
		.subpassCount = 2,
		.dependencyCount = 2,
#else /* ENABLE_IMGUI */
		.subpassCount = 1,
		.dependencyCount = 1,
#endif /* ENABLE_IMGUI */
#endif /* DRAW_WINDOW_DECORATION */
		.pSubpasses = subpassDescriptions,
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
