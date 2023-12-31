#include <stdlib.h>
#include <stdio.h>

#ifdef THREADS
#include <pthread.h>
#endif

#include "stuck.h"
#include "fixed_boxes.h"
#include "xy_deadlock.h"
#include "util.h"
#include "wobblers.h"
#include "deadlock_cache.h"
#include "mini_search.h"

#define MAX_STUCK_PATTERN_SIZE 12
#define MAX_STUCK_PATTERNS 500


#ifdef THREADS
pthread_mutex_t stuck_pattern_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

typedef struct
{
	int boxes[MAX_STUCK_PATTERN_SIZE];
	int boxes_num;
	board b;
	int pull;
} stuck_pattern;


stuck_pattern stuck_patterns[MAX_STUCK_PATTERNS];
int stuck_patterns_num = 0;

void init_stuck_patterns()
{
	stuck_patterns_num = 0;
}


int is_in_stuck_patterns(board b, int pull_mode)
{
	int indices[MAX_INNER];
	int i, j, y, x;
	stuck_pattern *sp;

	if (stuck_patterns_num == 0) return 0;

	mark_indices_with_boxes(b, indices);

	for (i = 0; i < stuck_patterns_num; i++)
	{
		sp = stuck_patterns + i;

		if (sp->pull != pull_mode) continue;

		for (j = 0; j < sp->boxes_num; j++)
			if (indices[sp->boxes[j]] == 0)
				break;

		if (j == sp->boxes_num)
		{
			get_sokoban_position(b, &y, &x);

			if ((sp->b)[y][x] & SOKOBAN)
				return 1;
		}
	}

	return 0;
}


void add_stuck_pattern(board b, int pull_mode)
{
	int n;
	stuck_pattern *sp;

#ifdef THREADS
	pthread_mutex_lock(&stuck_pattern_mutex);
#endif

	if (is_in_stuck_patterns(b, pull_mode) == 0)
	{
		if (verbose >= 4)
		{
			if (pull_mode)
				printf("adding stuck pattern %d (pull)\n", stuck_patterns_num);
			else
				printf("adding stuck pattern %d \n", stuck_patterns_num);

			print_board(b);
		}

		n = boxes_in_level(b);
		if (n > MAX_STUCK_PATTERN_SIZE) exit_with_error("pattern too big");
		if (n == 0) exit_with_error("empty pattern");

		sp = stuck_patterns + stuck_patterns_num;

		copy_board(b, sp->b);
		get_indices_of_boxes(b, sp->boxes);
		sp->boxes_num = n;
		sp->pull = pull_mode;

		stuck_patterns_num++;

		if (stuck_patterns_num >= MAX_STUCK_PATTERNS) 
			stuck_patterns_num--;

	}

#ifdef THREADS
	pthread_mutex_unlock(&stuck_pattern_mutex);
#endif
}

#define DISTILL_ON_TARGETS 0
#define DISTILL_XY         1

void distill_pattern(board b, int pull_mode, int distill_method)
{
	board c;

	int i, j, res, deadlocked;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if (b[i][j] & BOX)
			{
				copy_board(b, c);
				c[i][j] &= ~BOX;
				expand_sokoban_cloud(c);
				deadlocked = 0;

				if (all_boxes_in_place(c, pull_mode)) continue;

				switch (distill_method)
				{
				case DISTILL_ON_TARGETS:
					if (pull_mode) exit_with_error("bad distill");
					res = can_put_all_on_targets(c, 1000); // note: -1 is search aborted
					if (res == 0) deadlocked = 1;
					break;
				case DISTILL_XY:
					res = xy_deadlock_search(c, pull_mode);
					if (res == 1) deadlocked = 1;
					break;
				default:
					exit_with_error("bad method");
				}

				if (deadlocked)
				{
					copy_board(c, b);
					i = 0;
					j = 0;
				}
			}
		}
}



void register_mini_corral(board b_in)
{
	board b;
	int i, j, res;

	copy_board(initial_board, b);
	clear_boxes_inplace(b);
	clear_sokoban_inplace(b);

	if (is_in_stuck_patterns(b, 0)) return;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if (b_in[i][j] & BOX)
				b[i][j] |= BOX;

			// restore wallified boxes
			if ((b_in[i][j] == WALL) && (initial_board[i][j] != WALL))
				b[i][j] |= BOX;
		}

	get_sokoban_position(b_in, &i, &j);
	b[i][j] |= SOKOBAN;
	expand_sokoban_cloud(b);

	res = can_put_all_on_targets(b, 1000); // note: -1 is search aborted

	if (res != 0) return;

	distill_pattern(b, 0, DISTILL_ON_TARGETS);

	if (boxes_in_level(b) > MAX_STUCK_PATTERN_SIZE) return;

	add_stuck_pattern(b, 0);
}


void slow_xy_deadlock(board b_in, int pull_mode)
{
	board b;
	int n, res;

	copy_board(b_in, b);

	if (is_in_stuck_patterns(b, pull_mode)) return;

	if (pull_mode == 0)
		remove_wobblers(b, pull_mode);

	clear_xy_boxes(b, pull_mode);

	n = boxes_in_level(b);
	if (n == 0) return;
	if (n > MAX_STUCK_PATTERN_SIZE) return;
	if (n == global_boxes_in_level) return;

	res = xy_deadlock_search(b, pull_mode);

	if (res == 1)
	{
		distill_pattern(b, pull_mode, DISTILL_XY);
		add_stuck_pattern(b, pull_mode);
	}
	
}


int is_stuck_deadlock(board b, int pull_mode)
{
	int res;
	
	res = is_in_stuck_patterns(b, pull_mode);

	return res;
}


void learn_from_subtrees(tree* t, expansion_data* e)
{
	board b;
	int pull_mode = t->pull_mode;

	while (e)
	{
		if (e->subtree_size == (64 + 64*pull_mode))
		{
			bytes_to_board(e->b, b);
			slow_xy_deadlock(b, pull_mode);
		}
		e = e->father;
	}
}


void recheck_tree_with_patterns(tree* t)
{
	int i;
	expansion_data* e;
	board b;

	for (i = 0; i < t->expansions_num; i++)
	{
		e = t->expansions + i;

		if (e->node->deadlocked) continue;

		bytes_to_board(e->b, b);

		if (is_in_stuck_patterns(b, t->pull_mode))
		{
//			printf("marking %d\n", i);
			set_subtree_as_deadlocked(t, e);
		}
	}
}

int test_best_for_patterns(tree* t, expansion_data* e)
{
	board b;
	bytes_to_board(e->b, b);

	return is_in_stuck_patterns(b, t->pull_mode);
}