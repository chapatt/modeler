#ifndef MODELER_IMAGE_VIEW_H
#define MODELER_IMAGE_VIEW_H

#include <stdbool.h>
#include <vulkan/vulkan.h>

#include "swapchain.h"

bool createImageViews(VkDevice device, VkImage *images, uint32_t imageCount, VkFormat format, VkImageView *imageViews, char **error);
void destroyImageViews(VkDevice device, VkImageView *imageViews, uint32_t count);

#endif /* MODELER_IMAGE_VIEW_H */