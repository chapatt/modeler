#ifndef MODELER_DESCRIPTOR_H
#define MODELER_DESCRIPTOR_H

#include <stdbool.h>
#include <vulkan/vulkan.h>

typedef struct createDescriptorSetInfo_t {
	VkDescriptorType type;
	VkShaderStageFlags stageFlags;
	void *descriptorInfos;
	size_t descriptorCount;
	VkDescriptorSetLayoutBinding *bindings;
	size_t bindingCount;
} CreateDescriptorSetInfo;

bool createDescriptorSets(
	VkDevice device,
	CreateDescriptorSetInfo *infos,
	size_t descriptorSetCount,
	VkDescriptorPool *descriptorPool,
	VkDescriptorSet *descriptorSets,
	VkDescriptorSetLayout *descriptorSetLayouts,
	char **error
);

void destroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout);

void destroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool);

#endif /* MODELER_DESCRIPTOR */
