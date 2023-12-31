#include <stdlib.h>
#include <stdio.h>

#include "util.h"
#include "queue.h"
#include "tree.h"
#include "score.h"

#define Q_LEFT(i) (((i)* 2) + 1)
#define Q_RIGHT(i) (((i)* 2) + 2)
#define Q_PARENT(i) (((i) - 1) / 2)

void init_queue(queue *q, tree *t)
{
	q->max_size = 64;
	q->elements = (int*)malloc(sizeof(int) * q->max_size);
	q->heap_size = 0;

	if (q->elements == 0) exit_with_error("can't allocate elements");
	q->t = t;
}

int is_better_element(queue *q, int first, int second)
{
	expansion_data *e1, *e2;
	int pull_mode = q->t->pull_mode;
	int search_mode = q->t->search_mode;

	e1 = q->t->expansions + first;
	e2 = q->t->expansions + second;

	if (e1->best_weight < e2->best_weight) return 1;
	if (e1->best_weight > e2->best_weight) return 0;

	if (is_better_score(&(e1->best_score), &(e2->best_score), pull_mode, search_mode)) return 1;
	if (is_better_score(&(e2->best_score), &(e1->best_score), pull_mode, search_mode)) return 0;

	if (first < second) return 1;
	return 0;
}


void heapify(queue *q, int i)
{
	int *elements;
	int l, r, tmp;
	int largest;

	if (q->heap_size == 0) return;

	elements = q->elements;

	l = Q_LEFT(i);
	r = Q_RIGHT(i);

	largest = i;

	if (l < q->heap_size)
		if (is_better_element(q, elements[l], elements[i]))
			largest = l;

	if (r < q->heap_size)
		if (is_better_element(q, elements[r], elements[largest]))
			largest = r;

	if (largest != i)
	{
		tmp = elements[i];
		elements[i] = elements[largest];
		elements[largest] = tmp;

		heapify(q, largest);
	}	
}

void enlarge_heap(queue *q)
{
	int *tmp;
	int i;

	tmp = (int*)malloc(sizeof(int) * q->max_size * 2);
	if (tmp == NULL) exit_with_error("can't alloc elements");

	for (i = 0; i < q->heap_size; i++)
		tmp[i] = q->elements[i];

	free(q->elements);
	q->elements = tmp;
	q->max_size *= 2;
}

void free_heap(queue *q)
{
	free(q->elements);
}

int extarct_max(queue *q)
{
	int res;

	if (q->heap_size == 0)
		return -1;

	res = q->elements[0];

	q->elements[0] = q->elements[q->heap_size - 1];
	(q->heap_size)--;

	heapify(q, 0);
	
	return res;
}

void heap_insert(queue *q, int val)
{
	int i;

	if (q->heap_size == q->max_size)
		enlarge_heap(q);

	(q->heap_size)++;
	i = q->heap_size - 1;

	while ((i > 0) && (is_better_element(q, val, q->elements[Q_PARENT(i)])))
	{
		q->elements[i] = q->elements[Q_PARENT(i)];
		i = Q_PARENT(i);
	}
	q->elements[i] = val;
}

void reset_heap(queue *q)
{
	if (q->max_size > 64)
	{
		free(q->elements);
		q->max_size = 64;
		q->elements = (int*)malloc(sizeof(int) * q->max_size);
	}

	q->max_size = 64;
	q->heap_size = 0;
}


void verify_heap(queue *q)
{
	int l, r;
	int i;

	for (i = 0; i < q->heap_size; i++)
	{
		l = Q_LEFT(i);
		r = Q_RIGHT(i);

		if (l < q->heap_size)
		{
			if (is_better_element(q, q->elements[l], q->elements[i]))
				printf("heap failed 1\n");
		}
		if (r < q->heap_size)
		{
			if (is_better_element(q, q->elements[r], q->elements[i]))
				printf("heap failed 2\n");
		}

	}
}

void print_queue(queue *q)
{
	int i;
	printf("QUEUE: ");

	for (i = 0; i < q->heap_size; i++)
		printf("%d ", q->elements[i]);
	printf("\n");
}

void remove_value_from_queue(queue *q, int val)
{
	// make the value the best item - and remove it
	
	int i;

	for (i = 0; i < q->heap_size; i++)
		if (q->elements[i] == val)
			break;

	if (i == q->heap_size)
		exit_with_error("can't find element");

	while (i > 0)
	{
		q->elements[i] = q->elements[Q_PARENT(i)];
		i = Q_PARENT(i);
	}
	extarct_max(q);
}