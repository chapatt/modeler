#ifndef MODELER_IMAGE_VIEW_H
#define MODELER_IMAGE_VIEW_H

#include <stdbool.h>
#include <vulkan/vulkan.h>

#include "swapchain.h"

bool createImageViews(VkDevice device, SwapchainInfo *swapchainInfo, VkImageView **imageViews, char **error);

#endif // MODELER_IMAGE_VIEW_H