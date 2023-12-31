// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include <stdio.h>
#include <stdlib.h>

#include "moves.h"
#include "graph.h"
#include "deadlock.h"
#include "distance.h"
#include "corral.h"
#include "deadlock_utils.h"
#include "fixed_boxes.h"
#include "packing_search.h"
#include "perimeter.h"
#include "util.h"
#include "k_dist_deadlock.h"
#include "mini_search.h"
#include "wobblers.h"
#include "bfs.h"
#include "heuristics.h"
#include "debug.h"
#include "rooms_deadlock.h"
#include "xy_deadlock.h"
#include "corral_deadlock.h"

int box_can_be_moved(board b, int index, int pull_mode)
{
	int i,y,x;

	index_to_y_x(index, &y, &x);

	if (pull_mode == 0)
	{
		for (i = 0; i < 4; i++)
		{
			if ((b[y + delta_y[i]][x + delta_x[i]] & OCCUPIED)) continue;
			if ((b[y - delta_y[i]][x - delta_x[i]] & SOKOBAN) == 0) continue;
			if (deadlock_in_direction(b, y, x, i)) continue;
			return 1;
		}
		return 0;
	}

	// pull mode

	for (i = 0; i < 4; i++)
	{
		if ((b[y + delta_y[i]][x + delta_x[i]] & SOKOBAN) == 0) continue;
		if ((b[y + 2*delta_y[i]][x + 2*delta_x[i]] & OCCUPIED)) continue;
		return 1;
	}
	return 0;
}


int find_box_moves(board b, int index, move *moves, int pull_mode, int search_mode,
	board possible_board, graph_data* gd)
{
	int y, x;
	int moves_num;
	int i;		

	if (box_can_be_moved(b, index, pull_mode) == 0)
		exit_with_error("box can't be moved");

	index_to_y_x(index, &y, &x);

	// find situation-specific deadlocks, in addition to the level deadlocks
	set_current_impossible(b, y, x, pull_mode, search_mode,
		possible_board, gd);

	mark_box_moves_in_graph(b, index, pull_mode, gd);

	if ((search_mode == NORMAL) || 
		(search_mode == WOBBLERS_SEARCH))
		find_reversible_moves(b, index, pull_mode, gd);

	moves_num = get_box_moves_from_graph(index, moves, gd);

	for (i = 0; i < moves_num; i++)
		moves[i].pull = pull_mode;

//	free(gd);

	return moves_num;
}



int keep_valid_moves(move *moves, int *valid, int moves_num)
{
	int i;
	int p = 0;

	for (i = 0; i < moves_num; i++)
		if (valid[i])
			moves[p++] = moves[i];

	return p;
}


void get_possible_places(board b, int pull_mode, int search_mode, board possible)
{
	// set situation-specific possible places for boxes
	int i, j;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			possible[i][j] = inner[i][j];

	if (pull_mode == 0)
	{
		get_srcs_of_all_targets(b, possible);
		return;
	}

	if (search_mode == BASE_SEARCH)
		compute_possible_from_bases(b, possible);

}


int remove_deadlocked_moves(board b, move* moves, int moves_num, int pull_mode, int search_mode,
	helper* h)
{
	int valid[MAX_MOVES];

	check_deadlock_in_moves(b, moves, moves_num, valid, pull_mode, search_mode, h);
	moves_num = keep_valid_moves(moves, valid, moves_num);

	return moves_num;
}

int detect_kill_moves(board b_in, board b, move* moves, int moves_num,
	int pull_mode, int search_mode, helper* h)
{
	if (search_mode == DEADLOCK_SEARCH)
		moves_num = check_for_deadlock_zone_moves(b_in, moves, moves_num);

	if (search_mode == K_DIST_SEARCH)
		moves_num = check_for_k_dist_moves(b, moves, moves_num, h);

	if (search_mode == WOBBLERS_SEARCH)
		moves_num = reduce_moves_in_wobblers_search(moves, moves_num);

	if (search_mode == ROOMS_DEADLOCK_SEARCH)
		moves_num = reduce_moves_in_rooms_deadlock_search(moves, moves_num, h);

	if (search_mode == XY_SEARCH)
		moves_num = reduce_moves_in_xy_search(b, moves, moves_num);

	return moves_num;
}



int find_possible_moves(board b_input, move *moves, int pull_mode,
						int *has_corral, int search_mode, helper *h)
{
	int moves_num = 0;
	int i, y, x;
	int box_moves_num;
	board b,c;
	board corral_options;
	int use_corral = 1;
	board possible_board;
	move* box_moves;

	graph_data* gd;
	gd = allocate_graph(); // allocate once here for all box moves


	*has_corral = 0;

	if (pull_mode == 0)
	{
		clear_deadlock_zone(b_input, c);
		turn_fixed_boxes_into_walls(c, b);
	}
	else
	{
		clear_deadlock_zone(b_input, b);
	}

	get_possible_places(b, pull_mode, search_mode, possible_board);

	if (search_mode == DEADLOCK_SEARCH) use_corral = 0;
	// the problem is that areas behind the deadlock zone might have packed boxes.
	// these boxes are removed in the deadlock search, making the targets seem empty - so there
	// seems to be work in the corral even if there isn't.
	
	if (search_mode == ROOMS_DEADLOCK_SEARCH) use_corral = 0; // same issue here
	if (search_mode == MINI_SEARCH) use_corral = 0; 
	if (search_mode == XY_SEARCH) use_corral = 0;
	if (search_mode == K_DIST_SEARCH) use_corral = 0;

	if (search_mode == IGNORE_CORRAL)
	{
		use_corral = 0;
		search_mode = NORMAL;
	}

	if (h->perimeter_found)          use_corral = 0;
	if (h->enable_inefficient_moves) use_corral = 0;

	if (use_corral)
		if (pull_mode == 0)
			*has_corral = detect_corral(b, corral_options);

	for (i = 0; i < index_num; i++)
	{
		index_to_y_x(i, &y, &x);

		if (*has_corral)
			if ((corral_options[y][x] & BOX) == 0)
				continue;

		if ((search_mode == REV_SEARCH) && pull_mode && (h->perimeter_found == 0))
			if (h->wallified[y][x]) continue; // box was wallified by the forward search

//		if (is_fast_frozen(y, x, pull_mode, search_mode)) continue;

		if ((b[y][x] & BOX) == 0) continue;
		if (box_can_be_moved(b, i, pull_mode) == 0) continue;

		box_moves = moves + moves_num;
		box_moves_num = find_box_moves(b, i, box_moves, pull_mode,
			search_mode, possible_board, gd);

		box_moves_num = remove_deadlocked_moves(b, box_moves, box_moves_num,
			pull_mode, search_mode, h);

		box_moves_num = detect_kill_moves(b_input, b, box_moves, box_moves_num,
			pull_mode, search_mode, h);

		if ((box_moves_num == 1) && (box_moves[0].kill))
		{
			moves[0] = box_moves[0];
			moves_num = 1;
			break;
		}

		moves_num += box_moves_num;

		if (moves_num > (MAX_MOVES - MAX_SIZE * MAX_SIZE))
		{
			if (verbose >= 4) printf("too many moves: %d moves\n", moves_num);
			sprintf(global_fail_reason, "Too many moves");
			break; // do not produce other moves
		}

	}

	free(gd);

	if (search_mode == BASE_SEARCH)
		moves_num = mark_sink_squares_moves(b, moves, moves_num);

	return moves_num;
}



void apply_move(board b, move *m, int search_type)
{
	int y, x, p;

	if (search_type == -1) exit_with_error("search type not initialized");

	if (m->kill)
	{
		index_to_y_x(m->from, &y, &x);
		b[y][x] &= ~BOX;
		expand_sokoban_cloud(b);

		if (search_type == DEADLOCK_SEARCH)
			update_deadlock_zone(b, m->pull);

		return;
	}

	clear_sokoban_inplace(b);

	index_to_y_x(m->from, &y, &x); // box to push: initial position

	if ((b[y][x] & BOX) == 0) exit_with_error("missing box2");
	b[y][x] &= ~BOX;

	index_to_y_x(m->to, &y, &x); // box to push: final position

	if (b[y][x] & OCCUPIED) exit_with_error("occupied place");

	b[y][x] |= BOX;

	if (m->base == 1)
	{
		b[y][x] &= ~BOX;
		if ((b[y][x] & BASE) == 0) exit_with_error("no base in apply");
		b[y][x] &= ~BASE;
	}

	p = m->sokoban_position;
	b[y + delta_y[p]][x + delta_x[p]] |= SOKOBAN;

	expand_sokoban_cloud(b);

	if (search_type == DEADLOCK_SEARCH)
		update_deadlock_zone(b, m->pull);
}


void print_move(move *m)
{
	int move_from_y, move_from_x;
	int move_to_y, move_to_x;

	index_to_y_x(m->from, &move_from_y, &move_from_x);
	index_to_y_x(m->to, &move_to_y, &move_to_x);

	if (m->kill)
	{
		printf(" kill %d %d ", move_from_y, move_from_x);
		return;
	}

	if (m->base)
	{
		printf(" base ");
	}

	printf("%2d %2d -> %2d %2d (%d) ",
		move_from_y, move_from_x,
		move_to_y, move_to_x, m->sokoban_position);

	printf("[%3d] ", m->attr.weight);

	if (m->attr.reversible)
		printf("[R");
	else
		printf("[-");

	printf("] ");
}



int attr_to_weight(move_attr *a)
{
	if (a->advisor == 1) return 0;
	return 1;
}
