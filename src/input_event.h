#ifndef MODELER_INPUT_EVENT_H
#define MODELER_INPUT_EVENT_H

#include <vulkan/vulkan.h>

#include "modeler.h"
#include "queue.h"

typedef enum input_event_type_t {
	BUTTON_DOWN,
	BUTTON_UP,
	POINTER_MOVE,
	POINTER_LEAVE,
	RESIZE,
	TERMINATE
} InputEventType;

typedef struct input_event_t {
	InputEventType type;
	void *data;
} InputEvent;

typedef struct mouse_position_t {
	int x;
	int y;
} MousePosition;

void enqueueInputEvent(Queue *queue, InputEventType type, void *data);
void enqueueInputEventWithPosition(Queue *queue, InputEventType type, int x, int y);
void enqueueInputEventWithExtent(Queue *queue, InputEventType type, int width, int height);
void enqueueInputEventWithWindowDimensions(Queue *queue, InputEventType type, WindowDimensions windowDimensions);

#endif /* MODELER_INPUT_EVENT_H */