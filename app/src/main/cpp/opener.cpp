// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include <stdio.h>

#include "opener.h"
#include "hotspot.h"
#include "distance.h"
#include "util.h"
#include "park_order.h"


int are_new_moves(board b, board c, int pull_mode)
{
	int i, j, k;
	int val;
	int sok_y, sok_x;

	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			// a box should be in (i,j) on both boards
			if ((b[i][j] & BOX) == 0) continue;
			if ((c[i][j] & BOX) == 0) continue;

			for (k = 0; k < 4; k++)
			{
				sok_y = i + delta_y[k];
				sok_x = j + delta_x[k];

				val = b[sok_y][sok_x]; // sokoban not there on old board
				if (val & SOKOBAN) continue;
				if (val & BOX) continue;

				if ((c[sok_y][sok_x] & SOKOBAN) == 0) continue; //  now sokoban is there

				if (pull_mode == 0)
				{
					if ((c[i - delta_y[k]][j - delta_x[k]] & OCCUPIED) == 0) return 1; // can push
				}
				else
				{
					if ((c[sok_y + delta_y[k]][sok_x + delta_x[k]] & OCCUPIED) == 0) return 1; // can pull
				}
			}

		}
	}
	return 0;
}

int explorer(board b, score_element *base_score,
	int moves_num, move *moves, score_element *scores, int pull_mode)
{
	board c;
	board blocked_area;
	int i, y, x,res;	
	score_element best_score;

	int best_move = -1;

	zero_board(blocked_area);
	for (i = 0; i < index_num; i++)
	{
		index_to_y_x(i, &y, &x);
		if ((b[y][x] == 0) || (b[y][x] == TARGET)) // without SOKOABN
			blocked_area[y][x] = 1;
	}

	for (i = 0; i < moves_num; i++)
	{
		if (scores[i].packed_boxes < base_score->packed_boxes) continue;

		if (pull_mode == 0)
		if (distance_from_to[moves[i].to][moves[i].from] == 1000000)
			continue; // reject irreversible moves

		copy_board(b, c);
		apply_move(c, moves + i, NORMAL);

		res = are_new_moves(b, c, pull_mode);

		if (res == 0) continue;
	
		if (best_move == -1)
		{
			best_score = scores[i];
			best_move = i;
			continue;
		}

		if (is_better_score(scores + i, &best_score, pull_mode, NORMAL))
		{
			best_score = scores[i];
			best_move = i;
		}		
	}
	return best_move;
}

int pick_opener(board b, score_element *base_score,
	int moves_num, move *moves, score_element *scores, int pull_mode, helper *h)
{
	int i, blocker_y = 0, blocker_x = 0, blocker_index;
	int connectivity, rooms_score, hotspots, packed_boxes, out_of_plan;
	int dist1, dist2;

	int min_dist_from = 1000000;
	int max_dist_to = 0;
	int best_move = -1;

	for (i = 0; i < moves_num; i++)
		if (scores[i].packed_boxes > base_score->packed_boxes)
			return -1;

	blocker_index = next_parking_place(b, h);
	if (blocker_index == -1)
		return -1;

	index_to_y_x(blocker_index, &blocker_y, &blocker_x);

	if (b[blocker_y][blocker_x] & BOX)
	{
		print_board(b);
		printf("next box: %d %d\n", blocker_y, blocker_x);
		exit_with_error("box in place?");
	}

	//	printf("MAJOR BLOCKER: %d %d\n", blocker_y, blocker_x);

	connectivity = base_score->connectivity;
	rooms_score  = base_score->rooms_score;
	packed_boxes = base_score->packed_boxes;
	out_of_plan  = base_score->out_of_plan;
	hotspots     = base_score->hotspots;

	for (i = 0; i < moves_num; i++)
	{
		if (scores[i].packed_boxes < packed_boxes) continue;
		if (scores[i].connectivity > connectivity) continue;
		if (scores[i].rooms_score  > rooms_score) continue;

		dist1 = bfs_distance_from_to[blocker_index][moves[i].from];
		dist2 = bfs_distance_from_to[blocker_index][moves[i].to];
		if (dist2 < dist1) continue;

		if (dist1 > min_dist_from) continue;

		if (dist1 < min_dist_from)
		{
			min_dist_from = dist1;
			best_move = i;
			max_dist_to = dist2;
			continue;
		}

		if (dist1 == min_dist_from)
		{
			if (dist2 > max_dist_to)
			{
				max_dist_to = dist2;
				best_move = i;
			}
		}
	}

	return best_move;
}
