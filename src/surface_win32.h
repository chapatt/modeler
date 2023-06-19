#ifndef SURFACE_WIN32_H
#define SURFACE_WIN32_H

#include <vulkan/vulkan.h>

VkSurfaceKHR createSurfaceWin32(VkInstance instance, HINSTANCE hinstance, HWND hwnd);

#endif /* SURFACE_WIN32_H */