#include <stdlib.h>

#include "inputEvent.h"

void enqueueInputEvent(Queue *queue, InputEventType type)
{
	InputEvent *event = (InputEvent *) malloc(sizeof(InputEvent));
	event->type = type;

	enqueue(queue, (void *) event);
}