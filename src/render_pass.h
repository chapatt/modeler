#ifndef MODELER_RENDER_PASS_H
#define MODELER_RENDER_PASS_H

#include <stdbool.h>
#include <vulkan/vulkan.h>

#include "swapchain.h"

bool createRenderPass(VkDevice device, SwapchainInfo swapchainInfo, VkFormat depthImageFormat, VkRenderPass *renderPass, char **error);

void destroyRenderPass(VkDevice device, VkRenderPass renderPass);

#endif /* MODELER_RENDER_PASS_H */