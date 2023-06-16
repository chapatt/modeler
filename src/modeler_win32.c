#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <windows.h>

#include "modeler_win32.h"
#include "instance.h"
#include "surface_win32.h"
#include "physical_device.h"
#include "device.h"

void initVulkanWin32(HINSTANCE hinstance, HWND hwnd)
{
        const char *platformExtension = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
        VkInstance instance = createInstance(&platformExtension, 1);
        VkSurfaceKHR surface = createSurfaceWin32(instance, hinstance, hwnd);
        VkPhysicalDevice physicalDevice = choosePhysicalDevice(instance, surface);
        VkDevice device = createDevice(physicalDevice);
}