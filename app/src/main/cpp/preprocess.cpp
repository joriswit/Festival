// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include <stdio.h>

#include "preprocess.h"

int player_can_leave_board(board b_in)
{
	int queue_y[MAX_SIZE*MAX_SIZE];
	int queue_x[MAX_SIZE*MAX_SIZE];
	board b;
	int i, j, y, x, next_y, next_x;
	int queue_len = 0;
	int queue_pos = 0;

	copy_board(b_in, b);
	clear_boxes_inplace(b);

	// init queue from existing sokoban positions
	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (b[i][j] & SOKOBAN)
			{
				queue_y[queue_len] = i;
				queue_x[queue_len] = j;
				queue_len++;
			}

	while (queue_pos < queue_len)
	{
		y = queue_y[queue_pos];
		x = queue_x[queue_pos];

		for (i = 0; i < 4; i++)
		{
			next_y = y + delta_y[i];
			next_x = x + delta_x[i];

			if (next_y < 0) return 1;
			if (next_x < 0) return 1;
			if (next_y >= height) return 1;
			if (next_x >= width) return 1;

			if (b[next_y][next_x] & SOKOBAN) continue;
			if (b[next_y][next_x] & OCCUPIED) continue;

			b[next_y][next_x] |= SOKOBAN;
			queue_y[queue_len] = next_y;
			queue_x[queue_len] = next_x;
			queue_len++;
		}
		queue_pos++;
	}
	return 0;
}

int sanity_checks(board b)
{
	int box_num;

	if (player_can_leave_board(b))
	{
		printf("Invalid board\n");
		sprintf(global_fail_reason, "Invalid board");
		return 0;
	}

	box_num = boxes_in_level(b);

	if (box_num >= MAX_BOXES)
	{
		printf("Too many boxes\n");
		sprintf(global_fail_reason, "Too many boxes");
		return 0;
	}
	return 1;
}

void close_holes_in_board(board b)
{
	int changed = 0;
	int i, j, k, sum;

	do
	{
		changed = 0;

		for (i = 1; i < (height - 1); i++)
			for (j = 1; j < (width - 1); j++)
			{
				if (b[i][j] != 0) continue;

				sum = 0;
				for (k = 0; k < 4; k++)
					if (b[i + delta_y[k]][j + delta_x[k]] == WALL)
						sum++;
				if (sum >= 3)
				{
					b[i][j] = WALL;
					changed = 1;
				}
			}
	} while (changed);

}


void remove_boxes_out_of_inner(board b)
{
	board inner;
	int i, j;

	// mark inner and boxes in inner
	clear_boxes(b, inner);
	expand_sokoban_cloud(inner);

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if ((inner[i][j] & SOKOBAN) == 0)
			{
				b[i][j] &= ~BOX;
				b[i][j] &= ~TARGET;
			}
}

void turn_decorative_boxes_to_walls(board b)
{
	board c;
	int i, j, k;
	int changed = 1;

	remove_boxes_out_of_inner(b);

	clear_boxes(b, c);

	// eliminate pullable boxes

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (c[i][j] & TARGET)
				c[i][j] = BOX;

	while (changed)
	{
		changed = 0;

		for (i = 0; i < height; i++)
			for (j = 0; j < width; j++)
			{
				if ((c[i][j] & BOX) == 0) continue;
				for (k = 0; k < 4; k++)
				{
					if (c[i + delta_y[k]][j + delta_x[k]] & OCCUPIED) continue;
					if (c[i + delta_y[k] * 2][j + delta_x[k] * 2] & OCCUPIED) continue;

					c[i][j] &= ~BOX;
					changed = 1;
				}
			}
	}

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (c[i][j] & BOX)
				b[i][j] = WALL;



	// eliminate pushable boxes
	copy_board(b, c);
	changed = 1;

	while (changed)
	{
		changed = 0;

		for (i = 0; i < height; i++)
			for (j = 0; j < width; j++)
			{
				if ((c[i][j] & BOX) == 0) continue;
				for (k = 0; k < 4; k++)
				{
					if (c[i + delta_y[k]][j + delta_x[k]] & OCCUPIED) continue;
					if (c[i - delta_y[k]][j - delta_x[k]] & OCCUPIED) continue;

					c[i][j] &= ~BOX;
					changed = 1;
				}
			}
	}

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (c[i][j] & BOX)
				b[i][j] = WALL;
}
