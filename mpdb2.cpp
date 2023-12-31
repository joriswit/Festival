// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include <stdlib.h>
#include <stdio.h>

#include "global.h"
#include "util.h"
#include "mpdb2.h"
#include "distance.h"
#include "hotspot.h"
#include "bfs.h"
#include "deadlock.h"
#include "mini_search.h"
#include "xy_deadlock.h"

#define MAX_MPDB_PATTERNS 50

int mpdb_list[MAX_MPDB_PATTERNS][2];
int mpdb_num = 0;

board mpdb_board;

void add_to_mpdb(int index1, int index2)
{
	int y, x;

	mpdb_list[mpdb_num][0] = index1;
	mpdb_list[mpdb_num][1] = index2;

	mpdb_num++;

	index_to_y_x(index1, &y, &x); 
	mpdb_board[y][x] = 1;
	index_to_y_x(index2, &y, &x);
	mpdb_board[y][x] = 1;


	if (mpdb_num >= MAX_MPDB_PATTERNS)
	{
		if (verbose >= 4)
			printf("mpdb overflow !\n");
		mpdb_num--;
	}
}

int is_pull_mpdb_deadlock(board b);

int is_mpdb_deadlock(board b, int pull_mode)
{
	int i, y, x;

	if (pull_mode)
		return is_pull_mpdb_deadlock(b);

	for (i = 0; i < mpdb_num; i++)
	{
		index_to_y_x(mpdb_list[i][0], &y, &x);
		if ((b[y][x] & BOX) == 0) continue;

		index_to_y_x(mpdb_list[i][1], &y, &x);
		if ((b[y][x] & BOX) == 0) continue;

		return 1;
	}
	return 0;
}



int two_near_wall(board b_in)
{
	int i, j, a, b;
	int n_box, n_wall;
	for (i = 0; i < (height - 1); i++)
		for (j = 0; j < (width - 1); j++)
		{
			n_box = 0;
			n_wall = 0;
			for (a = 0; a < 2; a++)
				for (b = 0; b < 2; b++)
				{
					if (b_in[i + a][j + b] == WALL) n_wall++;
					if (b_in[i + a][j + b] & BOX) n_box++;
				}
			if ((n_box == 2) && (n_wall == 2))
				return 1;
		}
	return 0;
}

int check_known_deadlock(board b)
{
	int i, j;
	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if ((b[i][j] & BOX) == 0) continue;
			
			if (check_all_patterns(b, i, j, 0) != -1) // 0 - push mode
				return 1;
		}
	return 0;
}

void build_mpdb2()
{
	int i, j,y,x;
	board empty_board, b;

	mpdb_num = 0;
	zero_board(mpdb_board);

	if (global_boxes_in_level == 1) return;

	clear_boxes(initial_board, empty_board);
	clear_sokoban_inplace(empty_board);

	for (i = 0; i < index_num; i++)
	{
		if (impossible_place[i]) continue;
		index_to_y_x(i, &y, &x); 
		if (forbidden_push_tunnel[y][x]) continue;

		for (j = i + 1; j < index_num; j++)
		{
			if (impossible_place[j]) continue;
			index_to_y_x(j, &y, &x); 
			if (forbidden_push_tunnel[y][x]) continue;

			if (double_blocking(i, j) == 0)
				continue;

			copy_board(empty_board, b);
			index_to_y_x(i, &y, &x);
			b[y][x] |= BOX;
			index_to_y_x(j, &y, &x);
			b[y][x] |= BOX;

			if (all_boxes_in_place(b,0)) continue;

			if (get_connectivity(b) > 1) continue; // can't know side of sokoban

			if (two_near_wall(b)) continue;

			if (check_known_deadlock(b)) continue;

			put_sokoban_everywhere(b);

			if (can_put_all_on_targets(b, 100)) // main search 
				continue;  // note that a "-1" result means search aborted

			if (verbose >= 5)
			{
				printf("two pattern:\n");
				print_board(b);
				// my_getch();
			}

			add_to_mpdb(i, j);
		}
	}
}






int pull_mpdb_list[MAX_MPDB_PATTERNS][2];
int pull_mpdb_num = 0;

board pull_mpdb_board;

void add_to_pull_mpdb(int index1, int index2)
{
	int y, x;

	pull_mpdb_list[pull_mpdb_num][0] = index1;
	pull_mpdb_list[pull_mpdb_num][1] = index2;

	pull_mpdb_num++;

	index_to_y_x(index1, &y, &x);
	pull_mpdb_board[y][x] = 1;
	index_to_y_x(index2, &y, &x);
	pull_mpdb_board[y][x] = 1;


	if (pull_mpdb_num >= MAX_MPDB_PATTERNS)
	{
		if (verbose >= 4)
			printf("pull mpdb overflow !\n");
		pull_mpdb_num--;
	}
}

int very_pullable(board b, int y, int x)
{
	int i, sum = 0;
	for (i = 0; i < 4; i++)
		if ((b[y + delta_y[i]][x + delta_x[i]] & OCCUPIED) == 0)
			if ((b[y + 2 * delta_y[i]][x + 2 * delta_x[i]] & OCCUPIED) == 0)
				sum++;
	return sum == 4;
}



void build_pull_mpdb2()
{
	int i, j, y1, x1, y2, x2, d;
	board empty_board, b;

	pull_mpdb_num = 0;
	zero_board(pull_mpdb_board);

	if (global_boxes_in_level == 1) return;

	clear_boxes(initial_board, empty_board);
	clear_sokoban_inplace(empty_board);

	for (i = 0; i < index_num; i++)
	{
		if (impossible_place[i]) continue;
		index_to_y_x(i, &y1, &x1);
		if (forbidden_pull_tunnel[y1][x1]) continue;

		for (j = i + 1; j < index_num; j++)
		{
			if (impossible_place[j]) continue;
			index_to_y_x(j, &y2, &x2);
			if (forbidden_pull_tunnel[y2][x2]) continue;

			if ((y1 != y2) && (x1 != x2)) continue;

			d = abs(y1 - y2) + abs(x1 - x2);
			if (d == 1) continue;
			if (d >= 10) continue;
			if (bfs_distance_from_to[i][j] != d) continue;

			copy_board(empty_board, b);
			b[y1][x1] |= BOX;
			b[y2][x2] |= BOX;

			if ((very_pullable(b, y1, x1) && very_pullable(b, y2, x2))) continue;

			if (all_boxes_in_place(b, 1)) continue;

			if (get_connectivity(b) > 1) continue; // can't know side of sokoban

			put_sokoban_everywhere(b);

			if (is_xy_deadlock(b, 1) == 0) continue;

			if (verbose >= 4)
			{
				printf("pull two pattern:\n");
				print_board(b);
//				my_getch();
			}

			add_to_pull_mpdb(i, j);
		}
	}
}


int is_pull_mpdb_deadlock(board b)
{
	int i, y, x;

	for (i = 0; i < pull_mpdb_num; i++)
	{
		index_to_y_x(pull_mpdb_list[i][0], &y, &x);
		if ((b[y][x] & BOX) == 0) continue;

		index_to_y_x(pull_mpdb_list[i][1], &y, &x);
		if ((b[y][x] & BOX) == 0) continue;

		return 1;
	}
	return 0;
}