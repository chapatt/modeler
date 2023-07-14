#ifndef MODELER_METAL_H
#define MODELER_METAL_H

#include <stdbool.h>

bool initVulkanMetal(void *surfaceLayer, int width, int height, const char *resourcePath, char **error);

#endif /* MODELER_METAL_H */
