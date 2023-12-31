// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include "hotspot.h"
#include "global.h"
#include "graph.h"
#include "util.h"
#include "distance.h"

/*
a hotspot is a box on a place, that blocks another box way to ANY target.
In order to compute if a hotspot is active, we prepare a list of all the sources
that would be blocked. 
*/


int targets_reachable_from[MAX_INNER];
int blocks_place[MAX_INNER][MAX_INNER];
// blocks_place[x][y] = number of targets reachable from x given a fixed y.

int completely_blocks[MAX_INNER][MAX_INNER];


int hotspots_score(board b)
{
	board h, weights;
	
	find_hotspots(b, h, weights);

	return board_popcnt(h);
}

int count_targets_on_mask(board b, board mask)
{
	int sum = 0;
	int i, j;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (mask[i][j])
				if (b[i][j] & TARGET)
					sum++;
	return sum;
}

void clear_hotspots_data()
{
	int i, j;

	for (i = 0; i < index_num; i++)
		for (j = 0; j < index_num; j++)
		{
			blocks_place[i][j] = 0;
			completely_blocks[i][j] = 0;
		}
}

int hotspots_take_too_much_time()
{
	int t;

	t = (int)time(0) - start_time;

	if (t < (time_limit / 4))
		return 0;

	clear_hotspots_data();
	if (verbose >= 4)
		printf("aborted hotspots calculation\n");
	return 1;
}


void init_hotspots(board b)
{
	board empty, targets;
	int p, y, x;
	int updated_targets;
	int blocker, blocker_y, blocker_x;
	board board_with_blocker;
	graph_data *gd;

	gd = allocate_graph();

	clear_hotspots_data();

	clear_boxes(b, empty);

	build_graph(empty, 0, gd);
	for (p = 0; p < index_num; p++)
	{
		index_to_y_x(p, &y, &x);

		find_targets_of_place_without_init_graph(y, x, targets, gd);

		targets_reachable_from[p] = count_targets_on_mask(b, targets);
	}

	for (blocker = 0; blocker < index_num; blocker++)
	{
		if (hotspots_take_too_much_time())
		{
			free(gd);
			return; // abort hotspots calculation if it is too heavy
		}

		if (impossible_place[blocker])
			continue;

		index_to_y_x(blocker, &blocker_y, &blocker_x);

		if (b[blocker_y][blocker_x] & TARGET)
			continue; // if a box reached a target, don't consider it as a blocker 


		copy_board(empty, board_with_blocker);
		board_with_blocker[blocker_y][blocker_x] = WALL;

		build_graph(board_with_blocker, 0, gd);

		for (p = 0; p < index_num; p++)
		{
			if (impossible_place[p])
				continue;

			if (p == blocker) continue;

			index_to_y_x(p, &y, &x);

			if (b[y][x] & TARGET)
				continue; // if the blocked box is on target, don't consider it to be blocked

			find_targets_of_place_without_init_graph(y, x, targets, gd);

			blocks_place[p][blocker] =  targets_reachable_from[p]; // how many targets without the blocker
			updated_targets = count_targets_on_mask(b, targets); // how many targets with the blocker
			blocks_place[p][blocker] -= updated_targets;

			if (updated_targets == 0)
				completely_blocks[p][blocker] = 1;

		}
	}

	free(gd);
}


int double_blocking(int index1, int index2)
{
	if (completely_blocks[index1][index2] == 0) return 0;
	if (completely_blocks[index2][index1] == 0) return 0;
	return 1;
}

void find_hotspots(board b, board h, board hotspot_weights)
{
	// given a board, find the active hotspots

	int i, j, y, x;
	int boxes_indices[MAX_INNER];
	int boxes_num = 0;

	zero_board(h);
	zero_board(hotspot_weights);

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if (inner[i][j] == 0) continue;

			if (b[i][j] & BOX)
				boxes_indices[boxes_num++] = y_x_to_index(i, j);
		}


	for (i = 0; i < boxes_num; i++)
	{
		for (j = 0; j < boxes_num; j++)
		{
			if (j == i)
				continue;

			if (blocks_place[boxes_indices[j]][boxes_indices[i]]) // i is blocking j
//			if (completely_blocks[boxes_indices[j]][boxes_indices[i]]) // i is blocking j
			{
				index_to_y_x(boxes_indices[i], &y, &x);
				h[y][x] = 1;
				hotspot_weights[y][x]++;
			}
		}
	}
}
