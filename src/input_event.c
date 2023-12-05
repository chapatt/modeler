#include <stdio.h>
#include <stdlib.h>

#include "input_event.h"

void enqueueInputEvent(Queue *queue, InputEventType type)
{
	InputEvent *event = (InputEvent *) malloc(sizeof(InputEvent));
	event->type = type;

	enqueue(queue, (void *) event);
}
