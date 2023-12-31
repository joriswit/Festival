#include <stdio.h>

#include "rooms_deadlock.h"
#include "rooms.h"
#include "board.h"
#include "util.h"
#include "score.h"
#include"positions.h"
#include "deadlock_cache.h"
#include "bfs.h"

board room_with_corridors[MAX_ROOMS];
int room_size[MAX_ROOMS];

int debug_rooms_deadlock = 0;

void init_rooms_deadlock()
{
	int t, i, j, k;
	board b;

	copy_board(initial_board, b);
	clear_boxes_inplace(b);

	for (t = 0; t < rooms_num; t++)
	{
		zero_board(room_with_corridors[t]);

		for (i = 0; i < height; i++)
			for (j = 0; j < width; j++)
			{
				if (rooms_board[i][j] == t)
				{
					room_with_corridors[t][i][j] = 1;

					for (k = 0; k < 4; k++)
					{
						if ((b[i + delta_y[k]][j + delta_x[k]] & OCCUPIED) == 0)
							room_with_corridors[t][i + delta_y[k]][j+delta_x[k]] = 1;
					}
				}
			}

		room_size[t] = board_popcnt(room_with_corridors[t]);
	}
}


#define NOT_DEADLOCK 0
#define IS_DEADLOCK  1
#define SEARCH_EXCEEDED 2

#define MAX_ROOMS_DEADLOCK_SEARCH 200


int reduce_moves_in_rooms_deadlock_search(move *moves, int moves_num, helper *h)
{
	int i;
	int y, x;

	for (i = 0; i < moves_num; i++)
	{
		index_to_y_x(moves[i].to, &y, &x);

		if (rooms_board[y][x] == 1000000) continue;

		if (rooms_board[y][x] != h->tested_deadlock_room)
		{
			moves[0] = moves[i];
			moves[0].kill = 1;
			return 1;
		}
	}

	return moves_num;
}

int boxes_in_room(board b, int r)
{
	int i, j;
	int sum = 0;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (room_with_corridors[r][i][j])
				if (b[i][j] & BOX)
					sum++;
	return sum;
}



int is_better_rooms_deadlock_score(score_element *new_score, score_element *old_score)
{
	if (new_score->boxes_in_level < old_score->boxes_in_level) return 1;
	if (new_score->boxes_in_level > old_score->boxes_in_level) return 0;

	if (new_score->connectivity < old_score->connectivity) return 1;
	if (new_score->connectivity > old_score->connectivity) return 0;

	if (new_score->boxes_on_targets > old_score->boxes_on_targets) return 1;
	if (new_score->boxes_on_targets < old_score->boxes_on_targets) return 0;

	return 0;
}


int get_best_rooms_deadlock_move(position *positions, int pos_num, int *son)
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

			if ((best_pos == -1) || (is_better_rooms_deadlock_score(p->scores + j, &best_score)))
			{
				best_pos = i;
				*son = j;
				best_score = p->scores[j];
			}
		}
	}

	return best_pos;
}



int rooms_deadlock_search_actual(board b, int pull_mode, int tested_deadlock_room)
{
	position *positions;
	int pos_num = 0;
	int res = 0;
	int pos, son;
	board c;
	int total_leaves;
	helper h;

	init_helper(&h);
	h.tested_deadlock_room = tested_deadlock_room;

	if (all_boxes_in_place(b, pull_mode))
		return NOT_DEADLOCK;

	positions = allocate_positions(MAX_ROOMS_DEADLOCK_SEARCH);

	set_first_position(b, pull_mode, positions, ROOMS_DEADLOCK_SEARCH, &h);
	pos_num = 1;

	while (1)
	{
		total_leaves = estimate_uniqe_moves(positions, pos_num);
		if (total_leaves > MAX_ROOMS_DEADLOCK_SEARCH)
		{
			res = SEARCH_EXCEEDED;
			break;
		}

		pos = get_best_rooms_deadlock_move(positions, pos_num, &son);

		if (pos == -1)
		{
			res = IS_DEADLOCK;
			break;
		}

		if (debug_rooms_deadlock)
		{
			printf("rooms deadlock search: position to expand:\n");
			print_position(positions + pos, 1);
		}

		expand_position(positions + pos, son, pull_mode, positions, pos_num, &h);
		pos_num++;

		if (debug_rooms_deadlock)
		{
			printf("rooms deadlock search: position after expansion\n");
			print_position(positions + pos_num - 1, 1);
			my_getch();
		}
		
		bytes_to_board(positions[pos_num - 1].board, c);
		if (all_boxes_in_place(c, pull_mode))
		{
			res = NOT_DEADLOCK;
			break;
		}

		if (pos_num == MAX_ROOMS_DEADLOCK_SEARCH)
		{
			res = SEARCH_EXCEEDED;
			break;
		}
	}

	free_positions(positions, MAX_ROOMS_DEADLOCK_SEARCH);

	return res;
}



int rooms_deadlock_search(board b, int pull_mode, int tested_deadlock_room)
{
	// this is a cache wrapper for rooms_deadlock_search_actual
	int res;

	UINT_64 hash = get_board_hash(b);

	res = get_from_deadlock_cache(hash, pull_mode, ROOMS_DEADLOCK_SEARCH);

	if ((res >= 0) && (res <= 2))
		return res;

	if (debug_rooms_deadlock)
	{
		print_board(b);
		printf("BOARD HASH= %016llx\n\n", hash);
		my_getch();
	}

	res = rooms_deadlock_search_actual(b, pull_mode, tested_deadlock_room);

	insert_to_deadlock_cache(hash, res, pull_mode, ROOMS_DEADLOCK_SEARCH);

	if (res == IS_DEADLOCK)
	{
		if (verbose >= 4)
		{
			printf("new rooms deadlock pos: %s\n", (pull_mode ? "(pull mode)" : ""));
			print_board(b);

//			printf("hash= %016llx\n", hash);
//			my_getch();
		}
	}

	return res;
}

void keep_boxes_in_room(board b, int r)
{
	int i, j;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (room_with_corridors[r][i][j] == 0)
				b[i][j] &= ~BOX;

	expand_sokoban_cloud(b);
}


int is_rooms_deadlock(board b_in, int pull_mode, move *last_move)
{
	board b, c;
	int n,res,t, y, x;
	int tested_deadlock_room;

	if (last_move->base || last_move->kill) return 0;

	if (rooms_num == 1) return 0;

	index_to_y_x(last_move->to, &y, &x);

	if (debug_rooms_deadlock)
	{
		printf("debugging rooms deadlock:\n");
		print_board(b_in);
	}

	copy_board(b_in, b);
	if (pull_mode)
		clear_bases_inplace(b);

	for (t = 0; t < rooms_num; t++)
	{
		// check if the box was moved to room t
		if (room_with_corridors[t][y][x] == 0) continue;

		if (room_size[t] >= 25) continue;

		n = boxes_in_room(b, t);
		if ((n <= 1) || (n > 5)) continue;

		if ((n == 5) && (room_size[t] > 17)) continue;
		if ((n == 4) && (room_size[t] > 20)) continue;

		tested_deadlock_room = t;

		copy_board(b, c);
		keep_boxes_in_room(c, t);

		res = rooms_deadlock_search(c, pull_mode, tested_deadlock_room);

		if (res == 1)
			return 1;
	}
	return 0;
}

int is_in_room_with_corridor(int r, int y, int x)
{
	return room_with_corridors[r][y][x];
}