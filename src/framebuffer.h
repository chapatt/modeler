#ifndef MODELER_FRAMEBUFFER_H
#define MODELER_FRAMEBUFFER_H

#include <stdbool.h>
#include <vulkan/vulkan.h>

bool createFramebuffers(VkDevice device, SwapchainInfo swapchainInfo, VkImageView *offscreenImageView, VkImageView *imageViews, VkRenderPass renderPass, VkFramebuffer **framebuffers, char **error);

void destroyFramebuffers(VkDevice device, VkFramebuffer *framebuffers, uint32_t count);

#endif /* MODELER_FRAMEBUFFER_H */
