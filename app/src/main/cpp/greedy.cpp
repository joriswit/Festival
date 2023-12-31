#include <math.h>
#include <stdio.h>

#include "greedy.h"
#include "util.h"
#include "tree.h"
#include "debug.h"

int inv_epsilon = 20;

int debug_greedy = 0;

void set_inv_epsilon()
{
	inv_epsilon = global_boxes_in_level + 1;
}

expansion_data *go_to_root_greedy_tree(tree *t)
{
	int i;
	int min_subtree_size = 1000000;
	int best_tree = -1;
	expansion_data *expansions = t->expansions;

	for (i = 0; i < t->expansions_num; i++)
	{
		if (expansions[i].node->deadlocked) continue;

		if (expansions[i].depth > 0)
			break; // processed all roots

		if ((best_tree == -1) || (expansions[i].subtree_size < min_subtree_size))
		{
			min_subtree_size = expansions[i].subtree_size;
			best_tree = i;
		}
	}

	if (best_tree == -1)
		return 0;

	return expansions + best_tree;
}

int choose_cyclic_son_tree(tree *t, expansion_data *e)
{
	int son;
	move_hash_data *mh = t->move_hashes + e->move_hash_place;

	if (e->node->deadlocked)
		exit_with_error("choosing son for deadlocked position");

	son = e->next_son_to_check;

	while (mh[son].deadlocked)
	{
		son = (son + 1) % e->moves_num;

		if (son == e->next_son_to_check)
			exit_with_error("returned to first");
	}

	e->next_son_to_check = (son + 1) % e->moves_num;

	return son;
}

void recompute_values_tree(tree *t, expansion_data *e, helper *h)
{
	int i;
	move_hash_data *mh = t->move_hashes + e->move_hash_place;
	score_element s;

	if (e->node->deadlocked)
		exit_with_error("recomputing for deadlock\n");


	// now update node score using sons' scores
	e->node->score.value = -1000000;

	for (i = 0; i < e->moves_num; i++)
	{
		if (mh[i].deadlocked) continue;

		get_score_of_hash(t, mh[i].hash, &s);

		if (s.value > e->node->score.value)
			e->node->score.value = s.value;
	}

	if (e->node->score.value == -1000000)
	{
		print_node(t, e, 1, (char*)"", h);
		exit_with_error("score problem");
	}
}

int choose_best_value_son_tree(tree *t, expansion_data *e, helper *h)
{
	int i;
	double value;
	double best_value = -1000000;
	int best_son = -1;
	move_hash_data *mh = t->move_hashes + e->move_hash_place;
	score_element s;

	if (e->node->deadlocked)
		exit_with_error("choosing son for deadlocked");

	for (i = 0; i < e->moves_num; i++)
	{
		if (mh[i].deadlocked) continue;

		get_score_of_hash(t, mh[i].hash, &s);
		value = s.value;

		if ((best_son == -1) || (value > best_value))
		{
			best_value = value;
			best_son = i;
		}
	}

	if (best_son == -1)
	{
		print_node(t,e, 1, (char*)"", h);
		exit_with_error("no best son");
	}

	return best_son;
}


int pick_son_Epsilon_greedy_tree(tree *t, expansion_data *e, helper *h)
{
	int son;

	if (e->node->deadlocked)
		exit_with_error("epsilon deadlocked");

	if ((e->subtree_size % inv_epsilon) == 0)
	{
		son = choose_cyclic_son_tree(t,e);
		return son;
	}

	son = choose_best_value_son_tree(t,e, h);
	return son;
}

void choose_pos_son_to_expand_Epsilon_greedy_tree(tree *t, int *pos, int *son, helper *h)
{
	expansion_data *e,*next;
	move_hash_data *mh;
	node_element *node = 0;
	int place;

	*pos = -1;

	e = go_to_root_greedy_tree(t);

	if (e == 0) return; // processed all subtrees; pos will be -1

	if (e->node->deadlocked)
		exit_with_error("root is deadlocked");

	do
	{
		set_deadlock_status(t, e);

		if (e->node->deadlocked)
		{
			if (debug_greedy)
				print_node(t, e, 1, (char*)"", h);

			if (verbose >= 4)
			{
				printf("Setting deadlock. Returning to root\n");
				my_getch(); // this shouldn't happen
			}

			e = go_to_root_greedy_tree(t);
			if (e == 0) return; // processed all subtrees; pos will be -1

			continue;
		}

		recompute_values_tree(t, e, h);

		if (debug_greedy)
			print_node(t, e, 1, (char*)"", h);

		*son = pick_son_Epsilon_greedy_tree(t,e, h);

		mh = t->move_hashes + e->move_hash_place;
		place = find_node_by_hash(t, mh[*son].hash);
		if (place == -1) exit_with_error("missing son");
		node = t->nodes + place;

		if (node->expansion != -1)
		{
			next = t->expansions + node->expansion;
			e = next;
		}
	} while (node->expansion != -1);

	*pos = (int)(e - t->expansions);
}


void back_propagate_values_tree(tree *t, expansion_data *e)
{
	int i;
	float max_value = -1000000;
	float value;
	int has_son = 0;
	move_hash_data *mh;
	int node_place;
	node_element *node;

	if (e == 0) return;
	if (e->node->deadlocked) return;

	mh = t->move_hashes + e->move_hash_place;

	for (i = 0; i < e->moves_num; i++)
	{
		if (mh[i].deadlocked) continue;

		node_place = find_node_by_hash(t, mh[i].hash);
		if (node_place == -1) exit_with_error("missing node");
		node = t->nodes + node_place;

		value = node->score.value;

		if (value > max_value)
			max_value = value;

		has_son = 1;
	}

	if (has_son == 0)
	{
		if (node_is_solved(t,e) == 0)
			exit_with_error("no sons?");

		max_value = e->node->score.value;
	}

	e->node->score.value = max_value;

	if (max_value == -1000000) exit_with_error("max val");

	back_propagate_values_tree(t, e->father);
}
