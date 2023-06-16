#include <stdbool.h>
#include <string.h>

#include "utils.h"

bool compareExtensions(const char **extensions, size_t extensionCount, VkExtensionProperties *availableExtensions, uint32_t availableExtensionCount)
{
        for (size_t i = 0; i < extensionCount; ++i) {
                bool extensionFound = false;

                for (uint32_t j = 0; j < availableExtensionCount; ++j) {
                        if (strcmp(extensions[i], availableExtensions[j].extensionName) == 0) {
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
