#include <vulkan/vulkan.h>

VkInstance createInstance(const char **extensions, size_t extensionCount);
void destroyInstance(VkInstance instance);