#ifndef MODELER_INPUT_EVENT_H
#define MODELER_INPUT_EVENT_H

#include "queue.h"

typedef enum input_event_type_t {
	MOUSE_DOWN,
	MOUSE_UP,
	MOUSE_MOVE,
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

#endif /* MODELER_INPUT_EVENT_H */