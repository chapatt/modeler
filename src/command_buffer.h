#ifndef MODELER_COMMAND_BUFFER_H
#define MODELER_COMMAND_BUFFER_H

#include <stdbool.h>
#include <vulkan/vulkan.h>

#include "swapchain.h"

bool createCommandBuffers(VkDevice device, VkCommandPool commandPool, VkCommandBuffer **commandBuffers, char **error);

bool beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkCommandBuffer *commandBuffer, char **error);

bool endSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer, char **error);

void freeCommandBuffers(VkDevice device, VkCommandPool commandPool, VkCommandBuffer *commandBuffers, uint32_t count);

#endif /* MODELER_COMMAND_BUFFER_H */
