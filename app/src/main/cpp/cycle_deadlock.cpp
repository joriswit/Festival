#include <stdio.h>

#include "cycle_deadlock.h"
#include "util.h"
#include "corral_deadlock.h"
#include "debug.h"


void set_leaf_direction_in_tree(tree* t)
{
	int i,j,place;
	node_element* n;
	int changed = 1;
	expansion_data *e, *e2;
	move_hash_data* mh;

	for (i = 0; i < t->expansions_num; i++)
		t->expansions[i].leaf_direction = -1;

	while (changed)
	{
		changed = 0;

		for (i = t->expansions_num - 1; i >= 0; i--)
		{
			e = t->expansions + i;

			if (e->leaf_direction != -1) continue;
			if (e->node->deadlocked) continue;
			mh = t->move_hashes + e->move_hash_place;

			for (j = 0; j < e->moves_num; j++)
			{
				if (mh[j].deadlocked) continue;
				place = find_node_by_hash(t, mh[j].hash);
				n = t->nodes + place;

				if (n->deadlocked)
				{
					mh[j].deadlocked = 1;
					continue;
				}
				
				if (n->expansion == -1)
				{
					e->leaf_direction = j;
					changed = 1;
					break;
				}

				e2 = t->expansions + n->expansion;
				if (e2->leaf_direction != -1)
				{
					e->leaf_direction = j;
					changed = 1;
					break;
				}
			}
		}
	}

	for (i = 0; i < t->expansions_num; i++)
	{
		e = t->expansions + i;
		if (e->node->deadlocked)
			continue;

		if (e->leaf_direction != -1) continue;

		e->node->deadlocked = 1;
	}
}

void check_cycle_deadlock(tree* t, expansion_data* e, helper *h)
{
	move_hash_data* mh;
	int place;
	node_element* n;
	int depth = 0;
	expansion_data* base = e;

	if (t->search_mode == BASE_SEARCH) return;
	if (is_cyclic_level()) return;

	// check the current route from e to a leaf:
	while (e->leaf_direction != -1)
	{
		if (e->node->deadlocked) break;

		mh = t->move_hashes + e->move_hash_place;
		place = find_node_by_hash(t, mh[e->leaf_direction].hash);
		n = t->nodes + place;

		if (n->deadlocked) break;

		if (n->expansion == -1)
			return;

		e = t->expansions + n->expansion;
		depth++;

		if (depth > 500) exit_with_error("cycle cycle");
	}

	if (verbose >= 5) printf("recalculating leaves ");

	set_leaf_direction_in_tree(t);

	if (verbose >= 5) printf("Done\n");

	if (base->node->deadlocked)
	{
		if (verbose >= 5)
		{
			printf("best position is deadlocked!\n");
			print_node(t, base, 0, (char*)"", h);
		}
	}
}


/*
int cycle_deadlock_bfs(tree* t, expansion_data* e, expansion_data** q, int max_len)
{
	int q_pos = 0;
	int q_len;
	int i,place;
	move_hash_data* mh;
	int result = 1;
	expansion_data* f;

	e->visited = 1;
	q[0] = e;
	q_len = 1;

	while (q_pos < q_len)
	{
		e = q[q_pos++];

		mh = t->move_hashes + e->move_hash_place;

		for (i = 0; i < e->moves_num; i++)
		{
			if (mh[i].deadlocked) continue;
			place = find_node_by_hash(t, mh[i].hash);
			if (place == -1) exit_with_error("bad tree\n");
			if (t->nodes[place].deadlocked)
			{
				mh[i].deadlocked = 1;
				continue;
			}
			if (t->nodes[place].expansion == -1)
			{ // found a leaf - not a cycle
				result = 0;
				break;
			}

			f = t->expansions + t->nodes[place].expansion;
			if (f->visited) continue;

			f->visited = 1;
			q[q_len++] = f;

			if (q_len == max_len)
				break;
		}

		if (q_len == max_len)
		{
			result = 0;
			break;
		}

	}

	for (i = 0; i < q_len; i++)
		q[i]->visited = 0;

	if (result == 0) return 0;

	return q_len;
}


void cycles_explorer(tree* t, expansion_data* e)
{
	expansion_data* cycle[300];
	int len = 1;
	int i;
	board b;

	while (len)
	{
		len = cycle_deadlock_bfs(t, e, cycle, 300);

		if (len > 0)
		{
//			printf("Found a cycle of len %d!\n", len);
//			print_node(t, e, 0, (char*)"");

			for (i = 0; i < len; i++)
			{
				bytes_to_board(cycle[i]->b, b);
//				learn_pull_deadlock(b);
			}

		}
		e = e->father;
		if (e == 0) return;
	}
}


void learn_deadlocks_from_tree(tree* t)
{
	static int first_time = 1;
	static int checkpoints[100];	
	int i;

	if (first_time)
	{
		first_time = 0;
		for (i = 0; i < 100; i++)
		{
			checkpoints[i] = (int)pow(1.2, i);
//			printf("%d\n", checkpoints[i]);
		}
	}

	for (i = 0; i < 100; i++)
	{
		if (t->expansions_num == checkpoints[i])
		{
			printf("checkpoint %d\n", checkpoints[i]);

			set_leaf_direction_in_tree(t);

			printf("checkpoint done\n");

			return;
		}
	}

	cycles_explorer(t, t->expansions + t->expansions_num - 1);
}
*/