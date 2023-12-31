#ifndef __DRAGONFLY_QUEUE
#define __DRAGONFLY_QUEUE

#include "dragonfly.h"

typedef struct dragonfly_queue
{
	int* elements;
	int heap_size;
	int max_size;
	dragonfly_node *n;
} dragonfly_queue;

void dragonfly_free_heap(dragonfly_queue* q);
void dragonfly_reset_heap(dragonfly_queue* q);
void dragonfly_init_queue(dragonfly_queue* q, dragonfly_node *n);
void dragonfly_heap_insert(dragonfly_queue* q, int val);
int  dragonfly_extarct_max(dragonfly_queue* q);
void dragonfly_print_queue(dragonfly_queue* q);


#endif