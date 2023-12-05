#include <stdlib.h>

#include "queue.h"

Queue *createQueue(void)
{
	Queue *queue = (Queue *) malloc(sizeof(Queue));
	initializeQueue(queue);
	return queue;
}

/*
 * initialize(Q: pointer to queue_t)
 * 	node = new_node()		// Allocate a free node
 * 	node->next = NULL		// Make it the only node in the linked list
 * 	Q->Head = Q->Tail = node	// Both Head and Tail point to it
 * 	Q->H_lock = Q->T_lock = FREE	// Locks are initially free
 */
void initializeQueue(Queue *queue)
{
	Node *node = (struct node *) malloc(sizeof(struct node));
	*node = (Node) {
		.value = NULL,
		.next = NULL
	};
	*queue = (Queue) {
		.head = node,
		.tail = node
	};
	pthread_mutex_init(&queue->headLock, NULL);
	pthread_mutex_init(&queue->tailLock, NULL);
}
  
/*
 * enqueue(Q: pointer to queue_t, value: data type)
 * 	node = new_node()	// Allocate a new node from the free list
 *	node->value = value	// Copy enqueued value into node
 *	node->next = NULL	// Set next pointer of node to NULL
 *	lock(&Q->T_lock)	// Acquire T_lock in order to access Tail
 *	Q->Tail->next = node	// Link node at the end of the linked list
 *	Q->Tail = node		// Swing Tail to node
 *	unlock(&Q->T_lock)	// Release T_lock
 */
void enqueue(Queue *queue, void *value)
{
	Node *node = (struct node *) malloc(sizeof(struct node));
	*node = (Node) {
		.value = value,
		.next = NULL
	};
	pthread_mutex_lock(&queue->tailLock);
	queue->tail->next = node;
	queue->tail = node;
	pthread_mutex_unlock(&queue->tailLock);
}
  
/*
 * dequeue(Q: pointer to queue_t, pvalue: pointer to data type): boolean
 * 	lock(&Q->H_lock)		// Acquire H_lock in order to access Head
 * 	node = Q->Head			// Read Head
 * 	new_head = node->next		// Read next pointer
 * 	if new_head == NULL		// Is queue empty?
 * 		unlock(&Q->H_lock)	// Release H_lock before return
 * 		return FALSE		// Queue was empty
 * 	endif
 * 	*pvalue = new_head->value	// Queue not empty. Read value before release
 * 	Q->Head = new_head		// Swing Head to next node
 * 	unlock(&Q->H_lock)		// Release H_lock
 * 	free(node)			// Free node
 * 	return TRUE			// Queue was not empty, dequeue succeeded
 */
bool dequeue(Queue *queue, void **value)
{
	pthread_mutex_lock(&queue->headLock);
	Node *node = queue->head;
	Node *newHead = node->next;

	if (!newHead) {
		pthread_mutex_unlock(&queue->headLock);
		return false;
	}

	*value = newHead->value;
	queue->head = newHead;
	pthread_mutex_unlock(&queue->headLock);
	free(node);

	return true;
}