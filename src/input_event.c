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
