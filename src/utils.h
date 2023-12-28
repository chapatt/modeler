#ifndef MODELER_UTILS_H
#define MODELER_UTILS_H

#include <stdarg.h>
#include <stdbool.h>
#include <vulkan/vulkan.h>

bool compareExtensions(const char **extensions, size_t extensionCount, VkExtensionProperties *availableExtensions, uint32_t availableExtensionCount);
int asprintf(char **strp, const char *fmt, ...);
int vasprintf(char **strp, const char *fmt, va_list ap);
long readFileToString(char *path, char **bytes);

#endif /* MODELER_UTILS_H */