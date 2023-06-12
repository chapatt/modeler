#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_metal.h>

#include "modeler.h"

VkInstance createInstance(void);
void destroyInstance(VkInstance instance);
VkSurfaceKHR createSurface(VkInstance instance, void *layer);
VkPhysicalDevice choosePhysicalDevice(VkInstance, VkSurfaceKHR surface);
bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
VkDevice createLogicalDevice(VkPhysicalDevice physicalDevice);
void destroyLogicalDevice(VkDevice device);
uint32_t findFirstMatchingFamily(VkPhysicalDevice device, VkQueueFlags flag);

void initVulkan(void *surfaceLayer)
{
        printf("metal layer pointer: %ld\n", (long) surfaceLayer);
    
        VkInstance instance = createInstance();
        VkSurfaceKHR surface = createSurface(instance, surfaceLayer);
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
        createInfo.enabledLayerCount = 0;
        createInfo.enabledExtensionCount = 4;
        const char *requiredExtensions[] = {
                VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
                VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
                VK_KHR_SURFACE_EXTENSION_NAME,
                VK_EXT_METAL_SURFACE_EXTENSION_NAME
        };
        createInfo.ppEnabledExtensionNames = requiredExtensions;
        createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    
        VkInstance instance;
        VkResult result = vkCreateInstance(&createInfo, VK_NULL_HANDLE, &instance);
        if (result != VK_SUCCESS) {
            printf("Failed to create instance: ");
            
            switch (result) {
                case VK_ERROR_OUT_OF_HOST_MEMORY:
                    printf("VK_ERROR_OUT_OF_HOST_MEMORY\n");
                    break;
                case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                    printf("VK_ERROR_OUT_OF_DEVICE_MEMORY\n");
                    break;
                case VK_ERROR_INITIALIZATION_FAILED:
                    printf("VK_ERROR_INITIALIZATION_FAILED\n");
                    break;
                case VK_ERROR_LAYER_NOT_PRESENT:
                    printf("VK_ERROR_LAYER_NOT_PRESENT\n");
                    break;
                case VK_ERROR_EXTENSION_NOT_PRESENT:
                    printf("VK_ERROR_EXTENSION_NOT_PRESENT\n");
                    break;
                case VK_ERROR_INCOMPATIBLE_DRIVER:
                    printf("VK_ERROR_INCOMPATIBLE_DRIVER\n");
                    break;
                default:
                    printf("UNKNOWN\n");
                    break;
            }
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
            printf("Failed to find a Physical Device\n");
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
                printf("Failed to find a suitable GPU!\n");
                return NULL;
        }
    
        return physicalDevice;
}

bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface)
{
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        bool hasProperties = deviceProperties.limits.maxMemoryAllocationCount >= 1000000;

        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
        bool hasFeatures = deviceFeatures.robustBufferAccess;
    
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);
    
        VkQueueFamilyProperties *queueFamilies = (VkQueueFamilyProperties *) malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);
    
        VkQueueFlags compiledFlags = 0;
        VkBool32 hasPresent = false;
        for (uint32_t i = 0; i < queueFamilyCount; ++i) {
                compiledFlags |= queueFamilies[i].queueFlags;
                
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &hasPresent);
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
        createInfo.ppEnabledExtensionNames = &deviceExtension;
        createInfo.enabledExtensionCount = 1;
        createInfo.enabledLayerCount = 0;
    
        VkDevice device;
        VkResult result = vkCreateDevice(physicalDevice, &createInfo, NULL, &device);
        if (result != VK_SUCCESS) {
                printf("Failed to create logical device!\n");
                return NULL;
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
uint32_t findFirstMatchingFamily(VkPhysicalDevice device, VkQueueFlags flags)
{
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);

        VkQueueFamilyProperties *queueFamilies = (VkQueueFamilyProperties *) malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);

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

VkSurfaceKHR createSurface(VkInstance instance, void *layer)
{
    VkMetalSurfaceCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
    createInfo.pLayer = layer;
    
    VkSurfaceKHR surface;
    VkResult result;
    result = vkCreateMetalSurfaceEXT(instance, &createInfo, NULL, &surface);
    if (result != VK_SUCCESS) {
            printf("Failed to create surface!\n");
            return NULL;
    }
    
    return surface;
}
