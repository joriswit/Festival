#ifndef __QUEUE
#define __QUEUE

struct tree;
struct expansion_data;

typedef struct queue
{
	int *elements;
	int heap_size;
	int max_size;
	tree *t;
} queue;

void free_heap(queue *q);
void reset_heap(queue *q);
void init_queue(queue *q, tree *t);
void heap_insert(queue *q, int val);
int extarct_max(queue *q);
void verify_heap(queue *q);
void print_queue(queue *q);
void remove_value_from_queue(queue *q, int val);

#endif