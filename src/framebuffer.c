#include <stdlib.h>

#include "swapchain.h"
#include "utils.h"
#include "vulkan_utils.h"

#include "framebuffer.h"

bool createFramebuffer(VkDevice device, SwapchainInfo swapchainInfo, VkImageView *attachments, uint32_t attachmentCount, VkRenderPass renderPass, VkFramebuffer *framebuffer, char **error)
{
	VkFramebufferCreateInfo framebufferCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.renderPass = renderPass,
		.attachmentCount = attachmentCount,
		.pAttachments = attachments,
		.width = swapchainInfo.extent.width,
		.height = swapchainInfo.extent.height,
		.layers = 1
	};

	VkResult result;
	if ((result = vkCreateFramebuffer(device, &framebufferCreateInfo, NULL, framebuffer)) != VK_SUCCESS) {
		asprintf(error, "Failed to create framebuffers: %s", string_VkResult(result));
		return false;
	}

	return true;
}

void destroyFramebuffers(VkDevice device, VkFramebuffer *framebuffers, uint32_t count)
{
	for (uint32_t i = 0; i < count; ++i) {
		vkDestroyFramebuffer(device, framebuffers[i], NULL);
	}
	free(framebuffers);
}