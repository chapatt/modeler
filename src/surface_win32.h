#ifndef SURFACE_WIN32_H
#define SURFACE_WIN32_H

#include <stdbool.h>

#include <vulkan/vulkan.h>

bool createSurfaceWin32(VkInstance instance, HINSTANCE hinstance, HWND hwnd, VkSurfaceKHR *surface, char **error);

#endif /* SURFACE_WIN32_H */