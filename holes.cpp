#include "holes.h"
#include "util.h"

board target_holes;
board semi_holes;

int is_square_a_target_hole(board b, int y, int x)
{
	int i;
	int sum = 0;

	if ((b[y][x] & TARGET) == 0) return 0;

	for (i = 0; i < 4; i++)
		if (b[y + delta_y[i]][x + delta_x[i]] == WALL)
			sum++;

	return (sum == 3 ? 1 : 0);
}

void mark_semi_holes(board b)
{
	zero_board(semi_holes);

	int i, j, k, val1, val2;

	for (i = 0 ; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if (b[i][j] & TARGET)
			{
				for (k = 0; k < 4; k++)
				{
					val1 = b[i + delta_y[k]][j + delta_x[k]];
					val2 = b[i + delta_y[(k+1)&3]][j + delta_x[(k+1)&3]];

					if ((val1 == WALL) && (val2 == WALL))
						semi_holes[i][j] = 1;
				}
			}
		}

//	show_on_initial_board(semi_holes);
//	my_getch();
}


void mark_target_holes(board b_in)
{
	board b;
	int changed = 1;
	int i, j;

	copy_board(b_in, b);
	clear_boxes_inplace(b);
	clear_sokoban_inplace(b);

	zero_board(target_holes);

	while (changed)
	{
		changed = 0;

		for (i = 0; i < height; i++)
		{
			for (j = 0; j < width; j++)
			{
				if (is_square_a_target_hole(b, i, j))
				{
					b[i][j] = WALL;
					changed = 1;
					target_holes[i][j] = 1;
				}
			}
		}
	}

//	show_on_initial_board(target_holes);
//	my_getch();

	mark_semi_holes(b);
}


void turn_target_holes_into_walls(board b)
{
	int i, j;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (target_holes[i][j])
				b[i][j] = WALL;
}


/*
An example where a box must be pulled from hole before all other boxes are on bases.
(when they are on bases, they obstruct the move)

#######
#     #
#  # @#
## #$$#
#.   .#
#######

*/
