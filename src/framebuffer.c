#include <stdlib.h>

#include "swapchain.h"
#include "utils.h"
#include "vulkan_utils.h"

#include "framebuffer.h"

bool createFramebuffers(VkDevice device, SwapchainInfo *swapchainInfo, VkImageView *imageViews, VkRenderPass renderPass, VkFramebuffer **framebuffers, char **error)
{
	*framebuffers = malloc(sizeof(*framebuffers) * swapchainInfo->imageCount);
	for (uint32_t i = 0; i < swapchainInfo->imageCount; ++i) {
		VkFramebufferCreateInfo framebufferCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.renderPass = renderPass,
			.attachmentCount = 1,
			.pAttachments = imageViews + i,
			.width = swapchainInfo->extent.width,
			.height = swapchainInfo->extent.height,
 			.layers = 1
		};

		VkResult result;
		if ((result = vkCreateFramebuffer(device, &framebufferCreateInfo, NULL, *framebuffers + i)) != VK_SUCCESS) {
			asprintf(error, "Failed to create framebuffers: %s", string_VkResult(result));
			return false;
		}
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