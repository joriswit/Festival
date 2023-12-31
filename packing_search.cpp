// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "packing_search.h"

#include "util.h"
#include "moves.h"
#include "distance.h"
#include "request.h"
#include "advanced_deadlock.h"
#include "graph.h"
#include "bfs.h"
#include "deadlock.h"
#include "perimeter.h"
#include "deadlock_cache.h"
#include "engine.h"
#include "greedy.h"
#include "girl.h"
#include "match_distance.h"
#include "tree.h"
#include "imagine.h"
#include "debug.h"
#include "park_order.h"
#include "cycle_deadlock.h"

#include "xy_deadlock.h"
#include "hf_search.h"
#include "stuck.h"


int compute_start_zones(int_board start_zones)
{
	board b;
	int zones_num;

	clear_boxes(initial_board, b);
	turn_targets_into_walls(b);

	zones_num = mark_connectivities(b, start_zones);

	if (verbose >= 5)
	{
		printf("%d start zones\n", zones_num);
		print_board_with_zones(b, start_zones);
	}

	return zones_num;
}


void declare_roots(tree *t, board b, int_board start_zones, int start_zones_num, helper *h)
{
	board c;
	int i;

	// This routine is useful when one root is the child of another root.
	// By declaring the roots now, the child will be blocked in girl mode.

	for (i = 0; i < start_zones_num; i++)
	{
		copy_board(b, c);
		put_sokoban_in_zone(c, start_zones, i);
		if (has_pull_moves(c) == 0) continue;

		add_board_to_tree(t, c, h);
	}
}

void packing_search(board b_in, int time_allocation, int search_mode, tree *t, helper *h)
{
	board b, c;
	
	int iter_num = 0;
	int i,res;
	int cleared_all_targets = -1;
	int local_start_time;
	pack_request req;
	int failed_cell = 0;
	expansion_data *best_so_far = 0;
	score_element *s;
	int label;
	int tree_pos, tree_son, tree_weight;
	expansion_data *new_node = 0, *expanded_node;
	move *move_to_play;
	int prev_perimeter;
	expansion_data *last_best = 0;
	int_board start_zones;
	int start_zones_num;
	int perimeter_entries = 0;
	int testing_best;
	int trim = 0;

	if (time_allocation <= 0) return;

	init_scored_distance(b_in, 1, h); // 1 = pull_mode

	// TREE
	reset_tree(t);
	t->pull_mode = 1;
	t->search_mode = search_mode;

	local_start_time = (int)time(0);

	h->perimeter_found = 0; // when perimeter is found, inefficent moves are allowed
	h->enable_inefficient_moves = 0;

	copy_board(b_in, b);

	if (search_mode == BASE_SEARCH)
		enter_reverse_mode(b, 1); // mark bases
	else
		enter_reverse_mode(b, 0);

	start_zones_num = compute_start_zones(start_zones);

	init_pack_requets(h);

	declare_roots(t, b, start_zones, start_zones_num, h); 
	// to avoid a root being the child of another root

	for (i = 0; i < start_zones_num; i++)
	{
		copy_board(b, c);
		put_sokoban_in_zone(c, start_zones, i);

		if (has_pull_moves(c) == 0) continue;

		if (start_zone_is_deadlocked(c))
		{
			if (verbose >= 4)
			{
				printf("deadlocked start zone!\n");
				print_board(c);
			}
			continue;
		}


		if (verbose >= 5)
		{
			printf("zone %d / %d:\n", i, start_zones_num);
			print_board(c);
		}

		set_root(t, c, h);
		new_node = last_expansion(t);

		set_advisors_and_weights_to_last_node(t, h);
		new_node->best_past = &(new_node->node->score); // score might be updated by match-distance

		s = new_node->best_past;
		add_pack_request(s, h);

		label = get_label_of_score(s, 1, h);
		add_label_to_last_node(t, label, h);

		if (search_mode == GIRL_SEARCH)
			handle_new_pos_in_girl_mode_tree(t, last_expansion(t));

		update_best_so_far(&best_so_far, new_node, 1, search_mode);
		perimeter_entries += insert_node_into_perimeter(t, new_node, 1);

	}

	while (1)
	{
		iter_num++;

		if (time_limit_exceeded(time_allocation, local_start_time)) break;
		if (any_core_solved) break;

		testing_best = 0;

		if ((iter_num & 1) || h->perimeter_found)
			get_next_pack_request(&req, h);
		else
		{
			get_pack_request_of_score(&req, best_so_far->best_past, h); // process best cell
			testing_best = 1;
		}


		if ((verbose >= 5) && (search_mode != GIRL_SEARCH))
			printf("Looking for lvl: %d targets: %d connect: %d room: %d\n",
				req.boxes_in_level, req.boxes_on_targets,
				req.connectivity, req.room_connectivity);

		if ((search_mode != GIRL_SEARCH) || (h->perimeter_found))
		{
			label = get_label_of_pack_request(&req, h);
			tree_weight = best_move_in_tree(t, label, &tree_pos, &tree_son);
		}
		else
		{
			choose_pos_son_to_expand_Epsilon_greedy_tree(t, &tree_pos, &tree_son, h);
			if (tree_pos == -1)
			{
				if (verbose >= 4) printf("can't solve level\n");
				break;
			}
		}


		if ((h->perimeter_found) && (tree_weight >= 0))
			continue; // look only in the subtrees of perimeter nodes

		expanded_node = t->expansions + tree_pos;


		// in base search it is possible to exhaust the search without finding a solution
		// some boxes are removed and few remaining boxes just move around.
		// Also in rev_search when the wrong boxes are wallified.
		if (failed_cell >= 10000) break;
		if (tree_pos == -1)
		{
			failed_cell++;
			continue;
		}
		failed_cell = 0;

		move_to_play = get_node_move(t, expanded_node, tree_son);

		if (verbose >= 5)
		{
			printf("\nNode to expand: (%d)\n", tree_pos);
			print_node(t, expanded_node, 0, (char*)"", h);
			printf("with move: ");
			print_move(move_to_play);
			printf("\n");
		}

		bytes_to_board(expanded_node->b, b);

		if (move_is_deadlocked(b, move_to_play, 1, search_mode)) // 1 - pull mode
		{
			trim++;

			if (verbose >= 5) printf("position will be deadlocked\n");

			set_node_as_deadlocked(t, tree_pos, tree_son);

			if (h->perimeter_found)
			{
				UINT_16 depth;
				UINT_64 hash;
				hash = get_node_hash(t, expanded_node, tree_son);

				if (is_in_perimeter(hash, &depth, 0))
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

		if (can_played_reversed(b, move_to_play, 1, search_mode) == 0)
		{
			remove_son_from_tree(t, t->expansions + tree_pos, tree_son);

			if (verbose >= 5) printf("can't play reversed\n");
			continue; // it's the move that is impossible, not the position
		}

		expand_node(t, t->expansions + tree_pos, tree_son, h);
		set_advisors_and_weights_to_last_node(t, h);
		new_node = last_expansion(t);

		if (search_mode == GIRL_SEARCH)
			handle_new_pos_in_girl_mode_tree(t, new_node);

		if (verbose >= 5)
		{
			printf("\nNode after expansion:\n");
			print_node(t, new_node, 0, (char*)"", h);
		}

		add_pack_request(new_node->best_past, h);

		label = get_label_of_score(new_node->best_past, 1, h);
		add_label_to_last_node(t, label, h);


		// update best_so_far with the new node
		res = update_best_so_far(&best_so_far, new_node, 1, search_mode);
		if (res)
		{
			if (cleared_all_targets != -1) // keep removing boxes from board, if possible
				cleared_all_targets = iter_num;
		}

		if (node_is_solved(t, new_node))
			break; // all boxes in initial place

		// check if best position is deadlocked by cycles
		check_cycle_deadlock(t, best_so_far, h);

		// replace best position if it became deadlocked
		if (best_so_far->node->deadlocked)
			best_so_far = best_node_in_tree(t);

		if (best_so_far == 0)
		{
			if (verbose >= 4)
				printf("Tree is exhausted in packing search alg= %d\n", search_mode);
			break;
		}

		show_progress_tree(t, new_node, best_so_far, t->expansions_num, &last_best, h);

		learn_from_subtrees(t, new_node);
		if (test_best_for_patterns(t, best_so_far))
		{
			recheck_tree_with_patterns(t);
			best_so_far = best_node_in_tree(t);
		}


		if (new_node->node->score.boxes_in_level == 0)
		{
			if (verbose >= 4) printf("zero boxes\n");
			break;
		}

		if (tree_nearly_full(t))
		{
			strcpy(global_fail_reason, "Max nodes reached");
			break;
		}

		prev_perimeter = h->perimeter_found;
		if (check_if_perimeter_reached(t, h))
			// if a son is found, this will set the position weight to a negative value.
			// So it will become attractive to the FESS search
		{
			if ((verbose >= 4) && (prev_perimeter == 0))
				print_node(t, new_node, 0, (char*)"", h);
			set_next_pack_request_to_score(new_node->best_past , h);
		}

		if (h->perimeter_found == 0) // inserting the node may overwrite useful perimeter values
			perimeter_entries += insert_node_into_perimeter(t, new_node, 1);
		else
			perimeter_entries += insert_node_into_perimeter(t, new_node, 0); // insert just a small crumb

		if (verbose >= 5) my_getch();
	}


	if (verbose >= 4)
	{
		printf("back search done!\n");
		printf("################################################################\n");
	}

	if (verbose >= 4)
		print_node(t, best_so_far, 0, (char*)"", h);

	if (best_so_far == 0) // shouldn't really be here
	{
		if (verbose >= 4) exit_with_error("can't solve level");
		best_so_far = t->expansions;
	}

	bytes_to_board(best_so_far->b, c);
	set_imagined_hf_board(c, h);


	if (verbose >= 4)
		printf("perimeter entries: %d\n", perimeter_entries);

	conclude_parking(t, best_so_far, h);
	
	if (verbose >= 5)
	{
		show_parking_order(h);
		show_parking_order_on_board(h);
		my_getch();
	}

	if (verbose >= 4)
	{
		printf("\n");
		printf("time=        %12d\n", (int)time(0) - local_start_time);
		printf("expansions=  %12d\n", t->expansions_num);
		printf("nodes=       %12d\n", t->nodes_num);
		printf("move_hashes= %12d\n", t->move_hashes_num);
		printf("trim=        %12d\n", trim);
	}

	return;
}
