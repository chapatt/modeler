#include "stdlib.h"

#include "utils.h"
#include "vulkan_utils.h"

#include "descriptor.h"

static bool createDescriptorPool(VkDevice device, VkDescriptorPool *descriptorPool, VkDescriptorPoolSize *descriptorPoolSizes, size_t descriptorPoolSizeCount, size_t descriptorSetCount, char **error);

bool createDescriptorSets(
	VkDevice device,
	CreateDescriptorSetInfo *infos,
	size_t descriptorSetCount,
	VkDescriptorPool *descriptorPool,
	VkDescriptorSet *descriptorSets,
	VkDescriptorSetLayout *descriptorSetLayouts,
	char **error
) {
	VkDescriptorPoolSize descriptorPoolSizes[descriptorSetCount];
	for (uint32_t i = 0; i < descriptorSetCount; ++i) {
		descriptorPoolSizes[i] = (VkDescriptorPoolSize) {
			.type = infos[i].type,
			.descriptorCount = infos[i].descriptorCount
		};
	}

	if (!createDescriptorPool(device, descriptorPool, descriptorPoolSizes, descriptorSetCount, descriptorSetCount, error)) {
		return false;
	}

	for (size_t i = 0; i < descriptorSetCount; ++i) {
		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = infos[i].bindingCount,
			.pBindings = infos[i].bindings,
		};

		VkResult result;
		if ((result = vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, NULL, descriptorSetLayouts + i)) != VK_SUCCESS) {
			asprintf(error, "Failed to create descriptor set layout: %s", string_VkResult(result));
		}
	}

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = *descriptorPool,
		.descriptorSetCount = descriptorSetCount,
		.pSetLayouts = descriptorSetLayouts
	};

	vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, descriptorSets);

	VkWriteDescriptorSet *writeDescriptorSets = malloc(sizeof(*writeDescriptorSets) * descriptorSetCount);
	for (size_t i = 0; i < descriptorSetCount; ++i) {
		writeDescriptorSets[i] = (VkWriteDescriptorSet) {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSets[i],
			.descriptorType = infos[i].type,
			.descriptorCount = infos[i].descriptorCount,
			.dstBinding = 0,
			.pImageInfo = infos[i].descriptorInfos
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

static bool createDescriptorPool(VkDevice device, VkDescriptorPool *descriptorPool, VkDescriptorPoolSize *descriptorPoolSizes, size_t descriptorPoolSizeCount, size_t descriptorSetCount, char **error)
{
	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.poolSizeCount = descriptorPoolSizeCount,
		.pPoolSizes = descriptorPoolSizes,
		.maxSets = descriptorSetCount
	};

	VkResult result;
	if ((result = vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, NULL, descriptorPool)) != VK_SUCCESS) {
		asprintf(error, "Failed to create descriptor pool: %s", string_VkResult(result));
		return false;
	}

	return true;
}
