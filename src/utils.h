#ifndef MODELER_UTILS_H
#define MODELER_UTILS_H

#include <stdarg.h>
#include <stdbool.h>
#include <vulkan/vulkan.h>

int asprintf(char **strp, const char *fmt, ...);
int vasprintf(char **strp, const char *fmt, va_list ap);
long readFileToString(const char *path, char **bytes);
VkExtent2D getWindowExtent(void *platformWindow);
float getWindowScale(void *platformWindow);
void srgbToLinear(float vector[3]);

#endif /* MODELER_UTILS_H */
