#ifndef MODELER_H
#define MODELER_H

#include "queue.h"

struct threadArguments {
	void *platformWindow;
	Queue *inputQueue;
	char **error;
};

void *threadProc(void *arg);
void sendThreadFailureSignal(void *platformWindow);
void terminateVulkan(Queue *inputQueue, pthread_t thread);

#endif /* MODELER_H */
