#include <vulkan/vulkan.h>

VkInstance createInstance(const char **platformExtensions, size_t platformExtensionCount);
void destroyInstance(VkInstance instance);