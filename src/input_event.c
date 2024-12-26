#include <stdio.h>
#include <stdlib.h>

#include "input_event.h"

void enqueueInputEvent(Queue *queue, InputEventType type, void *data)
{
	InputEvent *event = malloc(sizeof(*event));
	*event = (const InputEvent) {
		.type = type,
		.data = data
	};

	enqueue(queue, (void *) event);
}

void enqueueInputEventWithPosition(Queue *queue, InputEventType type, int x, int y)
{
	MousePosition *position = malloc(sizeof(*position));
	*position = (const MousePosition) {
		.x = x,
		.y = y
	};
	enqueueInputEvent(queue, type, position);
}

void enqueueInputEventWithExtent(Queue *queue, InputEventType type, int width, int height)
{
	VkExtent2D *extent = malloc(sizeof(extent));
	*extent = (const VkExtent2D) {
		.width = width,
		.height = height
	};
	enqueueInputEvent(queue, type, extent);
}

void enqueueInputEventWithWindowDimensions(Queue *queue, InputEventType type, WindowDimensions windowDimensions)
{
	WindowDimensions *windowDimensionsP = malloc(sizeof(*windowDimensionsP));
	*windowDimensionsP = windowDimensions;
	enqueueInputEvent(queue, type, windowDimensionsP);
}

void enqueueInputEventWithResizeInfo(Queue *queue, InputEventType type, ResizeInfo resizeInfo)
{
	ResizeInfo *resizeInfoP = malloc(sizeof(*resizeInfoP));
	*resizeInfoP = resizeInfo;
	enqueueInputEvent(queue, type, resizeInfoP);
}

bool isPointerOnViewport(VkViewport viewport, MousePosition position)
{
	return position.x > viewport.x &&
		position.x < viewport.x + viewport.width &&
		position.y > viewport.y &&
		position.y < viewport.y + viewport.height;
}

NormalizedMousePosition normalizeMousePosition(VkViewport viewport, MousePosition position)
{
	return (NormalizedMousePosition) {
		.x = (position.x - viewport.x) / (float) viewport.width,
		.y = (position.y - viewport.y) / (float) viewport.height,
	};
}
