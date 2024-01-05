#ifndef MODELER_COMMAND_BUFFER_H
#define MODELER_COMMAND_BUFFER_H

#include <stdbool.h>
#include <vulkan/vulkan.h>

#include "swapchain.h"

bool createCommandBuffers(VkDevice device, SwapchainInfo swapchainInfo, VkCommandPool commandPool, VkCommandBuffer **commandBuffers, char **error);

#endif /* MODELER_COMMAND_POOL_H */
