#ifndef MODELER_FRAMEBUFFER_H
#define MODELER_FRAMEBUFFER_H

#include <stdbool.h>
#include <vulkan/vulkan.h>

bool createFramebuffer(VkDevice device, SwapchainInfo swapchainInfo, VkImageView *attachments, uint32_t attachmentCount, VkRenderPass renderPass, VkFramebuffer *framebuffer, char **error);

void destroyFramebuffers(VkDevice device, VkFramebuffer *framebuffers, uint32_t count);

#endif /* MODELER_FRAMEBUFFER_H */
