#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
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

int asprintf(char **strp, const char *fmt, ...)
{
	va_list ap;
	int rc;

	va_start(ap, fmt);
	rc = vasprintf(strp, fmt, ap);
	va_end(ap);

	return rc;
}

int vasprintf(char **strp, const char *fmt, va_list ap)
{
	int expstrlen;
	va_list tmpap;

	va_copy(tmpap, ap);
	if ((expstrlen = vsnprintf(NULL, 0, fmt, tmpap)) < 0) {
		return expstrlen;
	}
	va_end(tmpap);

	if (!(*strp = malloc(expstrlen + 1))) {
		return -1;
	}

	return vsnprintf(*strp, expstrlen + 1, fmt, ap);
}
