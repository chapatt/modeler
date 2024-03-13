#include "stdlib.h"

#include "utils.h"
#include "vulkan_utils.h"

#include "descriptor.h"

static bool createDescriptorPool(VkDevice device, VkDescriptorPool *descriptorPool, char **error);

bool createDescriptorSets(
	VkDevice device,
	CreateDescriptorSetInfo info,
	VkDescriptorPool *descriptorPool,
	VkDescriptorSet **imageDescriptorSets,
	VkDescriptorSetLayout **imageDescriptorSetLayouts,
	VkDescriptorSet **bufferDescriptorSets,
	VkDescriptorSetLayout **bufferDescriptorSetLayouts,
	char **error
) {
	if (!createDescriptorPool(device, descriptorPool, error)) {
		return false;
	}

	*imageDescriptorSetLayouts = malloc(sizeof(**imageDescriptorSetLayouts) * info.imageCount);
	for (size_t i = 0; i < info.imageCount; ++i) {
		VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {
			.binding = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = NULL
		};

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = 1,
			.pBindings = &descriptorSetLayoutBinding,
		};

		VkResult result;
		if ((result = vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, NULL, imageDescriptorSetLayouts[i])) != VK_SUCCESS) {
			asprintf(error, "Failed to create descriptor set layout: %s", string_VkResult(result));
		}
	}

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = *descriptorPool,
		.descriptorSetCount = info.imageCount,
		.pSetLayouts = *imageDescriptorSetLayouts
	};

	*imageDescriptorSets = malloc(sizeof(**imageDescriptorSets) * info.imageCount);
	vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, *imageDescriptorSets);

	VkWriteDescriptorSet *writeDescriptorSets = malloc(sizeof(*writeDescriptorSets) * info.imageCount);
	for (size_t i = 0; i < info.imageCount; ++i) {
		VkDescriptorImageInfo descriptorImageInfo = {
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.imageView = info.imageViews[i],
			.sampler = VK_NULL_HANDLE
		};

		writeDescriptorSets[i] = (VkWriteDescriptorSet) {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = *imageDescriptorSets[i],
			.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
			.descriptorCount = 1,
			.dstBinding = 0,
			.pImageInfo = &descriptorImageInfo
		};
	}

	vkUpdateDescriptorSets(device, 1, writeDescriptorSets, 0, NULL);

	return true;
}

void destroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout)
{
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, NULL);
}

void destroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool)
{
	vkDestroyDescriptorPool(device, descriptorPool, NULL);
}

static bool createDescriptorPool(VkDevice device, VkDescriptorPool *descriptorPool, char **error) {
	VkDescriptorPoolSize descriptorPoolSize = {
		.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
		.descriptorCount = 1
	};

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.poolSizeCount = 1,
		.pPoolSizes = &descriptorPoolSize,
		.maxSets = 1
	};

	VkResult result;
	if ((result = vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, NULL, descriptorPool)) != VK_SUCCESS) {
		asprintf(error, "Failed to create descriptor pool: %s", string_VkResult(result));
		return false;
	}

	return true;
}
