#include <stdbool.h>
#include <vulkan/vulkan.h>

bool compareExtensions(const char **extensions, size_t extensionCount, VkExtensionProperties *availableExtensions, uint32_t availableExtensionCount);