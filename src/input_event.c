#include <stdio.h>
#include <stdlib.h>

#include "input_event.h"

void enqueueInputEvent(Queue *queue, InputEventType type)
{
	InputEvent *event = malloc(sizeof(*event));
	event->type = type;

	enqueue(queue, (void *) event);
}
