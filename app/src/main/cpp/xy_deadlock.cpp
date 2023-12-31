#include <stdlib.h>
#include <stdio.h>

#include "xy_deadlock.h"
#include "positions.h"
#include "util.h"
#include "deadlock_cache.h"
#include "fixed_boxes.h"
#include "bfs.h"
#include "deadlock_utils.h"
#include "wobblers.h"

int debug_xy_deadlock = 0;

#define NOT_DEADLOCK 0
#define IS_DEADLOCK  1
#define SEARCH_EXCEEDED 2

#define MAX_XY_DEADLOCK_SEARCH 500


void mark_hv_indices(board b, move *moves, int moves_num, int *indices)
{
	char h_indices[MAX_INNER];
	char v_indices[MAX_INNER];
	int from_x,from_y,to_x,to_y, i, from, to;
	board c;
	char con_ok[MAX_MOVES];

	for (i = 0; i < moves_num; i++)
	{
		con_ok[i] = 0;
		copy_board(b, c);
		apply_move(c, moves + i, NORMAL);

		index_to_y_x(moves[i].from, &from_y, &from_x);

		if (c[from_y][from_x] & SOKOBAN)
			con_ok[i] = 1;
	}

	for (i = 0; i < index_num; i++)
		h_indices[i] = v_indices[i] = 0;

	for (i = 0; i < moves_num; i++)
	{
		if (con_ok[i] == 0)
			continue;

		from = moves[i].from;
		to = moves[i].to;
		index_to_y_x(from, &from_y, &from_x);
		index_to_y_x(to, &to_y, &to_x);

		if (to_y != from_y) v_indices[from] = 1;
		if (to_x != from_x) h_indices[from] = 1;
	}

	for (i = 0; i < index_num; i++)
		indices[i] = v_indices[i] & h_indices[i];
}

int reduce_moves_in_xy_search(board b, move *moves, int moves_num)
{
	int hv_indices[MAX_INNER];
	int i;

	mark_hv_indices(b, moves, moves_num, hv_indices);

	for (i = 0; i < moves_num; i++)
	{
		if (hv_indices[moves[i].from]) 
		{
			moves[0] = moves[i];
			moves[0].kill = 1;
			return 1;
		}
	}
	return moves_num;
}


int is_better_xy_score(score_element *new_score, score_element *old_score)
{
	if (new_score->boxes_in_level < old_score->boxes_in_level) return 1;
	if (new_score->boxes_in_level > old_score->boxes_in_level) return 0;

	if (new_score->connectivity < old_score->connectivity) return 1;
	if (new_score->connectivity > old_score->connectivity) return 0;

	if (new_score->boxes_on_targets > old_score->boxes_on_targets) return 1;
	if (new_score->boxes_on_targets < old_score->boxes_on_targets) return 0;

	return 0;
}



int get_best_xy_move(position *positions, int pos_num, int *son)
{
	int i, j;
	position *p;
	int best_pos = -1;
	score_element best_score;

	for (i = 0; i < pos_num; i++)
	{
		p = positions + i;
		if (p->position_deadlocked) continue;

		for (j = 0; j < p->moves_num; j++)
		{
			if (p->move_deadlocked[j]) continue;
			if (p->place_in_positions[j]) continue;

			if ((best_pos == -1) || (is_better_xy_score(p->scores + j, &best_score)))
			{
				best_pos = i;
				*son = j;
				best_score = p->scores[j];
			}
		}
	}

	return best_pos;
}

void show_deadlocking(position* positions, int pos_num)
{
	int best = 0;
	int i;

	for (i = 0; i < pos_num; i++)
		if (positions[i].position_score.boxes_in_level <
			positions[best].position_score.boxes_in_level)
			best = i;

	printf("DEADLOCKING:\n");
	print_position(positions + best, 0);
	my_getch();
}

int xy_search_actual(board b, int pull_mode, int max_positions)
{
	position *positions;
	int pos_num = 0;
	int res = 0;
	int pos, son;
	board c;
	int total_leaves;

	helper h;
	init_helper(&h);

	if (all_boxes_in_place(b, pull_mode))
		return NOT_DEADLOCK;

	positions = allocate_positions(max_positions);

	set_first_position(b, pull_mode, positions, XY_SEARCH, &h);
	pos_num = 1;

	while (1)
	{
		total_leaves = estimate_uniqe_moves(positions, pos_num);
		if (total_leaves > max_positions)
		{
			res = SEARCH_EXCEEDED;
			break;
		}

		pos = get_best_xy_move(positions, pos_num, &son);

		if (pos == -1)
		{
			res = IS_DEADLOCK;
			break;
		}

		if (debug_xy_deadlock)
		{
			printf("xy search: position to expand:\n");
			print_position(positions + pos, 1);
		}

		expand_position(positions + pos, son, pull_mode, positions, pos_num, &h);
		pos_num++;

		if (debug_xy_deadlock)
		{
			printf("xy search: position after expansion with %d\n", son);
			print_position(positions + pos_num - 1, 1);
			my_getch();
		}

		bytes_to_board(positions[pos_num - 1].board, c);
		if (all_boxes_in_place(c, pull_mode))
		{
			if (debug_xy_deadlock)
				show_position_path(positions + pos_num - 1);

			res = NOT_DEADLOCK;
			break;
		}

		if (pos_num == max_positions)
		{
			res = SEARCH_EXCEEDED;
			break;
		}
	}

	if ((res == IS_DEADLOCK) && (verbose >= 5))
		show_deadlocking(positions, pos_num);

	free_positions(positions, max_positions);

	return res;
}



int xy_deadlock_search(board b, int pull_mode)
{
	// this is a cache wrapper for xy_search_actual
	int res;

	UINT_64 hash = get_board_hash(b);

	res = get_from_deadlock_cache(hash, pull_mode, XY_SEARCH);

	if ((res >= 0) && (res <= 2))
		return res;

	if (debug_xy_deadlock)
	{
		print_board(b);
		printf("BOARD HASH= %016llx\n\n", hash);
		my_getch();
	}

	res = xy_search_actual(b, pull_mode, MAX_XY_DEADLOCK_SEARCH);

	insert_to_deadlock_cache(hash, res, pull_mode, XY_SEARCH);

	if (res == IS_DEADLOCK)
	{
		if (verbose >= 4)
		{
//			printf("new xy deadlock pos:\n");
//			print_board(b);
//			my_getch();
		}
	}


	if (res == 2)
	{
		if (verbose >= 5)
		{
			printf("WARNING: XY_SEARCH exceeded\n");
//			print_board(b);
		}
	}

	return res;
}


void clear_xy_boxes(board b, int pull_mode)
{
	int moves_num, y, x;
	move moves[MAX_MOVES];
	int corral;

	helper h;
	init_helper(&h);

	while (1)
	{
		moves_num = find_possible_moves(b, moves, pull_mode, &corral, XY_SEARCH, &h);

		if ((moves_num > 0) && (moves[0].kill))
		{
			index_to_y_x(moves[0].from, &y, &x);
			b[y][x] &= ~BOX;
			expand_sokoban_cloud(b);
		}
		else
			return;
	}
}



int is_xy_deadlock(board b_in, int pull_mode)
{
	board b;
	int res;

//	if (pull_mode == 0)	return 0; 

	if (debug_xy_deadlock)
	{
		printf("debugging xy deadlock. in:\n");
		print_board(b_in);
	}

	if (pull_mode == 0)
		turn_fixed_boxes_into_walls(b_in, b);
	else
	{
		copy_board(b_in, b);
		clear_bases_inplace(b);
	}


//	if (get_connectivity(b) != 1) return 0;

	res = xy_deadlock_search(b, pull_mode);

	if (res == IS_DEADLOCK) return 1;
	return 0;
}
