// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include <stdio.h>
#include <stdlib.h>

#include "graph.h"
#include "distance.h"
#include "bfs.h"
#include "util.h"

int distance_from_to[MAX_INNER][MAX_INNER];
int bfs_distance_from_to[MAX_INNER][MAX_INNER];

int impossible_place[MAX_INNER];

void compute_can_get_to_dest(board b, int can_get_to_dest[MAX_INNER])
{
	int i, x, y, place;

	for (i = 0; i < index_num; i++)
	{
		can_get_to_dest[i] = 0;

		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				if (inner[y][x] == 0) continue;

				if (b[y][x] & TARGET)
				{
					place = y_x_to_index(y, x);
					if (distance_from_to[i][place] < 1000000)
						can_get_to_dest[i] = 1;
				}
			}
		}
	}


	for (i = 0; i < index_num; i++)
	if (can_get_to_dest[i] == 0)
	{
		index_to_y_x(i, &y, &x);
//		printf("%d %d can't reach target\n", y, x);
	}
}


void compute_can_from_start(board b, int can_get_from_start[MAX_INNER])
{
	int i, x, y, place;

	for (i = 0; i < index_num; i++)
	{
		can_get_from_start[i] = 0;

		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				if (inner[y][x] == 0) continue;

				if (b[y][x] & BOX)
				{
					place = y_x_to_index(y, x);
					if (distance_from_to[place][i] < 1000000)
						can_get_from_start[i] = 1;
				}
			}
		}
	}


	for (i = 0; i < index_num; i++)
	if (can_get_from_start[i] == 0)
	{
		index_to_y_x(i, &y, &x);
//		printf("%d %d can't be reached from start\n", y, x);
	}
}

void set_impossible_places(int can_get_to_dest[MAX_INNER], int can_get_from_start[MAX_INNER])
{
	int i;

	for (i = 0; i < MAX_INNER; i++)
		impossible_place[i] = 1;

	for (i = 0; i < index_num; i++)
	if ((can_get_from_start[i]) && (can_get_to_dest[i]))
		impossible_place[i] = 0;

}



void set_bfs_distances(board b)
{
	int i, j;
	int y1, x1;
	int y2, x2;

	int_board d;

	for (i = 0; i < index_num; i++)
	{
		index_to_y_x(i, &y1, &x1);
		bfs(b, d, y1, x1, 1); // 1 = ignore boxes
		
		for (j = 0; j < index_num; j++)
		{
			index_to_y_x(j, &y2, &x2);

			bfs_distance_from_to[i][j] = d[y2][x2];
		}
	}
}



void set_distances(board b)
{
	board board_without_boxes;
	int can_get_to_dest[MAX_INNER];
	int can_get_from_start[MAX_INNER];

	int from_index, to_index;
	int dist;
	graph_data *gd;

	gd = allocate_graph();

	clear_boxes(b, board_without_boxes);

	build_graph(board_without_boxes, 0, gd); // 0 = push_mode

	for (from_index = 0; from_index < index_num; from_index++)
	{
		set_graph_weights_to_infinity(gd);
		clear_weight_around_cell(from_index, gd);
		do_graph_iterations(0, gd); // 0 = push mode
		
		for (to_index = 0; to_index < index_num; to_index++)
		{
			dist = get_weight_around_cell(to_index, gd);
			distance_from_to[from_index][to_index] = dist;
		}
	}

	compute_can_get_to_dest(b, can_get_to_dest);
	compute_can_from_start(b, can_get_from_start);
	set_impossible_places(can_get_to_dest, can_get_from_start);

	set_bfs_distances(b);

	free(gd);
}


void get_distance_from_targets(board b, int_board dist)
{
	board target_area;
	board board_without_boxes;
	int i, j;

	zero_board(target_area);
	clear_boxes(b, board_without_boxes);

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (b[i][j] & TARGET)
				target_area[i][j] = 1;

	find_distance_from_a_group(board_without_boxes, target_area, dist, 1); // 1 - pull mode
}