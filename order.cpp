#include <stdio.h>
#include <stdlib.h>

#include "order.h"
#include "moves.h"

int order_level[MAX_BOXES][MAX_BOXES];
int order_level_size[MAX_BOXES];
int order_levels_num;

void reverse_order()
{
	int tmp_level[MAX_BOXES];
	int tmp_size;
	int i,j;

	for (i = 0; i < order_levels_num / 2; i++)
	{
		tmp_size = order_level_size[i];
		for (j = 0; j < tmp_size; j++)
			tmp_level[j] = order_level[i][j];

		order_level_size[i] = order_level_size[order_levels_num - 1 - i]; 

		for (j = 0; j < order_level_size[i]; j++)
			order_level[i][j] = order_level[order_levels_num - 1 - i][j];

		order_level_size[order_levels_num - 1 - i] = tmp_size;
		for (j = 0; j < tmp_size; j++)
			order_level[order_levels_num - 1 - i][j] = tmp_level[j];
	}

}


void show_order_levels(board b_in)
{
	board b;

	int i, current, y, x;

	for (current = 0; current < order_levels_num; current++)
	{
		copy_board(b_in, b);
		clear_boxes_inplace(b);

		for (i = 0; i < order_level_size[current]; i++)
		{
			index_to_y_x(order_level[current][i], &y, &x);
			b[y][x] |= BOX;
		}

		printf("order level %d\n", current);
		print_board(b);
	}	
	printf("\n");
}

int get_next_group_to_peel(board b_in, int *group, int pull_mode)
{
	board b;
	int i,corral,moves_num;
	helper h;
	int from_y, from_x, to_y, to_x;
	int n = 0;
	move moves[MAX_MOVES];

	init_helper(&h);
	copy_board(b_in, b);

	moves_num = find_possible_moves(b, moves, pull_mode, &corral, NORMAL, &h);

	for (i = 0; i < moves_num; i++)
	{
		index_to_y_x(moves[i].from, &from_y, &from_x);
		index_to_y_x(moves[i].to, &to_y, &to_x);

		if ((b[from_y][from_x] & BOX) == 0) continue; // already removed

		b[from_y][from_x] &= ~BOX;
		group[n++] = moves[i].from;
	}
	return n;
}

int collect_remaining_boxes(board b)
{
	int current = order_levels_num;
	int i, j;
	int n = 0;

	for (i = 0 ; i < height ; i++)
		for (j = 0 ; j < width ; j++)
			if (b[i][j] & BOX)
				order_level[current][n++] = y_x_to_index(i, j);
	return n;
}

void set_order(board b_in, int pull_mode)
{
	board b;
	int i, j, n, current;

	copy_board(b_in, b);

	order_levels_num = 0;

	print_board(b);

	while (boxes_in_level(b))
	{
		n = 0;
		current = order_levels_num;

		n = get_next_group_to_peel(b, order_level[current], pull_mode);

		if (n == 0) // boxes that can't be pulled
			n = collect_remaining_boxes(b);

		order_level_size[current] = n;

		for (n = 0; n < order_level_size[current]; n++)
		{
			index_to_y_x(order_level[current][n], &i, &j);
			b[i][j] &= ~BOX;
		}
		expand_sokoban_cloud(b);

		order_levels_num++;
	}

	reverse_order();

//	for (i = 0; i < order_levels_num; i++)
//		printf("%d %d\n", i, order_level_size[i]);

//	show_order_levels(b_in);
//	exit(0);
}

int score_using_order(board b)
{
	int i, current, y, x, missing = 0;
	int sum = 0;

	for (current = 0; current < order_levels_num; current++)
	{
		for (i = 0; i < order_level_size[current]; i++)
		{
			index_to_y_x(order_level[current][i], &y, &x);

			if (b[y][x] & BOX)
				sum++;
			else
				missing = 1;
		}

		if (missing) break;
	}

	return sum;
}