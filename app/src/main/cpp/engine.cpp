// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include "tree.h"
#include "engine.h"
#include "advanced_deadlock.h"
#include "io.h"
#include "debug.h"
#include "request.h"
#include "sol.h"
#include "util.h"
#include "perimeter.h"
#include "girl.h"
#include "greedy.h"
#include "imagine.h"
#include "cycle_deadlock.h"
#include "stuck.h"
#include "hf_search.h"

int has_winning_move(tree *t, expansion_data *e, helper *h)
{
	// mostly for cyclic levels.
	// In cyclic level, the target position sometimes can't be found because it is identical to the
	// root position (which is already in the tree, and even expanded). So this routine identifies
	// a winning situtaion one move before it is played on the board.

	int i, place, y, x;
	move_hash_data *mh;
	board b;
	expansion_data *f;

	if (is_cyclic_level() == 0) return 0; // rely on normal positions

	mh = t->move_hashes + e->move_hash_place;

	for (i = 0; i < e->moves_num; i++)
	{
		place = find_node_by_hash(t, mh[i].hash);
		if (place == -1) exit_with_error("missing node");
		if (t->nodes[place].score.boxes_on_targets == global_boxes_in_level)
			break;
	}

	if (i == e->moves_num) return 0;

	bytes_to_board(e->b, b);
	apply_move(b, &mh[i].move, NORMAL);

	// perturb the board a little to differentiate it from the actual root

	for (y = 0; y < height; y++)
		for (x = 0; x < width; x++)
			b[y][x] &= ~TARGET;

	set_root(t, b, h);
	set_advisors_and_weights_to_last_node(t, h);

	f = last_expansion(t);
	f->father = e;
	f->depth = e->depth + 1;

	mh[i].hash = f->node->hash;

	return 1;
}

void FESS(board b, int time_allocation, int search_mode, tree *t, helper *h)
{
	request req;
	int weight;
	int solved = 0;
	expansion_data *best_so_far;
	int local_start_time;
	board c;
	int label;
	score_element *s;
	expansion_data *new_node, *expanded_node;
	int iter_num = 0;
	move *played_move;
	int prev_perimeter;
	int tree_weight, tree_pos, tree_son;
	int search_pos_num = 0;
	expansion_data *last_best = 0;
	int perimeter_entries = 0;
	int cycle_work = 1;
	UINT_16 depth;

	if (time_allocation <= 0) return;

	init_scored_distance(b, 0, h); // 0 = push_mode

	local_start_time = (int)time(0);
	
	// TREE
	reset_tree(t);
	t->pull_mode = 0;
	t->search_mode = search_mode;

	h->perimeter_found = 0;
	h->enable_inefficient_moves = 0;

	search_pos_num = 0;

	if (is_in_perimeter(get_board_hash(b), &depth, 1))
		h->enable_inefficient_moves = 1; // disable pi-corral pruning, even before the first node

	// TREE
	set_root(t, b, h);
	new_node = last_expansion(t);

	set_advisors_and_weights_to_last_node(t, h);
	new_node->best_past = &(new_node->node->score); // score might be updated by match-distance

	search_pos_num = 1;

	if (search_mode == GIRL_SEARCH)
		handle_new_pos_in_girl_mode_tree(t, new_node);

	init_requests(h);

	s = new_node->best_past;
	add_request(s, h);
	label = get_label_of_score(s, 0, h);
	add_label_to_last_node(t, label, h);

	best_so_far = new_node;

	perimeter_entries += insert_node_into_perimeter(t, new_node, 1);

	while (1)
	{
		iter_num++;

		if (time_limit_exceeded(time_allocation, local_start_time)) break;
		if (any_core_solved) break;


		if ((iter_num & 1) || h->perimeter_found)
			get_next_request(&req, h);
		else
			get_request_of_score(&req, best_so_far->best_past, h); // process best cell


		if ((verbose >= 5) && (search_mode != GIRL_SEARCH))
		{
			printf("Looking for box: %d connectivity: %d  OOP: %d imagine: %d. \n",
				req.packed_boxes, req.connectivity, req.out_of_plan, req.imagine);
		}

		if ((search_mode != GIRL_SEARCH) || (h->perimeter_found))
		{
			// TREE
			label = get_label_of_request(&req, h);
			tree_weight = best_move_in_tree(t, label, &tree_pos, &tree_son);
		}
		else
		{
			weight = 0;
			tree_weight = 0;
			choose_pos_son_to_expand_Epsilon_greedy_tree(t, &tree_pos, &tree_son, h);

			if (tree_pos == -1)
				exit_with_error("bad tree");
		}

		if ((h->perimeter_found) && (tree_weight >= 0))
			continue; // look only in the subtree of the perimeter node

		expanded_node = t->expansions + tree_pos;
		
		if (tree_weight == 1000000)
		{
			if (verbose >= 5) printf("could not find a move to expand\n");
			continue;
		}
		

		played_move = get_node_move(t, expanded_node, tree_son);

		if (verbose >= 5) 
		{
			printf("Node to expand:\n");
			print_node(t, expanded_node, 0, (char*)"", h);
			printf("with move: ");
			print_move(played_move);
			printf("\n");
		}
	
		// check if the chosen move is deadlocked
		bytes_to_board(expanded_node->b, c);
		if (move_is_deadlocked(c, played_move, 0, search_mode)) // 0 - pull mode
		{
			if (verbose >= 5)
				printf("position will be deadlocked\n");

			set_node_as_deadlocked(t, tree_pos, tree_son);

			if (h->perimeter_found)
			{
				UINT_16 depth;
				UINT_64 hash;
				hash = get_node_hash(t, expanded_node, tree_son);

				if (is_in_perimeter(hash, &depth, 1))
				{
					printf("should not be pruned!\n");
					my_getch();

					if (verbose >= 4)
					{
						print_node(t, expanded_node, 1, (char*)"", h);
						printf("son= %d\n", tree_son);
						printf("core= %d\n", h->my_core);
						printf("hash of deadlocked son: %016llx\n", hash);
					}
				}
			}

			continue;
		}

		search_pos_num++;
	
		// Expand node
		expand_node(t, t->expansions + tree_pos, tree_son, h);
		set_advisors_and_weights_to_last_node(t, h);

		new_node = last_expansion(t);

		if (verbose >= 5)
		{
			printf("Node after expansion:\n");
			print_node(t, new_node, 0, "", h);
		}

		if (search_mode == GIRL_SEARCH)
			handle_new_pos_in_girl_mode_tree(t, new_node);

		add_request(new_node->best_past , h);

		label = get_label_of_score(new_node->best_past, 0, h);
		add_label_to_last_node(t, label, h);

		// update best_so_far
		update_best_so_far(&best_so_far, new_node, 0, search_mode);

		if (node_is_solved(t, new_node))
		{
			solved = 1;
			break;
		}

		// check if best position is deadlocked by cycles
		check_cycle_deadlock(t, best_so_far, h);

		// replace best position if it became deadlocked
		if (best_so_far->node->deadlocked)
		{
			best_so_far = best_node_in_tree(t);
			if (verbose >= 5) print_node(t, best_so_far, 0, "new best\n", h);
		}

		if (best_so_far == 0)
		{
			if (verbose >= 4) printf("Tree is exhausted in engine\n");
			break;
		}

		if (tree_nearly_full(t))
		{
			strcpy(global_fail_reason, "Max nodes reached");
			break;
		}
		
		show_progress_tree(t, new_node, best_so_far, search_pos_num, &last_best, h);

		if (has_winning_move(t, new_node, h))
		{
			solved = 1;
			new_node = last_expansion(t); // updated by has_winning_move
			break;
		}

		learn_from_subtrees(t, new_node);

		prev_perimeter = h->perimeter_found;
		if (check_if_perimeter_reached(t, h))
		// if a son is found, this will set the position weight to a negative value.
		// So it will become attractive to the FESS search
		{
			if ((verbose >= 4) && (prev_perimeter == 0))
				print_node(t, new_node, 0, (char*)"", h);
			set_next_request_to_score(new_node->best_past, h); 
		}

		if (h->perimeter_found == 0) // inserting the node may overwrite useful perimeter values
			perimeter_entries += insert_node_into_perimeter(t, new_node, 1);

		if (verbose >= 5)
			my_getch();
	}

	if (verbose >= 4)
		printf("perimeter entries: %d\n", perimeter_entries);

	if (solved)
	{
		if (verbose >= 3) print_node(t, new_node, 0, (char*)"", h);
		if (verbose >= 4)
		{
			printf("solved. %d positions\n", search_pos_num);
			if (cores_num > 1)
				printf("solved by core %d at time %d\n", h->my_core,(int)time(0) - start_time);
		}
		store_solution_in_helper(t, new_node, h);
		any_core_solved = 1;
	}

	if (solved == 0)
	{
		if (save_best_flag) // save best position even if was not solved
			store_solution_in_helper(t, best_so_far, h);
	}

	h->level_solved = solved;


	if (t->search_mode == REV_SEARCH)
		setup_rev_board(best_so_far, h);
}
