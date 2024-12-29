#include "sampler.h"
#include "utils.h"

bool createSampler(VkDevice device, float anisotropy, int mipLevels, VkSampler *sampler, char **error)
{
	VkSamplerCreateInfo samplerInfo = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.anisotropyEnable = VK_TRUE,
		.maxAnisotropy = anisotropy,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.mipLodBias = 0.0f,
		.minLod = 0.0f,
		.maxLod = (float) mipLevels
	};

	VkResult result;
	if ((result = vkCreateSampler(device, &samplerInfo, NULL, sampler)) != VK_SUCCESS) {
		asprintf(error, "Failed to create sampler.\n");
		return false;
	}

	return true;
}

void destroySampler(VkDevice device, VkSampler sampler)
{
	vkDestroySampler(device, sampler, NULL);
}
