#ifndef MODELER_PIPELINE_H
#define MODELER_PIPELINE_H

#include <stdbool.h>
#include <vulkan/vulkan.h>

bool createPipeline(VkDevice device, VkRenderPass renderPass, const char *resourcePath, SwapchainInfo swapchainInfo, VkPipelineLayout *pipelineLayout, VkPipeline *pipeline, char **error);

void destroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout);

void destroyPipeline(VkDevice device, VkPipeline pipeline);

#endif /* MODELER_PIPELINE_H */