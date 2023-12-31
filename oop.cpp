#include <stdio.h>

#include "graph.h"
#include "distance.h"
#include "oop.h"
#include "util.h"
#include "helper.h"

void get_parked_boxes(board parked, int step, helper *h)
{
	int i, y, x;
	int init_num = 0;

	park_order_data *parking_order = h->parking_order;

	zero_board(parked);

	// count initials
	for (i = 0 ; i < h->parking_order_num; i++)
		if (parking_order[i].from == -2)
			init_num++;

	if (step < init_num)
		step = init_num; // there's no sense making order in the initials

	for (i = 0; i < step; i++)
	{
		if ((parking_order[i].until == -1) || (parking_order[i].until >= step))
		{
			index_to_y_x(parking_order[i].to, &y, &x);
			parked[y][x] = 1;
		}
	}
}

void get_oop_zone_for_step(int step, board oop, helper *h)
{
	board parked_boxes;
	board next_targets;
	board b, sources;
	int i, j, y, x;
	park_order_data *parking_order = h->parking_order;

	// oop zone for step i is the boxes that should be cleared before the i'th box is 
	// put and immobilized.


	zero_board(parked_boxes);

	for (i = 0; i <= step; i++)
	{
		if (parking_order[i].until == -1)
		{
			index_to_y_x(parking_order[i].to, &y, &x);
			parked_boxes[y][x] = 1;
		}
	}

	zero_board(next_targets);
	for (i = step + 1; i < h->parking_order_num; i++)
	{
		index_to_y_x(parking_order[i].to, &y, &x);
		next_targets[y][x] = 1;
	}

	clear_boxes(initial_board, b);
	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (parked_boxes[i][j])
				b[i][j] = WALL;

	find_sources_of_a_board_input(b, next_targets, sources);

	zero_board(oop);

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if (inner[i][j] == 0) continue;
			if (b[i][j] == WALL) continue;
			if (impossible_place[y_x_to_index(i, j)] == 1) continue;
			if (sources[i][j]) continue;

			oop[i][j] = 1;
		}

	//	printf("oop for stage %d\n", step);
	//	show_on_initial_board(oop);
	//	my_getch();
}

int temp_val = 0;


void set_oop_zones_range(board val, int from, int to, helper *h)
{
	int i;

	if (h->oop == 0) exit_with_error("missing oop");
	board *oop_zone_for_step = h->oop->oop_zone_for_step;
	int_board *oop_zone_distance_for_step = h->oop->oop_zone_distance_for_step;

	for (i = from; i < to; i++)
		copy_board(val, oop_zone_for_step[i]);
}



void get_oop_distance_for_step(int_board dist, int step, helper *h)
{
	board parked, empty_board;
	int i, j;

	if (h->oop == 0) exit_with_error("missing oop1");
	board *oop_zone_for_step = h->oop->oop_zone_for_step;
	int_board *oop_zone_distance_for_step = h->oop->oop_zone_distance_for_step;


	get_parked_boxes(parked, step, h); // 0: initial boxes are optional

	clear_boxes(initial_board, empty_board);
	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (parked[i][j])
				empty_board[i][j] = WALL;

	find_distance_from_a_group(empty_board, oop_zone_for_step[step], dist, 0); // 0 - push mode
}


void prepare_oop_zones(helper *h)
{
	int i;
	int_board dist;

	if (h->oop == 0) exit_with_error("missing oop2");
	board *oop_zone_for_step = h->oop->oop_zone_for_step;
	int_board *oop_zone_distance_for_step = h->oop->oop_zone_distance_for_step;

	for (i = 0; i < (h->parking_order_num - 1); i++)
		get_oop_zone_for_step(i, oop_zone_for_step[i], h);

	// oop_zone_for_step shows the impossible places after putting the box in step i
	// since we want to clear boxes before that happens, we will look at the next change

	board current_board;
	zero_board(current_board);
	int last_modified = 0;

	for (i = 0; i < (h->parking_order_num - 1); i++)
	{
		// we want to clear boxes away from OOP areas as soon as possible.
		// at this point oop_zone_for_step[i] is the last minute

		if (is_same_board(oop_zone_for_step[i], current_board))
			continue;

		copy_board(oop_zone_for_step[i], current_board);
//		get_oop_distance_for_step(dist, i, h);

		if (verbose >= 5)
		{
			printf("modifying board at step %d last=%d\n", i, last_modified);
			show_on_initial_board(current_board);
			show_ints_on_initial_board(dist);
		}

//		set_oop_zones_range(current_board, dist, last_modified, i + 1, h);
		set_oop_zones_range(current_board, last_modified, i + 1, h);

		last_modified = i;
	}

	for (i = 0; i < (h->parking_order_num - 1); i++)
		get_oop_distance_for_step(oop_zone_distance_for_step[i], i, h);

	/*
	for (i = 0; i < (h->parking_order_num - 1); i++)
	{
		printf("oop for step %d:\n", i);
		show_on_initial_board(oop_zone_for_step[i]);
	}
	my_getch();
	*/
}


int count_boxes_in_oop_zone(board b, board oop)
{
	int i, j;
	int sum = 0;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (oop[i][j])
				if (b[i][j] & BOX)
					sum++;

	return sum;
}

int get_out_of_plan_score(board b, int parked_num, helper *h)
{
	int res;

	if (h->oop == 0) exit_with_error("missing oop3");
	board *oop_zone_for_step = h->oop->oop_zone_for_step;
	int_board *oop_zone_distance_for_step = h->oop->oop_zone_distance_for_step;


	res = count_boxes_in_oop_zone(b, oop_zone_for_step[parked_num]);
	return res;
}


int new_oop_advisor(board b, score_element *base_score,
	int moves_num, move *moves, score_element *scores, int pull_mode, helper *h)
{
	int i;
	int from, to, y, x;
	int best_move = -1;
	int min_from = 1000000;
	int max_to = -1;
	int from_dist, to_dist;
	int min_rooms = 1000000;
	int min_connect = 1000000;
	int step = base_score->packed_boxes;

	if (h->oop == 0) exit_with_error("missing oop");
	board *oop_zone_for_step = h->oop->oop_zone_for_step;
	int_board *oop_zone_distance_for_step = h->oop->oop_zone_distance_for_step;


	if (base_score->out_of_plan == 0)
		return -1;

	min_connect = base_score->connectivity;
	min_rooms = base_score->rooms_score;

	for (i = 0; i < moves_num; i++)
	{
		if (scores[i].connectivity > min_connect) continue;
		if (scores[i].rooms_score > min_rooms) continue;
		if (scores[i].out_of_plan > base_score->out_of_plan) continue;
		if (scores[i].packed_boxes < base_score->packed_boxes) continue;

		from = moves[i].from;
		to = moves[i].to;

		// require reversible
		if (distance_from_to[to][from] == 1000000) continue;

		index_to_y_x(from, &y, &x);
		from_dist = oop_zone_distance_for_step[step][y][x];

		index_to_y_x(to, &y, &x);
		to_dist = oop_zone_distance_for_step[step][y][x];

		if (from_dist == 1000000) continue;
		if (to_dist == 1000000) continue;

		if (to_dist <= from_dist)
			continue;

		if (from_dist > min_from) continue;

		if (from_dist < min_from)
		{
			min_from = from_dist;
			max_to = to_dist;
			best_move = i;
		}

		if (from_dist == min_from)
		{
			if (to_dist > max_to)
			{
				max_to = to_dist;
				best_move = i;
			}
		}
	}

	return best_move;
}
