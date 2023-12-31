// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include <stdio.h>

#include "bfs.h"
#include "util.h"

void init_dist(int_board dist)
{
	int i, j;
	for (i = 0; i < height; i++)
	for (j = 0; j < width; j++)
		dist[i][j] = 1000000;
}

void bfs_from_place(board b, int_board dist, int start_y, int start_x,
		int ignore_boxes)
{
	// do bfs, assuming "dist" is initialized

	int current_x[MAX_SIZE * MAX_SIZE];
	int current_y[MAX_SIZE * MAX_SIZE];
	int next_x[MAX_SIZE * MAX_SIZE];
	int next_y[MAX_SIZE * MAX_SIZE];

	int current_size, next_size;
	int i, j, y, x, d;

	int occupied_mask = OCCUPIED;

	if (ignore_boxes)
		occupied_mask = WALL;

	current_x[0] = start_x;
	current_y[0] = start_y;
	current_size = 1;

	d = 0;
	dist[start_y][start_x] = d;

	do
	{
		next_size = 0;
		for (i = 0; i < current_size; i++)
		{
			for (j = 0; j < 4; j++)
			{
				x = current_x[i] + delta_x[j];
				y = current_y[i] + delta_y[j];

				if (((b[y][x] & occupied_mask) == 0) && (dist[y][x] == 1000000))
				{
					next_x[next_size] = x;
					next_y[next_size] = y;
					dist[y][x] = d + 1;
					next_size++;
				}
			}
		} // i

		d++;

		for (i = 0; i < next_size; i++)
		{
			current_x[i] = next_x[i];
			current_y[i] = next_y[i];
		}
		current_size = next_size;

	} while (current_size > 0);
}


void bfs(board b, int_board dist, int start_y, int start_x,
	int ignore_boxes)
{
	init_dist(dist);

	bfs_from_place(b, dist,  start_y,  start_x, ignore_boxes);
}
 
int get_connectivity(board b)
{
	int_board dist;
	int c = 0;
	int i, j;

	init_dist(dist);

	for (i = 0; i < height; i++)
	for (j = 0; j < width; j++)
	{
		if (inner[i][j] == 0) continue;
		if (dist[i][j] != 1000000) continue;
		if (b[i][j] & OCCUPIED) continue;

		bfs_from_place(b, dist, i, j, 0);
		c++;
	}

	return c;
}

void print_connectivity_comps(int_board dist)
{
	int i, j;

	for (i = 0; i < height; i++)
	{

		for (j = 0; j < width; j++)
		{
			if (dist[i][j] == 1000000)
				printf(" ");
			else
				printf("%d", dist[i][j] % 10);
		}
		printf("\n");
	}
}


int mark_connectivities(board b, int_board dist)
{
	int_board local_dist;
	int c = 0;
	int y, x, i, j;

	init_dist(dist);

	for (i = 0; i < height; i++)
	for (j = 0; j < width; j++)
	{
		if (inner[i][j] == 0) continue;
		if (dist[i][j] != 1000000) continue;
		if (b[i][j] & OCCUPIED) continue;

		init_dist(local_dist);
		bfs_from_place(b, local_dist, i, j, 0);

		for (y = 0; y < height; y++)
		for (x = 0;  x < width; x++)
		if (local_dist[y][x] < 1000000)
			dist[y][x] = c;

		c++;
	}
	return c;
}

void mark_connectivities_around_place(board b, int_board dist,
									 int start_y, int start_x)
{
	int_board local_dist;
	int c, i, j, y, x;

	init_dist(dist);

	for (c = 0; c < 4 ; c++)
	{
		i = start_y + delta_y[c];
		j = start_x + delta_x[c];

		if (inner[i][j] == 0) continue;
		if (dist[i][j] != 1000000) continue;
		if (b[i][j] & OCCUPIED) continue;

		init_dist(local_dist);
		bfs_from_place(b, local_dist, i, j, 0);

		for (y = 0; y < height; y++)
			for (x = 0; x < width; x++)
				if (local_dist[y][x] < 1000000)
					dist[y][x] = c;
	}
}

void get_connectivity_sizes(int_board dist, int n, int *sizes)
{
	int i, j;

	for (i = 0; i < n; i++)
		sizes[i] = 0;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (dist[i][j] != 1000000)
				sizes[dist[i][j]]++;
}

void get_srcs_of_all_targets(board b_in, board srcs)
{
	// the board configuration may change after freezing boxes.
	// find updated sources of all targets.

	int queue[MAX_INNER];
	int queue_len = 0;
	int queue_place = 0;
	int i, j, k, box_y, box_x;
	board b;

	zero_board(srcs);
	clear_boxes(b_in, b);

	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			if (inner[i][j] == 0) continue;
			if ((b[i][j] & TARGET) == 0) continue;
			queue[queue_len++] = y_x_to_index(i, j);
			srcs[i][j] = 1;
		}
	}

	while (queue_place < queue_len)
	{
		index_to_y_x(queue[queue_place++], &i, &j);

		for (k = 0; k < 4; k++)
		{
			box_y = i + delta_y[k];
			box_x = j + delta_x[k];

			if (b[box_y][box_x] & OCCUPIED) continue;
			if (b[box_y + delta_y[k]][box_x + delta_x[k]] & OCCUPIED) continue;
			if (srcs[box_y][box_x]) continue;

			srcs[box_y][box_x] = 1;
			queue[queue_len++] = y_x_to_index(box_y, box_x);
		}
	}
}

void expand_group(board b, board group, int_board dist)
{
	int current[MAX_INNER];
	int current_num = 0;
	int next[MAX_INNER];
	int next_num;
	int i, j, y, x, next_y, next_x;


	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			dist[i][j] = 1000000;

			if (group[i][j])
			{
				current[current_num++] = y_x_to_index(i, j);
				dist[i][j] = 0;
			}
		}

	while (current_num > 0) 
	{
		next_num = 0;

		for (i = 0; i < current_num; i++)
		{
			index_to_y_x(current[i], &y, &x);

			for (j = 0; j < 4; j++)
			{
				next_y = y + delta_y[j];
				next_x = x + delta_x[j];

				if (group[next_y][next_x]) continue;
				if (b[next_y][next_x] & OCCUPIED) continue;

				group[next_y][next_x] = 1;
				next[next_num++] = y_x_to_index(next_y, next_x);
				dist[next_y][next_x] = dist[y][x] + 1;
			}
		}

		for (i = 0; i < next_num; i++)
			current[i] = next[i];
		current_num = next_num;
	}
}

