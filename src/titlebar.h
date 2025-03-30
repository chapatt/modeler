#ifndef MODELER_TITLEBAR_H
#define MODELER_TITLEBAR_H

#include <stdbool.h>

typedef struct titlebar_t *Titlebar;

#include "input_event.h"
#include "window.h"
#include "buffer.h"
#include "vulkan_utils.h"
#include "vk_mem_alloc.h"

bool createTitlebar(Titlebar *titlebar, VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, VkRenderPass renderPass, uint32_t subpass, VkSampleCountFlagBits sampleCount, const char *resourcePath, float aspectRatio, float height, char **error);
bool drawTitlebar(Titlebar self, VkCommandBuffer commandBuffer, char **error);
void destroyTitlebar(Titlebar self);
void titlebarHandleInputEvent(void *titlebar, InputEvent *inputEvent);
void titlebarSetAspectRatio(Titlebar self, float aspectRatio);
void titlebarSetHeight(Titlebar self, float height);

#endif /* MODELER_TITLEBAR_H */
