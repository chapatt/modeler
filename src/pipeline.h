#ifndef MODELER_PIPELINE_H
#define MODELER_PIPELINE_H

#include <stdbool.h>
#include <vulkan/vulkan.h>

typedef struct push_constants_t {
	float extent[2];
	float offset[2];
	float cornerRadius;
} PushConstants;

bool createPipeline(VkDevice device, VkRenderPass renderPass, uint32_t subpassIndex, const char *vertexShaderBytes, long vertexShaderSize, const char *fragmentShaderBytes, long fragmentShaderSize, VkExtent2D extent, VkDescriptorSetLayout *descriptorSetLayouts, uint32_t descriptorSetLayoutCount, VkPipelineLayout *pipelineLayout, VkPipeline *pipeline, char **error);

void destroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout);

void destroyPipeline(VkDevice device, VkPipeline pipeline);

#endif /* MODELER_PIPELINE_H */