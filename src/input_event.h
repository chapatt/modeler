#ifndef MODELER_INPUT_EVENT_H
#define MODELER_INPUT_EVENT_H

#include "queue.h"

typedef enum input_event_type_t {
	MOUSE_DOWN,
	MOUSE_UP,
	TERMINATE
} InputEventType;

typedef struct input_event_t {
	InputEventType type;
} InputEvent;

void enqueueInputEvent(Queue *queue, InputEventType);

#endif /* MODELER_INPUT_EVENT_H */