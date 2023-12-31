#include <stdlib.h>
#include <stdio.h>

#include "wobblers.h"
#include "util.h"
#include "fixed_boxes.h"
#include "positions.h"
#include "deadlock_cache.h"
#include "bfs.h"

int debug_wobblers_deadlock = 0;

#define NOT_DEADLOCK 0
#define IS_DEADLOCK  1
#define SEARCH_EXCEEDED 2

#define MAX_WOBBLERS_DEADLOCK_SEARCH 50

int reduce_moves_in_wobblers_search(move *moves, int moves_num)
{
	int i;
	int kill;
	int from_y, from_x, to_y, to_x;

	for (i = 0; i < moves_num; i++)
	{
		kill = 0;

		if (moves[i].attr.reversible)
			kill = 1; // a wobbler

		index_to_y_x(moves[i].from, &from_y, &from_x);
		index_to_y_x(moves[i].to, &to_y, &to_x);

		if (abs(from_y - to_y) >= 2)
			if (abs(from_x - to_x) >= 2)
				kill = 1;

		if (kill)
		{
			moves[0] = moves[i];
			moves[0].kill = 1;
			return 1;
		}
	}

	return moves_num;
}

int box_can_be_pulled(board b, int y, int x)
{
	int i;

	for (i = 0; i < 4; i++)
		if (b[y + delta_y[i]][x+delta_x[i]] & SOKOBAN)
			if (b[y + delta_y[i]*2][x + delta_x[i]*2] & SOKOBAN)
				return 1;
	return 0;
}

void remove_boxes_with_no_player_access(board b, int pull_mode, int with_diag)
{
	int i, j, k, val;
	int touched;
	int untouched;

	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			if ((b[i][j] & BOX) == 0) continue;

			touched = 0;
			untouched = 0;

			for (k = 0; k < 8; k++)
			{
				if ((with_diag == 0) && (k & 1)) continue;

				val = b[i + delta_y_8[k]][j + delta_x_8[k]];
				if (val & SOKOBAN)
					touched++;

				if ((val & OCCUPIED) == 0)
					if ((val & SOKOBAN) == 0)
						untouched++;
			}

			if (touched == 0)
			{
				b[i][j] &= ~BOX;
				continue;
			}

			if (untouched) 
				if (box_can_be_pulled(b,i,j))
					b[i][j] &= ~BOX;
		}
	}

	expand_sokoban_cloud(b);
}

int push_is_reversible(board b_in, int y, int x, int d)
{
	board b;
	// y,x - box. d - direction of push
	if ((b_in[y - delta_y[d]][x - delta_x[d]] & SOKOBAN) == 0) return 0;
	if ((b_in[y + delta_y[d]][x + delta_x[d]] & OCCUPIED)) return 0;
	if ((b_in[y + 2*delta_y[d]][x + 2*delta_x[d]] & OCCUPIED)) return 0;

	copy_board(b_in, b);

	b[y][x] &= ~BOX;
	b[y + delta_y[d]][x + delta_x[d]] |= BOX;
	clear_sokoban_inplace(b);
	b[y][x] |= SOKOBAN;
	expand_sokoban_cloud(b);

	if (b[y + 2 * delta_y[d]][x + 2 * delta_x[d]] & SOKOBAN)
		return 1;
	return 0;
}

int pull_is_reversible(board b_in, int y, int x, int d)
{
	board b;

	if ((b_in[y + delta_y[d]][x + delta_x[d]] & SOKOBAN) == 0) return 0;
	if ((b_in[y - delta_y[d]][x - delta_x[d]] & OCCUPIED)) return 0;
	if ((b_in[y + 2 * delta_y[d]][x + 2 * delta_x[d]] & OCCUPIED)) return 0;

	copy_board(b_in, b);

	b[y][x] &= ~BOX;
	b[y + delta_y[d]][x + delta_x[d]] |= BOX;
	clear_sokoban_inplace(b);
	b[y + 2 * delta_y[d]][x + 2 * delta_x[d]] |= SOKOBAN;
	expand_sokoban_cloud(b);

	if (b[y][x] & SOKOBAN)
		return 1;
	return 0;
}


void remove_wobblers(board b, int pull_mode)
{
	int changed = 1;
	int i, j, k;

	while (changed)
	{
		changed = 0;

		for (i = 0; i < height; i++)
		{
			for (j = 0; j < width; j++)
			{
				if ((b[i][j] & BOX) == 0) continue;

				for (k = 0; k < 4; k++)
				{
					if (pull_mode == 0)
					{
						if (push_is_reversible(b, i, j, k) == 0) continue;
					}
					else
					{
						if (pull_is_reversible(b, i, j, k) == 0) continue;
					}

					b[i][j] &= ~BOX;
					b[i][j] |= SOKOBAN;
					changed = 1;
					break;
				}
			}
		}

		if (changed)
			expand_sokoban_cloud(b);
	}
}



void fill_wobblers_scores(board b, score_element *s, int pull_mode)
{
	// this is ugly... but I hate adding another field to score_element
	if (pull_mode == 0)
		s->boxes_on_targets = boxes_on_targets(b);
	else
		s->boxes_on_targets = boxes_on_bases(b);
}

int is_better_wobblers_score(score_element *new_score, score_element *old_score)
{
	if (new_score->boxes_in_level < old_score->boxes_in_level) return 1;
	if (new_score->boxes_in_level > old_score->boxes_in_level) return 0;

	if (new_score->connectivity < old_score->connectivity) return 1;
	if (new_score->connectivity > old_score->connectivity) return 0;

	if (new_score->boxes_on_targets > old_score->boxes_on_targets) return 1;
	if (new_score->boxes_on_targets < old_score->boxes_on_targets) return 0;

	return 0;
}


int get_best_wobblers_move(position *positions, int pos_num, int *son)
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

			if ((best_pos == -1) || (is_better_wobblers_score(p->scores + j, &best_score)))
			{
				best_pos = i;
				*son = j;
				best_score = p->scores[j];
			}
		}
	}

	return best_pos;
}



int wobblers_search_actual(board b, int pull_mode, int max_positions)
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

	set_first_position(b, pull_mode, positions, WOBBLERS_SEARCH, &h);
	pos_num = 1;

	while (1)
	{
		total_leaves = estimate_uniqe_moves(positions, pos_num);
		if (total_leaves > max_positions)
		{
			res = SEARCH_EXCEEDED;
			break;
		}

		pos = get_best_wobblers_move(positions, pos_num, &son);

		if (pos == -1)
		{
			res = IS_DEADLOCK;
			break;
		}

		if (debug_wobblers_deadlock)
		{
			printf("wobblers search: position to expand:\n");
			print_position(positions + pos, 1);
		}

		expand_position(positions + pos, son, pull_mode, positions, pos_num, &h);
		pos_num++;

		if (debug_wobblers_deadlock)
		{
			printf("wobblers search: position after expansion\n");
			print_position(positions + pos_num - 1, 1);
			my_getch();
		}

		bytes_to_board(positions[pos_num - 1].board, c);
		if (all_boxes_in_place(c, pull_mode))
		{
			res = NOT_DEADLOCK;
			break;
		}

		if (pos_num == max_positions)
		{
			res = SEARCH_EXCEEDED;
			break;
		}
	}

	free_positions(positions, max_positions);

	return res;
}



int wobbler_deadlock_search(board b, int pull_mode)
{
	// this is a cache wrapper for wobblers_search_actual
	int res;

	UINT_64 hash = get_board_hash(b);

	res = get_from_deadlock_cache(hash, pull_mode, WOBBLERS_SEARCH);

	if ((res >= 0) && (res <= 2))
		return res;

	if (debug_wobblers_deadlock)
	{
		print_board(b);
		printf("BOARD HASH= %016llx\n\n", hash);
		my_getch();
	}

	res = wobblers_search_actual(b, pull_mode, MAX_WOBBLERS_DEADLOCK_SEARCH);

	insert_to_deadlock_cache(hash, res, pull_mode, WOBBLERS_SEARCH);

	if (res == IS_DEADLOCK)
	{
		if (verbose >= 4)
		{
//			printf("new wobblers deadlock pos:\n");
//			print_board(b);
//			my_getch();
		}
	}

	return res;
}




int is_wobblers_deadlock(board b_in, int pull_mode)
{
	board b;
	int n;
	int res;

	if (pull_mode == 0) 
		return 0; // wobblers search does not seem to be cost-effective in push mode.

	if (debug_wobblers_deadlock)
	{
		printf("debugging wobblers. in:\n");
		print_board(b_in);
	}

	if (pull_mode == 0)
		turn_fixed_boxes_into_walls(b_in, b);
	else
	{
		copy_board(b_in, b);
		clear_bases_inplace(b);
	}

	remove_boxes_with_no_player_access(b, pull_mode, 0);

	if (debug_wobblers_deadlock)
	{
		printf("debugging wobblers. contact:\n");
		print_board(b);
	}

	remove_wobblers(b, pull_mode);

	if (debug_wobblers_deadlock)
	{
		printf("debugging wobblers. without wobblers:\n");
		print_board(b);
	}


	n = boxes_in_level(b);
	if (n == 0) return 0;
	if (n == global_boxes_in_level) return 0;

	res = wobbler_deadlock_search(b, pull_mode);

	if (res == IS_DEADLOCK) return 1;
	return 0;
}