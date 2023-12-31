// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include <stdio.h>

#include "board.h"
#include "global.h"
#include "util.h"
#include "deadlock_utils.h"
#include "bfs.h"

void get_deadlock_zone_indicator(board b, board dz)
{
	int i, j;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			dz[i][j] = (b[i][j] >> 5) & 1;
}

void clear_deadlock_zone_inplace(board b)
{
	int i, j;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			b[i][j] &= ~DEADLOCK_ZONE;
}

void clear_deadlock_zone(board b_in, board b_out)
{
	int i, j;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			b_out[i][j] = b_in[i][j] & ~DEADLOCK_ZONE;
}


int work_left_in_deadlock_zone(board b)
{
	int i, j;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if ((b[i][j] & DEADLOCK_ZONE) == 0) continue;
			if (b[i][j] & SOKOBAN) continue; // probably a diagonal added to the zone

			if ((b[i][j] & BOX) && ((b[i][j] & TARGET) == 0)) return 1;
			if ((b[i][j] & TARGET) && ((b[i][j] & BOX) == 0)) return 1;

		}
	return 0;
}

int work_left_outside_deadlock_zone(board b)
{
	// this should be called only in pull mode
	// make sure boxes match bases.
	int i, j;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if (b[i][j] & DEADLOCK_ZONE) continue;

			if (b[i][j] & BOX) 
				if ((initial_board[i][j] & BOX) == 0)
					return 1;

			if (initial_board[i][j] & BOX)
				if ((b[i][j] & BOX) == 0)
					return 1;
		}
	return 0;
}

void get_sokoban_cloud_as_group(board b, board g)
{
	int i, j;
	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			g[i][j] = (b[i][j] & SOKOBAN ? 1 : 0);
}


int can_be_pushed_sideways(board b, int y, int x, int d)
{
	// if the box is free to move twice, assume that it is not part of the corral

	d = (d + 1) & 3;

	if ((b[y+delta_y[d]][x+delta_x[d]] & OCCUPIED) == 0)
		if ((b[y+2*delta_y[d]][x+2*delta_x[d]] & OCCUPIED) == 0)
			if ((b[y - delta_y[d]][x - delta_x[d]] & OCCUPIED) == 0)
				return 1;

	d = (d + 2) & 3;

	if ((b[y + delta_y[d]][x + delta_x[d]] & OCCUPIED) == 0)
		if ((b[y + 2 * delta_y[d]][x + 2 * delta_x[d]] & OCCUPIED) == 0)
			if ((b[y - delta_y[d]][x - delta_x[d]] & OCCUPIED) == 0)
				return 1;

	return 0;
}

void update_deadlock_zone_from_group_push_mode(board b, board group)
{
	int i, j, k;
	int y, x;
	board prev_deadlock_zone;

	get_deadlock_zone_indicator(b, prev_deadlock_zone);

	clear_deadlock_zone_inplace(b);

	for (i = 0; i < index_num; i++)
	{
		index_to_y_x(i, &y, &x);

		if (group[y][x] == 0) continue;

		b[y][x] |= DEADLOCK_ZONE;

		// add squares that touch the zone
		for (k = 0; k < 8; k++)
		{
			if ((b[y + delta_y_8[k]][x + delta_x_8[k]]) != WALL)
				 b[y + delta_y_8[k]][x + delta_x_8[k]] |= DEADLOCK_ZONE;
		}
		// we also consider diagonals, because if a box gets there it should become
		// a part of the deadlock zone. 

		// add boxes that block access to the zone
		for (k = 0; k < 4; k++)
		{
			if ((b[y + delta_y[k]    ][x + delta_x[k]]) & BOX) 
			if ((b[y + delta_y[k] * 2][x + delta_x[k] * 2]) & BOX) 
			if (can_be_pushed_sideways(b, y + delta_y[k] * 2, x + delta_x[k] * 2, k) == 0)
				b[y + delta_y[k] * 2][x + delta_x[k] * 2] |= DEADLOCK_ZONE;
		}
	}

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if (prev_deadlock_zone[i][j] == 0)
				b[i][j] &= ~DEADLOCK_ZONE;
		}
}


void add_islands(board b)
{
	int i, j, k;
	board dz;

	get_deadlock_zone_indicator(b, dz);

	for (i = 0 ; i < height ; i++)
		for (j = 0; j < width; j++)
		{
			if (inner[i][j] == 0) continue;
			if (b[i][j] & OCCUPIED) continue;
			if (dz[i][j]) continue;

			// check if touches DZ
			for (k = 0; k < 4; k++)
				if (dz[i + delta_y[k]][j + delta_x[k]])
					break;

			if (k < 4) // check surrounded
			{
				if (b[i + 1][j] & OCCUPIED)
				if (b[i - 1][j] & OCCUPIED)
				if (b[i][j + 1] & OCCUPIED)
				if (b[i][j - 1] & OCCUPIED)
				{ 
					b[i][j] |= DEADLOCK_ZONE;
					for (k = 0; k < 8; k++)
						if (b[i + delta_y_8[k]][j + delta_x_8[k]] & BOX)
							b[i + delta_y_8[k]][j + delta_x_8[k]] |= DEADLOCK_ZONE;
				}
			}
		}
}

void init_deadlock_zone_from_group(board b, board group, int pull_mode)
{
	int i, j, k;

	if (pull_mode)
	{
		clear_deadlock_zone_inplace(b);

		for (i = 0; i < height; i++)
			for (j = 0; j < width; j++)
			{
				if (group[i][j] == 0) continue;

				b[i][j] |= DEADLOCK_ZONE;
				for (k = 0; k < 8; k++)
					if (b[i + delta_y_8[k]][j + delta_x_8[k]] & BOX)
						b[i + delta_y_8[k]][j + delta_x_8[k]] |= DEADLOCK_ZONE;
			}

		add_islands(b);

		return;
	}


	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			b[i][j] |= DEADLOCK_ZONE;

	update_deadlock_zone_from_group_push_mode(b, group);
}


void set_deadlock_zone_in_pull_mode(board b)
{
	board g;

	get_sokoban_cloud_as_group(b, g);
	init_deadlock_zone_from_group(b, g, 1);
}


void update_deadlock_zone_push_mode(board b)
{
	int i, j;
	board unreachable;

	zero_board(unreachable);

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if (b[i][j] & SOKOBAN) continue;
			if (inner[i][j] == 0) continue;
			if (b[i][j] & OCCUPIED) continue;

			if ((b[i][j] & DEADLOCK_ZONE) == 0) continue; // new DZ must be inside older DZ

			unreachable[i][j] = 1;
		}

	update_deadlock_zone_from_group_push_mode(b, unreachable);
}


int near_deadlock_zone(board b, int y, int x)
{
	int i;

	for (i = 0; i < 8; i++)
		if (b[y + delta_y_8[i]][x + delta_x_8[i]] & DEADLOCK_ZONE)
			return 1;
	return 0;
}

void clear_frozen_boxes_away_from_deadlock_zone(board b)
{
	// suppose that we investigate a corral. But there are several ways of freezing boxes (on targets) away
	// from it. They all should have the same effect on the deadlock search.

	int i, j;
	int has_frozen = 0;
	board b_in;
	int changed = 0;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if (inner[i][j] == 0) continue;

			if (b[i][j] == WALL)
				if (initial_board[i][j] != WALL)
					has_frozen = 1;
		}

	if (has_frozen == 0) return;

	copy_board(b, b_in);

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if (inner[i][j] == 0) continue;

			if (b[i][j] == WALL)
				if (initial_board[i][j] != WALL)
					if (near_deadlock_zone(b, i, j) == 0)
					{
						b[i][j] = initial_board[i][j] & TARGET; // holes can be wallified
						changed = 1;
					}
		}

	expand_sokoban_cloud(b);
	/*
	if (changed)
	{
		printf("CHANGED:\n");
		print_board(b_in);
		print_board(b);
		my_getch();
	}
	*/
}

void clear_boxes_out_of_deadlock_zone(board b)
{
	int i, j;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if ((b[i][j] & DEADLOCK_ZONE) == 0)
				b[i][j] &= ~BOX;

	expand_sokoban_cloud(b);
}


int exited_deadlock_zone(board b)
{
	int i, j;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (b[i][j] & SOKOBAN)
				if ((b[i][j] & DEADLOCK_ZONE) == 0)
					return 1;
	return 0;
}

void update_deadlock_zone(board b, int pull_mode)
{
	if (pull_mode)
	{
		set_deadlock_zone_in_pull_mode(b);
	}
	else
		update_deadlock_zone_push_mode(b);

	clear_boxes_out_of_deadlock_zone(b);
}

int size_of_deadlock_zone(board b)
{
	int i, j;
	int sum = 0;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (b[i][j] & DEADLOCK_ZONE)
				sum++;
	return sum;
}