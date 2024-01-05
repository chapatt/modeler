#ifndef MODELER_COMMAND_POOL_H
#define MODELER_COMMAND_POOL_H

#include <stdbool.h>
#include <vulkan/vulkan.h>

#include "device.h"

bool createCommandPool(VkDevice device, QueueInfo queueInfo, VkCommandPool *commandPool, char **error);

void destroyCommandPool(VkDevice device, VkCommandPool commandPool);

#endif /* MODELER_COMMAND_POOL_H */
