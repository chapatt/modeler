#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "instance.h"
#include "utils.h"
#include "vulkan_utils.h"

typedef enum support_result_t {
	SUPPORT_ERROR = -1,
	SUPPORT_UNSUPPORTED = 0,
	SUPPORT_SUPPORTED = 1
} SupportResult;

SupportResult areInstanceExtensionsSupported(const char **extensions, size_t extensionCount, char **error);
SupportResult areLayersSupported(const char **layers, size_t layerCount, char **error);
static VKAPI_ATTR VkBool32 VKAPI_CALL debugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT object_type, uint64_t object, size_t location, int32_t message_code, const char *layer_prefix, const char *message, void *user_data);

bool createInstance(const char **extensions, size_t extensionCount, VkInstance *instance, VkDebugReportCallbackEXT *debugCallback, char **error)
{
	VkApplicationInfo applicationInfo = {};
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.pApplicationName = "modeler";
	applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	applicationInfo.pEngineName = "No Engine";
	applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	applicationInfo.apiVersion = VK_API_VERSION_1_1;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &applicationInfo;

#ifdef DEBUG
	const char *validationLayers[] = {"VK_LAYER_KHRONOS_validation"};
	switch (areLayersSupported(validationLayers, 1, error)) {
	case SUPPORT_ERROR:
		return false;
	case SUPPORT_UNSUPPORTED:
		asprintf(error, "Validation layers not available!");
		return false;
	}

	createInfo.enabledLayerCount = 1;
	createInfo.ppEnabledLayerNames = validationLayers;
#endif

	createInfo.enabledExtensionCount = extensionCount;
	const char **requiredExtensions = malloc(sizeof(*requiredExtensions) * (createInfo.enabledExtensionCount + 1));
	for (size_t i = 0; i < extensionCount; ++i) {
	    requiredExtensions[i] = extensions[i];
	}
	switch (areInstanceExtensionsSupported(requiredExtensions, createInfo.enabledExtensionCount, error)) {
	case SUPPORT_ERROR:
		return false;
	case SUPPORT_UNSUPPORTED:
		asprintf(error, "Required instance extensions not available!");
		return false;
	}
	const char *optionalExtension = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
	switch (areInstanceExtensionsSupported(&optionalExtension, 1, error)) {
	case SUPPORT_ERROR:
		return false;
	case SUPPORT_SUPPORTED:
		requiredExtensions[createInfo.enabledExtensionCount] = optionalExtension;
		createInfo.enabledExtensionCount++;
		createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
	}
	createInfo.ppEnabledExtensionNames = requiredExtensions;

	VkResult result;
	if ((result = vkCreateInstance(&createInfo, VK_NULL_HANDLE, instance)) != VK_SUCCESS) {
		asprintf(error, "Failed to create instance: %s", string_VkResult(result));
		return false;
	}

#ifdef DEBUG
	PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT) (vkGetInstanceProcAddr(*instance, "vkCreateDebugReportCallbackEXT"));
	VkDebugReportCallbackCreateInfoEXT debugReportCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
		.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT,
		.pfnCallback = (PFN_vkDebugReportCallbackEXT) debugReportCallback
	};
	if ((result = vkCreateDebugReportCallbackEXT(*instance, &debugReportCreateInfo, NULL, debugCallback)) != VK_SUCCESS) {
		asprintf(error, "Failed to create debug report callback: %s", string_VkResult(result));
		return false;
	}
#endif /* DEBUG */

	free(requiredExtensions);

	return true;
}

void destroyInstance(VkInstance instance, VkDebugReportCallbackEXT debugCallback)
{
#ifdef DEBUG
	PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT) (vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT"));
	vkDestroyDebugReportCallbackEXT(instance, debugCallback, NULL);
#endif /* DEBUG */

	vkDestroyInstance(instance, NULL);
}

SupportResult areInstanceExtensionsSupported(const char **extensions, size_t extensionCount, char **error)
{
	VkResult result;
	uint32_t availableExtensionCount;
	if ((result = vkEnumerateInstanceExtensionProperties(NULL, &availableExtensionCount, NULL)) != VK_SUCCESS) {
		asprintf(error, "Failed to get available instance extension count: %s", string_VkResult(result));
		return SUPPORT_ERROR;
	}

	VkExtensionProperties *availableExtensions = malloc(sizeof(*availableExtensions) * availableExtensionCount);
	if ((result = vkEnumerateInstanceExtensionProperties(NULL, &availableExtensionCount, availableExtensions)) != VK_SUCCESS) {
		asprintf(error, "Failed to get available instance extensions: %s", string_VkResult(result));
		return SUPPORT_ERROR;
	}

	bool match = compareExtensions(extensions, extensionCount, availableExtensions, availableExtensionCount);

	free(availableExtensions);

	return match ? SUPPORT_SUPPORTED : SUPPORT_UNSUPPORTED;
}

SupportResult areLayersSupported(const char **layers, size_t layerCount, char **error)
{
	VkResult result;
	uint32_t availableLayerCount;
	if ((result = vkEnumerateInstanceLayerProperties(&availableLayerCount, NULL)) != VK_SUCCESS) {
		asprintf(error, "Failed to get available instance layer count: %s", string_VkResult(result));
		return SUPPORT_ERROR;
	}

	VkLayerProperties *availableLayers = malloc(sizeof(*availableLayers) * availableLayerCount);
	if ((result = vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers)) != VK_SUCCESS) {
		asprintf(error, "Failed to get available instance layers: %s", string_VkResult(result));
		return SUPPORT_ERROR;
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
			return SUPPORT_UNSUPPORTED;
		}
	}

	free(availableLayers);

	return SUPPORT_SUPPORTED;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT object_type, uint64_t object, size_t location, int32_t message_code, const char *layer_prefix, const char *message, void *user_data)
{
	printf("%s\n", message);

	return VK_FALSE;
}
