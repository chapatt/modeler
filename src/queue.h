#ifndef MODELER_QUEUE_H
#define MODELER_QUEUE_H

#include <stdbool.h>
#include <pthread.h>

/*
 * structure node_t {
 * 	value: data type,
 * 	next: pointer to node_t
 * }
 */
typedef struct node {
	void *value;
	struct node *next;
} Node;

/*
 * structure queue_t {
 * 	Head: pointer to node_t,
 * 	Tail: pointer to node_t,
 * 	H_lock: lock type,
 * 	T_lock: lock type
 * }
 */
typedef struct queue {
	Node *head;
	Node *tail;
	pthread_mutex_t headLock;
	pthread_mutex_t tailLock;
} Queue;

Queue *createQueue(void);
void initialize(Queue *queue);
void enqueue(Queue *queue, void *value);
bool dequeue(Queue *queue, void **value);

#endif /* MODELER_QUEUE_H */