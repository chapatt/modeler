#ifndef MODELER_WINDOW_H
#define MODELER_WINDOW_H

#include <stdbool.h>

#include "vulkan_utils.h"

typedef enum orientation_t {
	ROTATE_0,
	ROTATE_90,
	ROTATE_180,
	ROTATE_270
} Orientation;

typedef struct insets_t {
	int top;
	int right;
	int bottom;
	int left;
} Insets;

typedef struct window_dimensions_t {
	VkExtent2D surfaceArea;
	VkRect2D activeArea;
	int cornerRadius;
	float scale;
	int titlebarHeight;
	bool fullscreen;
	Orientation orientation;
	Insets insets;
} WindowDimensions;

typedef struct resize_info_t {
	WindowDimensions windowDimensions;
	void *platformWindow;
} ResizeInfo;

static inline Orientation negateRotation(Orientation orientation)
{
	switch (orientation) {
	case ROTATE_90:
		return ROTATE_270;
	case ROTATE_270:
		return ROTATE_90;
	case ROTATE_0: case ROTATE_180: default:
		return orientation;
	}
}

void updateWindowDimensionsExtent(WindowDimensions *windowDimensions, VkExtent2D savedExtent);
void applyWindowDimensionsOrientation(WindowDimensions *windowDimensions);
void rotateInsets(Insets *insets, Orientation orientation);
void updateWindowDimensionsInsets(WindowDimensions *windowDimensions, Insets insets);
Orientation transformToOrientation(enum VkSurfaceTransformFlagBitsKHR transform);

#endif /* MODELER_WINDOW_H */
