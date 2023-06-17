#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "instance.h"
#include "utils.h"

bool areInstanceExtensionsSupported(const char **extensions, size_t extensionCount);
bool areLayersSupported(const char **layers, size_t layerCount);

VkInstance createInstance(const char **extensions, size_t extensionCount)
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

        size_t independentExtensionCount = 2;
        const char *independentExtensions[] = {
                VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
                VK_KHR_SURFACE_EXTENSION_NAME
        };
        createInfo.enabledExtensionCount = extensionCount;
        const char **requiredExtensions = (const char **) malloc(sizeof(char *) * (createInfo.enabledExtensionCount + 1));
        for (size_t i = 0; i < extensionCount; ++i) {     
            requiredExtensions[i] = extensions[i];
        }
        if (!areInstanceExtensionsSupported(requiredExtensions, createInfo.enabledExtensionCount)) {
                fprintf(stderr, "Required instance extensions not available!");
                exit(EXIT_FAILURE);
        }
        const char *optionalExtension = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
        if (areInstanceExtensionsSupported(&optionalExtension, 1)) {
                requiredExtensions[createInfo.enabledExtensionCount] = optionalExtension;
                createInfo.enabledExtensionCount++;
                createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        }
        createInfo.ppEnabledExtensionNames = requiredExtensions;
    
        VkInstance instance;
        if (vkCreateInstance(&createInfo, VK_NULL_HANDLE, &instance) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create instance: ");
            exit(EXIT_FAILURE);
        }

        free(requiredExtensions);

        return instance;
}

void destroyInstance(VkInstance instance)
{
        vkDestroyInstance(instance, NULL);
}

bool areInstanceExtensionsSupported(const char **extensions, size_t extensionCount)
{
        uint32_t availableExtensionCount;
        if (vkEnumerateInstanceExtensionProperties(NULL, &availableExtensionCount, NULL) != VK_SUCCESS) {
                fprintf(stderr, "Failed to get available instance extension count!\n");
                exit(EXIT_FAILURE);
        }

        VkExtensionProperties *availableExtensions = (VkExtensionProperties *) malloc(sizeof(VkExtensionProperties) * availableExtensionCount);
        if (vkEnumerateInstanceExtensionProperties(NULL, &availableExtensionCount, availableExtensions) != VK_SUCCESS) {
                fprintf(stderr, "Failed to get available instance extensions!\n");
                exit(EXIT_FAILURE);
        }

        bool match = compareExtensions(extensions, extensionCount, availableExtensions, availableExtensionCount);

        free(availableExtensions);

        return match;
}

bool areLayersSupported(const char **layers, size_t layerCount)
{
        uint32_t availableLayerCount;
        if (vkEnumerateInstanceLayerProperties(&availableLayerCount, NULL) != VK_SUCCESS) {
                fprintf(stderr, "Failed to get available instance layer count!\n");
                exit(EXIT_FAILURE);
        }

        VkLayerProperties *availableLayers = (VkLayerProperties *) malloc(sizeof(VkLayerProperties) * availableLayerCount);
        if (vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers) != VK_SUCCESS) {
                fprintf(stderr, "Failed to get available instance layers!\n");
                exit(EXIT_FAILURE);
        }

        for (size_t i = 0; i < layerCount; ++i) {
                bool layerFound = false;

                for (uint32_t j = 0; j < availableLayerCount; ++j) {
                        if (strcmp(layers[i], availableLayers[j].layerName) == 0) {
                                layerFound = true;
                                break;
                        }
                }

                if (!layerFound) {
                        return false;
                }
        }

        free(availableLayers);

        return true;
}
