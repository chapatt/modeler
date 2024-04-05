#ifndef MODELER_INSTANCE_H
#define MODELER_INSTANCE_H

#include <vulkan/vulkan.h>

bool createInstance(const char **extensions, size_t extensionCount, VkInstance *instance, VkDebugReportCallbackEXT *debugCallback, char **error);
void destroyInstance(VkInstance instance, VkDebugReportCallbackEXT debugCallback);

#endif /* MODELER_INSTANCE_H */
