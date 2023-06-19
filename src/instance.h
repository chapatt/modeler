#ifndef MODELER_INSTANCE_H
#define MODELER_INSTANCE_H

#include <vulkan/vulkan.h>

bool createInstance(const char **extensions, size_t extensionCount, VkInstance *instance, char **error);
void destroyInstance(VkInstance instance);

#endif /* MODELER_INSTANCE_H */