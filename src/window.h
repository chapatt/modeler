#ifndef MODELER_WINDOW_H
#define MODELER_WINDOW_H

#include <stdbool.h>

#include "vulkan_utils.h"

typedef struct window_dimensions_t {
	VkExtent2D surfaceArea;
	VkRect2D activeArea;
	int cornerRadius;
	float scale;
	bool fullscreen;
} WindowDimensions;

typedef struct resize_info_t {
	WindowDimensions windowDimensions;
	void *platformWindow;
} ResizeInfo;

#endif /* MODELER_WINDOW_H */