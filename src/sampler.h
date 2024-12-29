#ifndef MODELER_SAMPLER_H
#define MODELER_SAMPLER_H

#include <stdbool.h>

#include "vulkan_utils.h"

bool createSampler(VkDevice device, float anisotropy, int mipLevels, VkSampler *sampler, char **error);
void destroySampler(VkDevice device, VkSampler sampler);

#endif /* MODELER_SAMPLER_H */
