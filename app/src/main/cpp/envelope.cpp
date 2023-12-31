// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include <stdio.h>
#include <time.h>

#include "envelope.h"
#include "bfs.h"
#include "util.h"
#include "moves.h"
#include "mini_search.h"


#define MAX_ENVELOPE_PATTERNS 50

int   new_envelope_patterns_num;
board new_envelope_patterns[MAX_ENVELOPE_PATTERNS];

int size_of_corral(int_board corrals, int corral_index)
{
	int sum = 0;
	int i, j;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (corrals[i][j] == corral_index)
				sum++;
	return sum;
}


void wallify_boxes_using_pattern(board b, int pat_num)
{
	int i, j;
	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if (new_envelope_patterns[pat_num][i][j] & BOX)
				b[i][j] = WALL;
		}
}


int matches_envelope_pattern(board b, int n)
{
	int i, j;

	// check that all pattern boxes are in place
	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (new_envelope_patterns[n][i][j] & BOX)
				if ((b[i][j] & OCCUPIED) == 0) // a wall is as good as box
					return 0;

	// make sure the sokoban is not in the corral
	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if ((new_envelope_patterns[n][i][j] & SOKOBAN) == 0)
				if (b[i][j] & SOKOBAN)
					return 0;

	return 1;
}

void fix_boxes_using_envelopes(board b)
{
	int i;

	for (i = 0; i < new_envelope_patterns_num; i++)
	{
		if (matches_envelope_pattern(b, i) == 0) continue;

		wallify_boxes_using_pattern(b, i);
	}
}




int already_in_envelopes(board b)
{
	int i;
	for (i = 0; i < new_envelope_patterns_num; i++)
		if (matches_envelope_pattern(b, i))
			return 1;
	return 0;
}


void get_envelope_boxes(int_board corrals, int corral_index,
	int *regular_boxes, int *regular_boxes_num,
	int *diag_boxes, int *diag_boxes_num
	)
{
	board b;
	int i, j,k,y,x;

	clear_boxes(initial_board, b);

	// collect regular boxes
	*regular_boxes_num = 0;
	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			if (corrals[i][j] != corral_index) continue;

			for (k = 0; k < 4; k++)
			{
				y = i + delta_y[k];
				x = j + delta_x[k];

				if (b[y][x] & TARGET)
				{
					regular_boxes[*regular_boxes_num] = y_x_to_index(y, x);
					(*regular_boxes_num)++;
					b[y][x] &= ~TARGET;
				}
			}
		}
	}

	// collect diagonal boxes
	*diag_boxes_num = 0;
	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			if (corrals[i][j] != corral_index) continue;

			for (k = 0; k < 8; k++)
			{
				y = i + delta_y_8[k];
				x = j + delta_x_8[k];

				if (b[y][x] & TARGET)
				{
					diag_boxes[*diag_boxes_num] = y_x_to_index(y, x);
					(*diag_boxes_num)++;
					b[y][x] &= ~TARGET;
				}
			}
		}
	}
}

int simple_popcnt(int a)
{
	int i;
	int sum = 0;
	for (i = 0; i < 32; i++)
	{
		sum += a & 1;
		a >>= 1;
	}
	return sum;
}

void set_up_corral(int *regular_boxes, int regular_boxes_num,
	int *diag_boxes, int diag_boxes_num,
	int diag_indices, board b)
{
	int i, y, x;

	clear_boxes(initial_board, b);
	clear_sokoban_inplace(b);

	for (i = 0; i < regular_boxes_num; i++)
	{
		index_to_y_x(regular_boxes[i], &y, &x);
		b[y][x] |= BOX;
	}

	for (i = 0; i < 32; i++)
	{
		if ((diag_indices >> i) & 1)
		{
			index_to_y_x(diag_boxes[i], &y, &x);
			b[y][x] |= BOX;
		}
	}
}

void put_sokoban_out_of_corral(board b, int_board corrals, int corral_index)
{
	int i, j;

	clear_sokoban_inplace(b);

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if (inner[i][j] == 0) continue;
			if (corrals[i][j] == corral_index) continue;
			if (b[i][j] & OCCUPIED) continue;
			b[i][j] |= SOKOBAN;
		}
}


int can_solve_from_any_zone(int_board corrals, int corral_index, board b_in)
{
	int corral_zone = -1;
	int_board zones;
	int zones_num;
	int i, j,z;
	board b;

	copy_board(b_in, b);

	zones_num = mark_connectivities(b, zones);

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (corrals[i][j] == corral_index)
				corral_zone = zones[i][j];

	if (corral_zone == -1) exit_with_error("no corral zone");

	for (z = 0; z < zones_num; z++)
	{
		if (z == corral_zone) continue;

		put_sokoban_in_zone(b, zones, z);

		// note that this is a little recursive. 
		// The move generator uses "turn_fixed_boxes_into_walls" which uses the envelopes
		// and computing the envelopes uses find_box_moves...
		// But using patterns that were found so far can't hurt the move generator.


		if (can_put_all_on_targets(b, 100)) return 1; 
		// note: -1 is unknown result - so maybe boxes can be put on targets
	}

	return 0;
}

int should_abort_envelopes()
{
	int t;
	t = (int)time(0) - start_time;
	if (t > (time_limit / 2))
	{
		if (verbose >= 4)
			printf("aborted envelopes\n");
		return 1;
	}
	return 0;
}

void analyse_envelope(int_board corrals, int corral_index,
	int *regular_boxes, int regular_boxes_num,
	int *diag_boxes, int diag_boxes_num)
{
	int i, enum_diag;
	board b;

	if (diag_boxes_num >= 8) return; // limit enumeration size

	for (enum_diag = 0; enum_diag <= diag_boxes_num; enum_diag++)
	{ // how many diagonal boxes are in the envelope

		for (i = 0; i < (1 << diag_boxes_num); i++)
		{
			if (should_abort_envelopes()) return;

			if (simple_popcnt(i) != enum_diag) continue;

			set_up_corral(regular_boxes, regular_boxes_num,	diag_boxes, diag_boxes_num, i, b);
			put_sokoban_out_of_corral(b, corrals, corral_index);

//			printf("checking:\n");
//			print_board(b);

			if (boxes_in_level(b) == global_boxes_in_level) continue; // position is solved

			if (already_in_envelopes(b))
				continue;

			if (can_solve_from_any_zone(corrals, corral_index, b))
				continue;


//			printf("adding envelope!\n");
			copy_board(b, new_envelope_patterns[new_envelope_patterns_num]);
			new_envelope_patterns_num++;

			if (new_envelope_patterns_num >= MAX_ENVELOPE_PATTERNS)
			{
				if (verbose >= 4)
					exit_with_error("Too many envelopes!");
				else
					new_envelope_patterns_num--;
			}

//				my_getch();

		}
	}
}


void init_envelope_patterns()
{
	int i, n;
	int_board corrals;
	board b;

	int regular_boxes[MAX_INNER];
	int diag_boxes[MAX_INNER];
	int regular_boxes_num, diag_boxes_num;

	new_envelope_patterns_num = 0;

	copy_board(initial_board, b);
	enter_reverse_mode(b, 0); // 0 : don't mark bases

	n = mark_connectivities(b, corrals);

//	print_board_with_zones(b, corrals);

	if (n == 1) return;

	for (i = 0; i < n; i++)
	{
		get_envelope_boxes(corrals, i,
							regular_boxes, &regular_boxes_num, diag_boxes, &diag_boxes_num);

		if (size_of_corral(corrals, i) > (index_num / 2)) continue;

		analyse_envelope(corrals, i,
			regular_boxes, regular_boxes_num, diag_boxes, diag_boxes_num);
	}

}