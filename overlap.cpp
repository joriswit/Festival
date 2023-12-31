// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include "overlap.h"
#include "park_order.h"
#include "util.h"
#include "imagine.h"


void save_full_parking_order(helper *h)
{
	int i,y,x;

	for (i = 0; i < h->parking_order_num; i++)
		(h->full_parking_order)[i] = (h->parking_order)[i];
	h->full_parking_order_num = h->parking_order_num;

	// init used_in_plan
	for (i = 0; i < h->parking_order_num; i++)
	{
		index_to_y_x(h->full_parking_order[i].to, &y, &x);
		h->used_in_plan[y][x] = 1;
	}


	init_imagine(h->full_parking_order, h->full_parking_order_num, h);
}


int overlap_score(board b, helper *h)
{
	board overlap_board; // mostly for debug
	int i;
	int from_y, from_x, to_y, to_x;
	int score = 0;
	int best_overlap_score = -1;

	if (h->boxes_were_removed) return 0;

	if (h->full_parking_order_num == 0) return 0;

	zero_board(overlap_board);

	for (i = 0; i < h->full_parking_order_num; i++)
	{
		index_to_y_x((h->full_parking_order)[i].to, &to_y, &to_x);
		overlap_board[to_y][to_x] = 1;
		if (b[to_y][to_x] & BOX)
			score++;

		if ((h->full_parking_order[i]).from >= 0)
		{
			index_to_y_x((h->full_parking_order)[i].from, &from_y, &from_x);
			overlap_board[from_y][from_x] = 0;
			if (b[from_y][from_x] & BOX)
				score--;
		}

		if (score >= best_overlap_score)
			best_overlap_score = score;
	}

	return best_overlap_score;
}
