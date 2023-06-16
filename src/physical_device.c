#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "physical_device.h"

bool isPhysicalDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

VkPhysicalDevice choosePhysicalDevice(VkInstance instance, VkSurfaceKHR surface)
{
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        uint32_t deviceCount = 0;
        if (vkEnumeratePhysicalDevices(instance, &deviceCount, NULL) != VK_SUCCESS) {
            fprintf(stderr, "Failed to get physical device count!");
            exit(EXIT_FAILURE);
        }
    
        if (deviceCount == 0) {
            fprintf(stderr, "Failed to find a Physical Device\n");
            exit(EXIT_FAILURE);
        }
    
        VkPhysicalDevice *devices = (VkPhysicalDevice *) malloc(sizeof(VkPhysicalDevice) * deviceCount);
        if (vkEnumeratePhysicalDevices(instance, &deviceCount, devices) != VK_SUCCESS) {
            fprintf(stderr, "Failed to get physical devices!");
            exit(EXIT_FAILURE);
        }
    
        for (uint32_t i = 0; i < deviceCount; ++i) {
                if (isPhysicalDeviceSuitable(devices[i], surface)) {
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

bool isPhysicalDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
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