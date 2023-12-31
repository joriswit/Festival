// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include "fixed_boxes.h"
#include "graph.h"
#include "hungarian.h"
#include "distance.h"
#include "bfs.h"
#include "util.h"
#include "packing_search.h"
#include "envelope.h"
#include "diags.h"
#include "deadlock.h"
#include "advanced_deadlock.h"

int turn_2x2_to_walls(board b)
{
	// this routine does not guarantee to blockify all boxes in one pass
	int i, j, y, x;
	int sum;
	int changed = 0;

	for (i = 0; i < (height - 1); i++)
	for (j = 0; j < (width - 1); j++)
	{
		sum = 0;

		for (y = 0; y < 2; y++)
		for (x = 0; x < 2; x++)
		if (b[i + y][j + x] & OCCUPIED) sum++;

		if (sum == 4)
		{
			// check if one of them is a box
			for (y = 0; y < 2; y++)
				for (x = 0; x < 2; x++)
				{
					if (b[i + y][j + x] & BOX) 
						changed = 1;
					b[i + y][j + x] = WALL;
				}
		}
	}
	return changed;
}

int close_holes(board b)
{
	int i, j, k;

	for (i = 1; i < (height - 1); i++)
	{
		for (j = 1; j < (width - 1); j++)
		{
			if (b[i][j] == SPACE)
			{
				// close holes of type
				//    $
				//   # #
				//    #

				for (k = 0; k < 4; k++)
				{
					if (b[i + delta_y[(k + 1) & 3]][j + delta_x[(k + 1) & 3]] != WALL) continue;
					if (b[i + delta_y[(k + 2) & 3]][j + delta_x[(k + 2) & 3]] != WALL) continue;
					if (b[i + delta_y[(k + 3) & 3]][j + delta_x[(k + 3) & 3]] != WALL) continue;
					b[i][j] = WALL;
					return 1;
				}
			}

			if (b[i][j] == SOKOBAN)
			{
				// close holes of type
				//    .
				//   #.#
				//    #

				for (k = 0; k < 4; k++)
				{
					if ((b[i + delta_y[(k + 0) & 3]][j + delta_x[(k + 0) & 3]] & SOKOBAN) == 0) continue;
					if (b[i + delta_y[(k + 1) & 3]][j + delta_x[(k + 1) & 3]] != WALL) continue;
					if (b[i + delta_y[(k + 2) & 3]][j + delta_x[(k + 2) & 3]] != WALL) continue;
					if (b[i + delta_y[(k + 3) & 3]][j + delta_x[(k + 3) & 3]] != WALL) continue;
					b[i][j] = WALL;
					return 1;
				}
			}
		}
	}

	return 0;
}


void remove_all_pushable_boxes(board b)
{
	// this routine assumes that the player can be everywhere
	
	int changed = 1;
	int i, j, k;
	int to_y, to_x, from_y, from_x;

	while (changed)
	{
		changed = 0;

		for (i = 0; i < height; i++)
		{
			for (j = 0; j < width; j++)
			{
				if ((b[i][j] & BOX) == 0) continue;

				for (k = 0; k < 4; k++)
				{
					to_y = i + delta_y[k];
					to_x = j + delta_x[k];

					if (b[to_y][to_x] & OCCUPIED) continue;

					from_y = i - delta_y[k];
					from_x = j - delta_x[k];

					if (b[from_y][from_x] & OCCUPIED) continue;

					if (deadlock_in_direction(b, i, j, k)) continue;

					b[i][j] &= ~BOX;
					changed = 1;
					break;
				} // k
			} // j
		} // i
	} // changed

}

int fix_crystaline(board b_in)
{
	// use retrogratde analysis to find boxes that can't be moved until the end
	// (because from the end they can't be pulled)

	board b;
	int i, j, k;
	int removed_box;
	int fixed_box = 0;

	copy_board(b_in, b);
	clear_sokoban_inplace(b);
	go_to_final_position_with_given_walls(b);

	do
	{
		removed_box = 0;
		for (i = 0; i < height; i++)
		{
			for (j = 0; j < width; j++)
			{
				if (b[i][j] != PACKED_BOX) continue;

				for (k = 0; k < 4; k++)
				{
					if (b[i +     delta_y[k]][j +     delta_x[k]] & OCCUPIED) continue;
					if (b[i + 2 * delta_y[k]][j + 2 * delta_x[k]] & OCCUPIED) continue;

					b[i][j] &= ~BOX;
					removed_box = 1;
					break;
				}
			}
		}
	}
	while (removed_box);

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (b[i][j] & BOX)
				if (b_in[i][j] & BOX)
					fixed_box = 1;

	if (fixed_box)
	{
		for (i = 0; i < height; i++)
			for (j = 0; j < width; j++)
				if (b[i][j] & BOX)
					if (b_in[i][j] & BOX)
						b_in[i][j] = WALL;
	}

	return fixed_box;
}


int turn_immovable_boxes_to_walls(board b)
{
	board remaining;
	int i, j;
	int changed = 0;

	copy_board(b, remaining);
	remove_all_pushable_boxes(remaining);

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (remaining[i][j] & BOX)
			{
				b[i][j] = WALL;
				changed = 1;
			}
	return changed;
}


int fix_diagonals(board w)
{
	board diag_right, diag_left;
	int i, j;
	int changed = 0;

	mark_diags(w, diag_right, diag_left);

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if (diag_right[i][j])
				if (diag_has_no_targets(w, i, j, diag_right[i][j], 1))
					changed |= wallify_diag(w, i, j, diag_right[i][j], 1);

			if (diag_left[i][j])
				if (diag_has_no_targets(w, i, j, diag_left[i][j], 0))
					changed |= wallify_diag(w, i, j, diag_left[i][j], 0);
		}
	return changed;
}

void turn_fixed_boxes_into_walls(board b, board w)
{
	int changed = 1;

	copy_board(b, w);	
	fix_boxes_using_envelopes(w);

	do
	{
		changed = turn_2x2_to_walls(w);
		changed |= close_holes(w);
		changed |= turn_immovable_boxes_to_walls(w);
		changed |= fix_diagonals(w);
		changed |= fix_crystaline(w);
	} 
	while (changed);
}




int is_box_2x2_frozen_at_index(board b_in, int index)
{
	int y, x;
	int a, b;

	index_to_y_x(index, &y, &x);
	if ((b_in[y][x] & BOX) == 0) exit_with_error("missing box");

	for (a = y - 1; a <= y; a++)
	{
		for (b = x - 1; b <= x; b++)
		{
			if (
				(b_in[a + 0][b + 0] & OCCUPIED) &&
				(b_in[a + 0][b + 1] & OCCUPIED) &&
				(b_in[a + 1][b + 0] & OCCUPIED) &&
				(b_in[a + 1][b + 1] & OCCUPIED))
				return 1;
		}
	}
	return 0;
}

int is_box_2x2_frozen_after_move(board b, move *m)
{
	int from_y, from_x, to_y, to_x;
	int res;

	index_to_y_x(m->from, &from_y, &from_x);
	index_to_y_x(m->to, &to_y, &to_x);

	if ((b[from_y][from_x] & BOX) == 0) exit_with_error("missing box");

	b[from_y][from_x] &= ~BOX;
	b[to_y][to_x] |= BOX;

	res = is_box_2x2_frozen_at_index(b, m->to);

	b[to_y][to_x] &= ~BOX;
	b[from_y][from_x] |= BOX;

	return res;
}
