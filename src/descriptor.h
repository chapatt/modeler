#ifndef MODELER_DESCRIPTOR_H
#define MODELER_DESCRIPTOR_H

#include <stdbool.h>
#include <vulkan/vulkan.h>

typedef struct createDescriptorSetInfo_t {
	VkImageView *imageViews;
	VkImageLayout *imageLayouts;
	VkSampler *imageSamplers;
	size_t imageCount;
	VkBuffer *buffers;
	VkDeviceSize *bufferOffsets;
	VkDeviceSize *bufferRanges;
	size_t bufferCount;
} CreateDescriptorSetInfo;

bool createDescriptorSets(
	VkDevice device,
	CreateDescriptorSetInfo info,
	VkDescriptorPool *descriptorPool,
	VkDescriptorType type,
	VkDescriptorSet **imageDescriptorSets,
	VkDescriptorSetLayout **imageDescriptorSetLayouts,
	VkDescriptorSet **bufferDescriptorSets,
	VkDescriptorSetLayout **bufferDescriptorSetLayouts,
	char **error
);

void destroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout);

void destroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool);

#endif /* MODELER_DESCRIPTOR */
