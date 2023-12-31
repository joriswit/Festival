// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include <stdio.h>
#include <stdlib.h>

#include "advanced_deadlock.h"
#include "bfs.h"
#include "util.h"
#include "fixed_boxes.h"
#include "distance.h"
#include "diags.h"
#include "corral.h"
#include "k_dist_deadlock.h"
#include "wobblers.h"
#include "hungarian.h"
#include "graph.h"
#include "rooms_deadlock.h"
#include "corral_deadlock.h"

#include "xy_deadlock.h"
#include "stuck.h"

/*
int stuck_after_pull(board b, int y, int x, int d)
{
	// a box in place y,x is pulled in direction d.
	// check if the player is stuck in 1x1 square

	int sum = 0;
	int sok_y, sok_x, next_y, next_x;
	int i,j,k;
	int ok = 1;

	sok_y = y + delta_y[d] * 2;
	sok_x = x + delta_x[d] * 2;

	next_y = y + delta_y[d];
	next_x = x + delta_x[d];


	if ((b[y][x] & BOX) == 0) exit_with_error("internal error");
	if (b[next_y][next_x] & OCCUPIED) exit_with_error("internal error");
	if (b[sok_y][sok_x] & OCCUPIED) exit_with_error("internal error");

	// pull box
	b[y][x] &= ~BOX;
	b[next_y][next_x] |= BOX;

	for (k = 0; k < 4; k++)
		if (b[sok_y + delta_y[k]][sok_x + delta_x[k]] & OCCUPIED)
			sum++;

	if (sum != 4)
	{
		b[y][x] |= BOX;
		b[next_y][next_x] &= ~BOX;
		return 0;
	}

	// so the player is stuck. Is the level solved?

	for (i = 0 ; i < height ; i++)
		for (j = 0; j < width; j++)
		{
			if (b[i][j] & BOX)
				if ((initial_board[i][j] & BOX) == 0)
					ok = 0;
		}

	b[y][x] |= BOX;
	b[next_y][next_x] &= ~BOX;

	return 1 - ok;
}
*/

int simple_pull_box_eliminator(board b)
{
	// eliminate a box by pulling it one square
	int i, j, k;
	int sok_y, sok_x, to_y, to_x;
	int changed = 0;

	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			if ((b[i][j] & BOX) == 0) continue;

			for (k = 0; k < 4; k++)
			{
				sok_y = i + delta_y[k];
				sok_x = j + delta_x[k];

				if ((b[sok_y][sok_x] & SOKOBAN) == 0)
					continue;

				to_y = i + delta_y[k] * 2;
				to_x = j + delta_x[k] * 2;

				if (b[to_y][to_x] & OCCUPIED) continue;

//				if (stuck_after_pull(b, i, j, k)) continue;

				b[i][j] &= ~BOX;
				b[i][j] |= SOKOBAN;
				changed = 1;
				break; // k				
			}
		}
	}
	return changed;
}

void back_eliminate_loop(board e)
{
	int res;

	do
	{
		res = simple_pull_box_eliminator(e);
		if (res)
			expand_sokoban_cloud(e);
	} while (res);
}


int eliminate_from_zone(board base, board final_pos, int_board zones, int z)
{
	board e;
	int i, j;

	copy_board(final_pos, e);

	put_sokoban_in_zone(e, zones, z);

	back_eliminate_loop(e);

	// so now we are left only with boxes that can never be moved.

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (e[i][j] & BOX)
				if ((base[i][j] & BOX) == 0)
					return 0;
	return 1;
}

int try_pulling_all_zones(board b_in)
{
	board final;
	int_board zones;
	int zones_num;
	int res, i;
	board b;

	turn_fixed_boxes_into_walls(b_in, b);
	clear_boxes(b, final);
	turn_targets_into_packed(final);

	zones_num = mark_connectivities(final, zones);

	for (i = 0; i < zones_num; i++)
	{
		res = eliminate_from_zone(b, final, zones, i);
		if (res == 1)
			return 1; // could eliminate all boxes from this zone
	}
	return 0;
}




int free_pull_deadlock(board b_in)
{
	board b;
	int i, j;

	copy_board(b_in, b);

	back_eliminate_loop(b);

	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			if (b[i][j] & BOX)
				if ((initial_board[i][j] & BOX) == 0)
				{
//					printf("found free pull deadlock\n");
//					print_board(b_in);
//					my_getch();
					return 1;
				}
		}
	}
	return 0;
}

int can_played_reversed(board b, move *m, int pull_mode, int search_mode)
{
	board c,w;
	int y,x, has_corral;
	board corral_options;

	if (search_mode != BASE_SEARCH) return 1;

	/*
	
	For base search, the packing order requires the exact position of boxes. 
	If a box is pushed due to a corral, it will no longer be in place.
	So we try to avoid pulls that create corrals.
	The pull itself is not pruned; but all subsequent pulls will be marked
	as deadlock brcause they don't deal with the corral.
	
	*/

	if (pull_mode == 0) exit_with_error("bad call");

	if (search_mode == GIRL_SEARCH) return 1;
	// the problem is that a backward move might be inserted to the search tree before executing it.
	// the move won't be able to execute due to the corral; but it is removed from other branches
	// in girl mode.

	copy_board(b, c);
	apply_move(c, m, NORMAL);

	if (board_is_solved(c, 1)) return 1; // (initial board might have multiple corrals)

	// now check if c could lead back to b. 
	// why not? corrals / inefficient moves

	index_to_y_x(m->to, &y, &x);

	// check if c has a corral, that does not include the pulled box
	if (get_connectivity(c) > 1)
	{
		clear_bases_inplace(c);
		turn_fixed_boxes_into_walls(c, w);
		has_corral = detect_corral(w, corral_options);
		if (has_corral)
		{
			if (m->base) return 0; // a box was killed while there was a corral

			if ((corral_options[y][x] & BOX) == 0)
			{
//				print_board(b);
//				print_board(c);
//				my_getch();
				return 0;
			}
		}
	}

	return 1;

}


int check_matching_deadlock(board b_in, int pull_mode, int search_mode)
{
	board b;
	int box_places[MAX_INNER];
	int box_num = 0;
	int target_places[MAX_INNER];
	int target_num = 0;
	int i, j, y, x;
	board empty, targets;
	graph_data *gd;

	box_mat cost;
	hungarian_solution sol;
	hungarian_cache *hc;

	int has_bases = 0;

	if (search_mode == DRAGONFLY)
		return 0; // dragonfly uses the distance to bases, no match problems here


	// see if this is BASE_SEARCH
	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (b_in[i][j] & BASE)
				has_bases = 1;


	if (pull_mode == 0)
	{
		turn_fixed_boxes_into_walls(b_in, b);

		for (i = 0; i < index_num; i++)
		{
			index_to_y_x(i, &y, &x);
			if (b[y][x] & BOX)
				box_places[box_num++] = i;
			if (b[y][x] & TARGET)
				target_places[target_num++] = i;
		}
	}
	else
	{
		for (i = 0; i < index_num; i++)
		{
			index_to_y_x(i, &y, &x);

			if (has_bases == 0)
			{
				if (initial_board[y][x] & BOX)
					box_places[box_num++] = i;
			}
			else
			{
				if (b_in[y][x] & BASE)
					box_places[box_num++] = i;
			}


			if (b_in[y][x] & BOX)
				target_places[target_num++] = i;
		}
	}


	if (pull_mode == 0) // in pull mode boxes can be eliminated
		if (box_num != target_num)
			return 1;

	if (box_num >= MAX_BOXES)
		exit_with_error("hungarian size overflow");

	for (i = 0; i < box_num; i++)
		for (j = 0; j < box_num; j++)
			cost[i][j] = 1;



	if (pull_mode == 0)
	{  // update the graph by the updated board

		gd = allocate_graph();

		clear_boxes(b, empty);
		build_graph(empty, 0, gd);

		make_holes_inactive(b, gd); // don't allow positions where the player is closed in a hole

		for (i = 0; i < box_num; i++)
		{
			index_to_y_x(box_places[i], &y, &x);

			find_targets_of_place_without_init_graph(y, x, targets, gd);

			for (j = 0; j < target_num; j++)
			{
				index_to_y_x(target_places[j], &y, &x);
				if (targets[y][x])
					cost[i][j] = 0;
			}
		}

		free(gd);
	}
	else
	{
		for (i = 0; i < box_num; i++)
			for (j = 0; j < target_num; j++)
				if (distance_from_to[box_places[i]][target_places[j]] < 1000000)
					cost[i][j] = 0;
	}

	hc = allocate_hungarian_cache(); // dummy, just for interface
	solve_hungarian(box_num, cost, &sol, hc);
	free(hc);

	if (pull_mode == 0)
	{
		if (sol.weight == 0)
			return 0;
	}
	else
	{
		if (sol.weight == (box_num - target_num))
			return 0;
	}

	return 1; // deadlock
}


int verify_simple_base_matching(board b, board board_with_bases)
{
	// check that every base has at least one box
	int i, j, y, x;
	int box_places[MAX_BOXES];
	int bases_places[MAX_BOXES];
	int box_num = 0;
	int bases_num = 0;
	int box_index, base_index;

	for (i = 0; i < index_num; i++)
	{
		index_to_y_x(i, &y, &x);

		if (board_with_bases[y][x] & BASE)
			bases_places[bases_num++] = i;

		if (b[y][x] & BOX)
			box_places[box_num++] = i;
	}

	if (bases_num != box_num)
	{
		print_board(b);
		printf("%d %d\n", bases_num, box_num);
		exit_with_error("Num box mismatch");
	}

	for (i = 0; i < bases_num; i++)
	{
		base_index = bases_places[i];

		for (j = 0; j < box_num; j++)
		{
			box_index = box_places[j];
			if (distance_from_to[base_index][box_index] != 1000000)
				break;
		}
		if (j == box_num)
		{
			//	printf("found simple match problem\n");
			//	print_board(b);
			//	my_getch();
			return 0;
		}
	}
	return 1;

}

int start_zone_is_deadlocked(board b)
{
	int res;
	res = wobblers_search_actual(b, 1, 1000);

	if (res == 1)
		return 1;
	return 0;
}


int board_is_deadlocked(board b, int pull_mode, int search_mode)
{
	if (check_matching_deadlock(b, pull_mode, search_mode))
		return 1;

	if (pull_mode)
	{
		if (free_pull_deadlock(b))
			return 1;
	}

	if (pull_mode == 0)
		if (try_pulling_all_zones(b) == 0)
			return 1;

	if (is_stuck_deadlock(b, pull_mode))
		return 1;

	if (get_connectivity(b) != 1)
		if (corral_deadlock(b, pull_mode))
			return 1;


	if (pull_mode == 0)
	{
		if (is_diag_deadlock(b))
			return 1;

		if (is_k_dist_deadlock(b))
			return 1;
	}

	if (is_wobblers_deadlock(b, pull_mode))
		return 1;

	return 0;
}


int move_is_deadlocked(board b_in, move *m, int pull_mode, int search_mode)
{
	board b;
	int res;

	if ((m->kill) || (m->base)) return 0;

	copy_board(b_in, b);
	apply_move(b, m, NORMAL);

	res = board_is_deadlocked(b, pull_mode, search_mode);

	if (res) return 1;

	if (is_rooms_deadlock(b, pull_mode, m))
		return 1;

	return 0;
}