#include <string.h>
#include <stdio.h>

#include "imagine.h"
#include "util.h"
#include "park_order.h"
#include "packing_search.h"
#include "io.h"
#include "distance.h"
#include "biconnect.h"
#include "hf_search.h"

void init_imagine(park_order_data *full, int n, helper *h)
{
	int i;
	int p = 0;
	board b, fixed;
	int to_y, to_x, from_y, from_x;
	int permanent = 0;

	if (n >= MAX_SOL_LEN) exit_with_error("plan is too long");

	if (h->imagine == 0) exit_with_error("imagine not allocated");

	h->imagine->imagined_boards_num = 0;

	for (i = 0; i < n; i++)
		if (full[i].from == -1) // box from sink 
			return;


	// verify that boxes are not shifted in place
	for (i = 0; i < n; i++)
		if (full[i].from == full[i].to)
		{
			printf("step %d\n", i);
			exit_with_error("shifting a box in place");
		}

	copy_board(initial_board, b);
	clear_boxes_inplace(b);
	clear_sokoban_inplace(b);
	zero_board(fixed);

	for (i = 0; i < n; i++)
	{
		index_to_y_x(full[i].to, &to_y, &to_x);

		if (full[i].until == -1)
		{
			permanent++;
			fixed[to_y][to_x] = 1;
		}

		if ((full[i].until == -1) && (full[i].from != -2)) 			
		{
//			printf("before permanently putting %d boxes, board is:\n", permanent);
//			print_board(b);

			copy_board(b, h->imagine->imagined_boards[h->imagine->imagined_boards_num]);
			copy_board(fixed, h->imagine->fixed_at_step[h->imagine->imagined_boards_num]);
			h->imagine->imagine_packed_num[h->imagine->imagined_boards_num] = permanent;
			h->imagine->imagined_boards_num++;
	//		my_getch();
		}

		index_to_y_x(full[i].to, &to_y, &to_x);
		b[to_y][to_x] |= BOX;

		if (full[i].from >= 0)
		{
			index_to_y_x(full[i].from, &from_y, &from_x);
			b[from_y][from_x] &= ~BOX;
		}
	}

	if (h->imagine->imagined_boards_num == 0)
	{ // for the extra rare case of no plan, i.e. all boxes are just in place

		if (n != global_boxes_in_level)	exit_with_error("corrupted plan");

		copy_board(b, h->imagine->imagined_boards[0]);
		copy_board(fixed, h->imagine->fixed_at_step[0]);
		h->imagine->imagine_packed_num[0] = permanent;
		h->imagine->imagined_boards_num = 1;
	}

}

void get_imagined_board(board b, board imagined, int *relevant_board, helper *h)
{
	int i;
	int packed_boxes;

	if (h->imagine == 0) exit_with_error("imagine not allocated");

	packed_boxes = score_parked_boxes(b, h);

	if (packed_boxes == global_boxes_in_level) // level is solved
	{
		copy_board(b, imagined);
		*relevant_board = h->imagine->imagined_boards_num - 1;
		return;
	}

	for (i = 0; i < h->imagine->imagined_boards_num; i++)
		if (h->imagine->imagine_packed_num[i] > packed_boxes)
			break;

	*relevant_board = i;

	copy_board(h->imagine->imagined_boards[i], imagined);
}



int imagine_score(board b, int search_mode, helper *h)
{
	int relevant_board;
	board imagined;
	int sum = 0;
	int i, j;

	if (h->imagine == 0) exit_with_error("imagine not allocated");

	if ((search_mode == FORWARD_WITH_BASES) || h->boxes_were_removed || (search_mode == GIRL_SEARCH))
		return 0;

	if (search_mode == HF_SEARCH)
		return compare_to_imagined_hf(b, h);

	if (search_mode != NORMAL)
		exit_with_error("why here?\n");

	if (h->parking_order_num != global_boxes_in_level)
		exit_with_error("debug order");

	get_imagined_board(b, imagined, &relevant_board, h);

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (b[i][j] & BOX)
				if ((h->imagine->imagined_boards)[relevant_board][i][j] & BOX)
					if ((h->imagine->fixed_at_step)[relevant_board][i][j] == 0) // don't count packed boxes
						sum++;

	return sum;
}


int choose_imagine_advisor(score_element *base_score, int moves_num, score_element *scores, int search_mode)
{
	int i;
	int best_son = -1;
	int min_dist = 0;

	if ((search_mode == FORWARD_WITH_BASES) || (search_mode == GIRL_SEARCH))
		return -1;

	for (i = 0; i < moves_num; i++)
	{
		if (scores[i].packed_boxes < base_score->packed_boxes) continue;
		if (scores[i].imagine <= base_score->imagine) continue;

		if ((best_son == -1) || (scores[i].dist_to_imagined < min_dist))
		{
			min_dist = scores[i].dist_to_imagined;
			best_son = i;
		}
	}
	return best_son;
}



void set_imagined_hf_board(board b, helper *h)
{
	int i, j;
	board c;

	copy_board(b, c);

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			c[i][j] &= ~TARGET;
			if (c[i][j] & BOX)
				c[i][j] |= BASE;
		}

	copy_board(c, h->imagined_hf_board);

	/*
	FILE *fp;
	fp = fopen("c:\\sokoban2\\dbg.sok", "a");
	clear_boxes_inplace(c);
	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if (initial_board[i][j] & BOX)
			{
				c[i][j] |= BOX;
				c[i][j] &= ~SOKOBAN;
			}

			if (c[i][j] & BASE)
			{
				c[i][j] &= ~BASE;
				c[i][j] |= TARGET;
			}
		}
	print_board(c);

	save_board_to_file(c, fp);
	fprintf(fp, "\n");
	fclose(fp);
	*/
}


void print_board_with_imagined(board b_in, helper* h)
{
	int i, j;
	board b;

	copy_board(b_in, b);

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			b[i][j] &= ~TARGET;
			if (h->imagined_hf_board[i][j] & BOX)
				b[i][j] |= BASE;
		}
	print_board(b);
}
