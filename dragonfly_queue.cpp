#include <stdlib.h>
#include <stdio.h>

#include "util.h"
#include "dragonfly_queue.h"

#define Q_LEFT(i) (((i)* 2) + 1)
#define Q_RIGHT(i) (((i)* 2) + 2)
#define Q_PARENT(i) (((i) - 1) / 2)

void dragonfly_init_queue(dragonfly_queue* q, dragonfly_node* n)
{
	q->max_size = 64;
	q->elements = (int*)malloc(sizeof(int) * q->max_size);
	q->heap_size = 0;

	if (q->elements == 0) exit_with_error("can't allocate elements");
	q->n = n;
}

int is_better_dragonfly_score(dragonfly_node* e1, dragonfly_node* e2)
{
	if (e1->dist < e2->dist) return 1;
	if (e1->dist > e2->dist) return 0;

	if (e1->packed > e2->packed) return 1;
	if (e1->packed < e2->packed) return 0;

	return 0;
}


int dragonfly_is_better_element(dragonfly_queue* q, int first, int second)
{
	dragonfly_node *e1, *e2;

	e1 = q->n + first;
	e2 = q->n + second;

	if (is_better_dragonfly_score(e1,e2)) return 1;
	if (is_better_dragonfly_score(e2,e1)) return 0;

	if (first < second) return 1;
	return 0;
}


void dragonfly_heapify(dragonfly_queue* q, int i)
{
	int* elements;
	int l, r, tmp;
	int largest;

	if (q->heap_size == 0) return;

	elements = q->elements;

	l = Q_LEFT(i);
	r = Q_RIGHT(i);

	largest = i;

	if (l < q->heap_size)
		if (dragonfly_is_better_element(q, elements[l], elements[i]))
			largest = l;

	if (r < q->heap_size)
		if (dragonfly_is_better_element(q, elements[r], elements[largest]))
			largest = r;

	if (largest != i)
	{
		tmp = elements[i];
		elements[i] = elements[largest];
		elements[largest] = tmp;

		dragonfly_heapify(q, largest);
	}
}

void dragonfly_enlarge_heap(dragonfly_queue* q)
{
	int* tmp;
	int i;

	tmp = (int*)malloc(sizeof(int) * q->max_size * 2);
	if (tmp == NULL) exit_with_error("can't alloc elements");

	for (i = 0; i < q->heap_size; i++)
		tmp[i] = q->elements[i];

	free(q->elements);
	q->elements = tmp;
	q->max_size *= 2;
}

void dragonfly_free_heap(dragonfly_queue* q)
{
	free(q->elements);
}

int dragonfly_extarct_max(dragonfly_queue* q)
{
	int res;

	if (q->heap_size == 0)
		return -1;

	res = q->elements[0];

	q->elements[0] = q->elements[q->heap_size - 1];
	(q->heap_size)--;

	dragonfly_heapify(q, 0);

	return res;
}

void dragonfly_heap_insert(dragonfly_queue* q, int val)
{
	int i;

	if (q->heap_size == q->max_size)
		dragonfly_enlarge_heap(q);

	(q->heap_size)++;
	i = q->heap_size - 1;

	while ((i > 0) && (dragonfly_is_better_element(q, val, q->elements[Q_PARENT(i)])))
	{
		q->elements[i] = q->elements[Q_PARENT(i)];
		i = Q_PARENT(i);
	}
	q->elements[i] = val;
}

void dragonfly_reset_heap(dragonfly_queue* q)
{
	if (q->max_size > 64)
	{
		free(q->elements);
		q->max_size = 64;
		q->elements = (int*)malloc(sizeof(int) * q->max_size);

		if (q->elements == 0) exit_with_error("can't allocate queue");
	}

	q->max_size = 64;
	q->heap_size = 0;
}




void dragonfly_print_queue(dragonfly_queue* q)
{
	int i;
	printf("QUEUE: ");

	for (i = 0; i < q->heap_size; i++)
		printf("%d ", q->elements[i]);
	printf("\n");
}
