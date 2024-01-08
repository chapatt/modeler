#ifndef MODELER_COMMAND_BUFFER_H
#define MODELER_COMMAND_BUFFER_H

#include <stdbool.h>
#include <vulkan/vulkan.h>

#include "swapchain.h"

bool createCommandBuffers(VkDevice device, SwapchainInfo swapchainInfo, VkCommandPool commandPool, VkCommandBuffer **commandBuffers, char **error);

void freeCommandBuffers(VkDevice device, VkCommandPool commandPool, VkCommandBuffer *commandBuffers, uint32_t count);

#endif /* MODELER_COMMAND_BUFFER_H */
