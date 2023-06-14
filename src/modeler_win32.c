#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <windows.h>

#include "modeler_win32.h"

VkInstance createInstance(void);
void destroyInstance(VkInstance instance);
VkSurfaceKHR createSurface(VkInstance instance, HINSTANCE hinstance, HWND hwnd);
VkPhysicalDevice choosePhysicalDevice(VkInstance, VkSurfaceKHR surface);
bool isDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
VkDevice createLogicalDevice(VkPhysicalDevice physicalDevice);
void destroyLogicalDevice(VkDevice device);
uint32_t findFirstMatchingFamily(VkPhysicalDevice physicalDevice, VkQueueFlags flag);
bool areLayersSupported(const char **layers, size_t layerCount);
bool areInstanceExtensionsSupported(const char **extensions, size_t extensionCount);
bool areDeviceExtensionsSupported(VkPhysicalDevice physicalDevice, const char **extensions, size_t extensionCount);
bool compareExtensions(const char **extensions, size_t extensionCount, VkExtensionProperties *availableExtensions, uint32_t availableExtensionCount);

void initVulkan(HINSTANCE hinstance, HWND hwnd)
{
        VkInstance instance = createInstance();
        VkSurfaceKHR surface = createSurface(instance, hinstance, hwnd);
        VkPhysicalDevice physicalDevice = choosePhysicalDevice(instance, surface);
        VkDevice device = createLogicalDevice(physicalDevice);
}

VkInstance createInstance(void)
{
        VkApplicationInfo applicationInfo = {};
        applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        applicationInfo.pApplicationName = "Hello Triangle";
        applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        applicationInfo.pEngineName = "No Engine";
        applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        applicationInfo.apiVersion = VK_API_VERSION_1_0;
    
        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &applicationInfo;

#ifdef DEBUG
        const char *validationLayers[] = {"VK_LAYER_KHRONOS_validation"};
        if (!areLayersSupported(validationLayers, 1)) {
                fprintf(stderr, "Validation layers not available!");
                exit(EXIT_FAILURE);
        }

        createInfo.enabledLayerCount = 1;
        createInfo.ppEnabledLayerNames = validationLayers;
#endif

        createInfo.enabledExtensionCount = 3;
        const char *requiredExtensions[4] = {
                VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
                VK_KHR_SURFACE_EXTENSION_NAME,
                VK_KHR_WIN32_SURFACE_EXTENSION_NAME
        };
        if (!areInstanceExtensionsSupported(requiredExtensions, 3)) {
                fprintf(stderr, "Required instance extensions not available!");
                exit(EXIT_FAILURE);
        }
        const char *optionalExtension = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
        if (areInstanceExtensionsSupported(&optionalExtension, 1)) {
                ++createInfo.enabledExtensionCount;
                requiredExtensions[3] = optionalExtension;
                createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        }
        createInfo.ppEnabledExtensionNames = requiredExtensions;
    
        VkInstance instance;
        VkResult result = vkCreateInstance(&createInfo, VK_NULL_HANDLE, &instance);
        if (result != VK_SUCCESS) {
            fprintf(stderr, "Failed to create instance: ");
            exit(EXIT_FAILURE);
        }

        return instance;
}

void destroyInstance(VkInstance instance)
{
        vkDestroyInstance(instance, NULL);
}

VkPhysicalDevice choosePhysicalDevice(VkInstance instance, VkSurfaceKHR surface)
{
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);
    
        if (deviceCount == 0) {
            fprintf(stderr, "Failed to find a Physical Device\n");
            exit(EXIT_FAILURE);
        }
    
        VkPhysicalDevice *devices = (VkPhysicalDevice *) malloc(sizeof(VkPhysicalDevice) * deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices);
    
        for (uint32_t i = 0; i < deviceCount; ++i) {
                if (isDeviceSuitable(devices[i], surface)) {
                        physicalDevice = devices[i];
                        break;
                }
        }

        if (physicalDevice == VK_NULL_HANDLE) {
                fprintf(stderr, "Failed to find a suitable GPU!\n");
                exit(EXIT_FAILURE);
        }
    
        return physicalDevice;
}

bool isDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
        bool hasProperties = deviceProperties.limits.maxMemoryAllocationCount >= 1;

        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
        bool hasFeatures = deviceFeatures.robustBufferAccess;
    
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);
    
        VkQueueFamilyProperties *queueFamilies = (VkQueueFamilyProperties *) malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies);
    
        VkQueueFlags compiledFlags = 0;
        bool hasPresent = false;
        for (uint32_t i = 0; i < queueFamilyCount; ++i) {
                compiledFlags |= queueFamilies[i].queueFlags;
                
                VkBool32 familyHasPresent = VK_FALSE;
                VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &familyHasPresent);
                if (result != VK_SUCCESS) {
                        fprintf(stderr, "Failed to check for GPU surface support: ");
                        exit(EXIT_FAILURE);
                }
                hasPresent = hasPresent || familyHasPresent == VK_TRUE;
        }
        free(queueFamilies);
        bool hasQueues = compiledFlags & VK_QUEUE_GRAPHICS_BIT;

        return hasProperties && hasFeatures && hasQueues && hasPresent;
}

VkDevice createLogicalDevice(VkPhysicalDevice physicalDevice)
{
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = findFirstMatchingFamily(physicalDevice, VK_QUEUE_GRAPHICS_BIT);
        queueCreateInfo.queueCount = 1;
        float queuePriority = 1.0f;
        queueCreateInfo.pQueuePriorities = &queuePriority;
    
        VkPhysicalDeviceFeatures deviceFeatures = {};
    
        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = &queueCreateInfo;
        createInfo.queueCreateInfoCount = 1;
        createInfo.pEnabledFeatures = &deviceFeatures;

        const char* deviceExtension = "VK_KHR_portability_subset";
        if (areDeviceExtensionsSupported(physicalDevice, &deviceExtension, 1)) {
                createInfo.ppEnabledExtensionNames = &deviceExtension;
                createInfo.enabledExtensionCount = 1;
        }

        createInfo.enabledLayerCount = 0;
    
        VkDevice device;
        VkResult result = vkCreateDevice(physicalDevice, &createInfo, NULL, &device);
        if (result != VK_SUCCESS) {
                fprintf(stderr, "Failed to create logical device!\n");
                exit(EXIT_FAILURE);
        }
    
        VkQueue queue;
        vkGetDeviceQueue(device, queueCreateInfo.queueFamilyIndex, 0, &queue);

        return device;
}

void destroyLogicalDevice(VkDevice device)
{
        vkDestroyDevice(device, NULL);
}

/* Not guaranteed to find a match (0 returned if none are found) */
uint32_t findFirstMatchingFamily(VkPhysicalDevice physicalDevice, VkQueueFlags flags)
{
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);

        VkQueueFamilyProperties *queueFamilies = (VkQueueFamilyProperties *) malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies);

        VkQueueFlags match = 0;
        for (uint32_t i = 0; i < queueFamilyCount; ++i) {
                if (queueFamilies[i].queueFlags & flags) {
                        match = i;
                        break;
                }
        }
        free(queueFamilies);
    
        return match;
}

VkSurfaceKHR createSurface(VkInstance instance, HINSTANCE hinstance, HWND hwnd)
{
        VkWin32SurfaceCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hwnd = hwnd;
        createInfo.hinstance = hinstance;
    
        VkSurfaceKHR surface;
        VkResult result;
        result = vkCreateWin32SurfaceKHR(instance, &createInfo, NULL, &surface);
        if (result != VK_SUCCESS) {
                fprintf(stderr, "Failed to create surface!\n");
                exit(EXIT_FAILURE);
        }
    
        return surface;
}

bool areLayersSupported(const char **layers, size_t layerCount)
{
        uint32_t availableLayerCount;
        vkEnumerateInstanceLayerProperties(&availableLayerCount, NULL);

        VkLayerProperties *availableLayers = (VkLayerProperties *) malloc(sizeof(VkLayerProperties) * availableLayerCount);
        vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers);

        while (layerCount--) {
                bool layerFound = false;

                while (availableLayerCount--) {
                        if (strcmp(layers[layerCount], availableLayers[availableLayerCount].layerName) == 0) {
                                layerFound = true;
                                break;
                        }
                }

                if (!layerFound) {
                        return false;
                }
        }

        return true;
}

bool areInstanceExtensionsSupported(const char **extensions, size_t extensionCount)
{
        uint32_t availableExtensionCount;
        vkEnumerateInstanceExtensionProperties(NULL, &availableExtensionCount, NULL);

        VkExtensionProperties *availableExtensions = (VkExtensionProperties *) malloc(sizeof(VkExtensionProperties) * availableExtensionCount);
        vkEnumerateInstanceExtensionProperties(NULL, &availableExtensionCount, availableExtensions);

        return compareExtensions(extensions, extensionCount, availableExtensions, availableExtensionCount);
}

bool areDeviceExtensionsSupported(VkPhysicalDevice physicalDevice, const char **extensions, size_t extensionCount)
{
        uint32_t availableExtensionCount;
        vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &availableExtensionCount, NULL);

        VkExtensionProperties *availableExtensions = (VkExtensionProperties *) malloc(sizeof(VkExtensionProperties) * availableExtensionCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &availableExtensionCount, availableExtensions);

        return compareExtensions(extensions, extensionCount, availableExtensions, availableExtensionCount);
}

bool compareExtensions(const char **extensions, size_t extensionCount, VkExtensionProperties *availableExtensions, uint32_t availableExtensionCount)
{
        while (extensionCount--) {
                bool extensionFound = false;

                while (availableExtensionCount--) {
                        if (strcmp(extensions[extensionCount], availableExtensions[availableExtensionCount].extensionName) == 0) {
                                extensionFound = true;
                                break;
                        }
                }

                if (!extensionFound) {
                        return false;
                }
        }

        return true;
}
