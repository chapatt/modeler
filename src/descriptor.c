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
	size_t bindingCount = 0;
	for (size_t i = 0; i < descriptorSetCount; ++i) {
		bindingCount += infos[i].bindingCount;
	}

	VkDescriptorPoolSize descriptorPoolSizes[bindingCount];
	size_t descriptorPoolSizeOffset = 0;
	for (size_t i = 0; i < descriptorSetCount; ++i) {
		for (size_t j = 0; j < infos[i].bindingCount; ++j) {
			descriptorPoolSizes[descriptorPoolSizeOffset] = (VkDescriptorPoolSize) {
				.type = infos[i].bindings[j].descriptorType,
				.descriptorCount = infos[i].bindings[j].descriptorCount
			};

			++descriptorPoolSizeOffset;
		}
	}

	if (!createDescriptorPool(device, descriptorPool, descriptorPoolSizes, bindingCount, descriptorSetCount, error)) {
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
			return false;
		}
	}

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = *descriptorPool,
		.descriptorSetCount = descriptorSetCount,
		.pSetLayouts = descriptorSetLayouts
	};

	vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, descriptorSets);

	VkWriteDescriptorSet writeDescriptorSets[bindingCount];
	size_t writeDescriptorSetOffset = 0;
	for (size_t i = 0; i < descriptorSetCount; ++i) {
		for (size_t j = 0; j < infos[i].bindingCount; ++j) {
			writeDescriptorSets[writeDescriptorSetOffset] = (VkWriteDescriptorSet) {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = descriptorSets[i],
				.descriptorType = infos[i].bindings[j].descriptorType,
				.descriptorCount = infos[i].bindings[j].descriptorCount,
				.dstBinding = j,
			};

			switch (infos[i].bindings[j].descriptorType) {
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				writeDescriptorSets[writeDescriptorSetOffset].pImageInfo = ((VkDescriptorImageInfo **) infos[i].descriptorInfos)[j];
				break;
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
				writeDescriptorSets[writeDescriptorSetOffset].pBufferInfo = ((VkDescriptorBufferInfo **) infos[i].descriptorInfos)[j];
				break;
			}

			++writeDescriptorSetOffset;
		}
	}

	vkUpdateDescriptorSets(device, bindingCount, writeDescriptorSets, 0, NULL);

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
