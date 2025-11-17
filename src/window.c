#include "window.h"

void updateWindowDimensionsExtent(WindowDimensions *windowDimensions, VkExtent2D savedExtent)
{
	uint32_t horizontalMargin = windowDimensions->surfaceArea.width - windowDimensions->activeArea.extent.width;
	uint32_t verticalMargin = windowDimensions->surfaceArea.height - windowDimensions->activeArea.extent.height;
	windowDimensions->surfaceArea = savedExtent;
	windowDimensions->activeArea.extent.width = savedExtent.width - horizontalMargin;
	windowDimensions->activeArea.extent.height = savedExtent.height - verticalMargin;
}

void applyWindowDimensionsOrientation(WindowDimensions *windowDimensions)
{
	uint32_t oldSurfaceAreaWidth = windowDimensions->surfaceArea.width;
	uint32_t oldActiveAreaWidth = windowDimensions->activeArea.extent.width;
	int32_t oldActiveAreaOffsetX = windowDimensions->activeArea.offset.x;

	switch (windowDimensions->orientation) {
		case ROTATE_90:
			windowDimensions->surfaceArea.width = windowDimensions->surfaceArea.height;
			windowDimensions->surfaceArea.height = oldSurfaceAreaWidth;

			windowDimensions->activeArea.extent.width = windowDimensions->activeArea.extent.height;
			windowDimensions->activeArea.extent.height = oldActiveAreaWidth;

			windowDimensions->activeArea.offset.x = (windowDimensions->surfaceArea.width - windowDimensions->activeArea.extent.width) - windowDimensions->activeArea.offset.y;
			windowDimensions->activeArea.offset.y = oldActiveAreaOffsetX;

			break;
		case ROTATE_270:
			windowDimensions->surfaceArea.width = windowDimensions->surfaceArea.height;
			windowDimensions->surfaceArea.height = oldSurfaceAreaWidth;

			windowDimensions->activeArea.extent.width = windowDimensions->activeArea.extent.height;
			windowDimensions->activeArea.extent.height = oldActiveAreaWidth;

			windowDimensions->activeArea.offset.x = windowDimensions->activeArea.offset.y;
			windowDimensions->activeArea.offset.y = (windowDimensions->surfaceArea.height - windowDimensions->activeArea.extent.height) - windowDimensions->activeArea.offset.x;

			break;
	}
}

void updateWindowDimensionsInsets(WindowDimensions *windowDimensions, Insets insets)
{
	windowDimensions->activeArea.offset.x = insets.left;
	windowDimensions->activeArea.offset.y = insets.top;
	windowDimensions->activeArea.extent.width = windowDimensions->surfaceArea.width - (insets.left + insets.right);
	windowDimensions->activeArea.extent.height = windowDimensions->surfaceArea.height - (insets.top + insets.bottom);
}

void rotateInsets(Insets *insets, Orientation orientation)
{
	Insets oldInsets = *insets;

	switch (orientation) {
	case ROTATE_0:
		break;
	case ROTATE_90:
		insets->right = oldInsets.bottom;
		insets->bottom = oldInsets.left;
		insets->left = oldInsets.top;
		insets->top = oldInsets.right;
		break;
	case ROTATE_180:
		insets->right = oldInsets.left;
		insets->bottom = oldInsets.top;
		insets->left = oldInsets.right;
		insets->top = oldInsets.bottom;
		break;
	case ROTATE_270:
		insets->right = oldInsets.top;
		insets->bottom = oldInsets.right;
		insets->left = oldInsets.bottom;
		insets->top = oldInsets.left;
		break;
	}
}