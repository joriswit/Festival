// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "park_order.h"
#include "util.h"
#include "rooms.h"
#include "girl.h"
#include "overlap.h"
#include "tree.h"
#include "helper.h"
#include "perimeter.h"

int debug_best_parking = 0;

void show_parking_order_on_board(helper *h)
{
	int i, j, k, index;
	board b;

	copy_board(initial_board, b);

	printf("parking order:\n\n");
	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			if (inner[i][j] == 0)
			{
				if (b[i][j] == WALL)
					printf("#");
				else
					printf(" ");
				continue;
			}

			index = y_x_to_index(i, j);

			for (k = 0; k < h->parking_order_num; k++)
				if (index == (h->parking_order)[k].to)
					break;

			if (k < h->parking_order_num)
			{
				if (k < 26)
					printf("%c", 'A' + k);
				else
					printf("%c", 'a' + k - 26);

			}
			else
			{
				if (b[i][j] & WALL)
					printf("#");
				else
					printf(" ");
			}
		}
		printf("\n");
	}
	printf("\n");
}


void show_parking_order(helper *h)
{
	int i, y, x;
	park_order_data *parking_order = h->parking_order;
	int parking_order_num = h->parking_order_num;

	if (verbose < 4) return;

	for (i = 0; i < parking_order_num; i++)
	{
		printf("%3d : ", i);
		index_to_y_x(parking_order[i].to, &y, &x);

		printf("%2d %2d ", y, x);

		printf("from: ");
		if (parking_order[i].from < 0)
		{
			if (parking_order[i].from == -1)
				printf("  -   ");
			if (parking_order[i].from == -2)
				printf(" init ");
		}
		else
		{
			index_to_y_x(parking_order[i].from, &y, &x);
			printf("%2d %2d ", y, x);
		}

		printf("until: ");
		if (parking_order[i].until == -1)
			printf("  -  ");
		else
			printf(" %2d", parking_order[i].until);
		printf("\n");
	}

	printf("\n");

	show_parking_order_on_board(h);
}

void verify_parking_order(helper *h)
{
	int i;

	park_order_data *parking_order = h->parking_order;
	int parking_order_num = h->parking_order_num;

	for (i = 0; i < parking_order_num; i++)
	{
		if ((parking_order[i].to >= index_num) || (parking_order[i].to < 0))
		{
			printf("parking order %d (to) is %d. index_num is:%d\n", i, parking_order[i].to, index_num);
			exit(0);
		}
		if (parking_order[i].from >= index_num)
		{
			printf("parking order %d (from) is %d. index_num is:%d\n", i, parking_order[i].from, index_num);
			exit(0);
		}
	}
}


void move_mandatory_boxes_to_beginning(park_order_data *p, int n)
{
	park_order_data tmp[MAX_SOL_LEN];
	int pos = 0;
	int i;

	for (i = 0; i < n; i++)
	{
		if ((p[i].from == -2) && (p[i].until == -1))
			tmp[pos++] = p[i];
	}
	for (i = 0; i < n; i++)
	{
		if ((p[i].from != -2) || (p[i].until != -1))
			tmp[pos++] = p[i];
	}

	for (i = 0; i < n; i++)
		p[i] = tmp[i];
}



void recompute_until(helper *h)
{
	int i, j;

	park_order_data *parking_order = h->parking_order;
	int parking_order_num = h->parking_order_num;

	int n = parking_order_num;

	for (i = 0; i < n; i++)
		parking_order[i].until = -1;

	for (i = 0; i < n; i++)
	{
		for (j = i + 1; j < n; j++)
		{
			if (parking_order[j].from == parking_order[i].to)
			{
				parking_order[i].until = j;
				break;
			}
		}
	}
}


int fix_consecutive_pushes(helper *h)
{
	int i,j;
	int fixed = 0;

	park_order_data *parking_order = h->parking_order;

	for (i = 0; i < (h->parking_order_num - 1); i++)
	{
		if (parking_order[i].from == -2)
			continue; // it's not really a push move. First have all inits on place.

		// we might concat a->b b->a into a->a
		if (parking_order[i].to == parking_order[i].from)
		{
			for (j = i; j < (h->parking_order_num - 1); j++)
				parking_order[j] = parking_order[j + 1];

			h->parking_order_num--;
			i = 0;
			fixed = 1;
		}


		if (parking_order[i].to == parking_order[i + 1].from)
		{
			parking_order[i].to = parking_order[i + 1].to;

			for (j = i + 1; j < (h->parking_order_num - 1); j++)
				parking_order[j] = parking_order[j + 1];

			h->parking_order_num--;
			i = 0;
			fixed = 1;
		}
	}

	recompute_until(h);

	return fixed;
}

void get_parking_plan(tree *t, expansion_data *e, helper *h)
{
	// compute a parking plan for girl mode (using all boxes)
	int from, to;
	int i, j;
	int n = 0;
	board b;
	move m;

	park_order_data *parking_order = h->parking_order;

	// add initial boxes
	bytes_to_board(e->b, b);

	for (i = height - 1; i >= 0; i--)
	{
		for (j = width - 1; j >= 0; j--)
			if (b[i][j] & BOX)
			{
				parking_order[n].from = -2;
				parking_order[n].to = y_x_to_index(i, j);
				n++;
			}
	}


	while (e->father != 0)
	{		
		m = get_move_to_node(t, e);

		if (m.base == 0)
		{
			from = m.from;
			to = m.to;
		}
		else
		{
			from = m.from;
			to = -1;
		}

		e = e->father;

//		print_node(t, e, 0, "", h);

		if (from == to)
			continue;

		parking_order[n].from = to;
		parking_order[n].to = from;
		n++;

		if (n >= MAX_SOL_LEN) exit_with_error("plan is too long");
	}

	h->parking_order_num = n;

	recompute_until(h);

	fix_consecutive_pushes(h);

	move_mandatory_boxes_to_beginning(parking_order, h->parking_order_num);
	// possibly change inner order for initial boxes

	if (verbose >= 5)
		show_parking_order(h);
}


void keep_only_perm(helper *h)
{
	// when all boxes remain on board, forget all moves and keep only targets perm
	int n = 0;
	int i;
	park_order_data tmp[MAX_SOL_LEN];

	park_order_data *parking_order = h->parking_order;

	for (i = 0; i < h->parking_order_num; i++)
	{
		if (parking_order[i].until == -1)
		{
			tmp[n] = parking_order[i];
			if (tmp[n].from != -2)
				tmp[n].from = -1;
			n++;
		}
	}

	for (i = 0; i < n; i++)
		parking_order[i] = tmp[i];

	h->parking_order_num = n;
}


void reduce_parking_order(helper *h)
{
	int i;
	int init_num = 0;

	park_order_data *parking_order = h->parking_order;

	save_full_parking_order(h);   // backup original plan (for "overlap")

	for (i = 0; i < h->parking_order_num; i++)
		if (parking_order[i].from == -2)
			init_num++;

	h->boxes_were_removed = (init_num == global_boxes_in_level ? 0 : 1);

	if (h->boxes_were_removed == 0)
		keep_only_perm(h);
}


void conclude_parking(tree *t, expansion_data *e, helper *h)
{
	if (verbose >= 5)
		printf("\nfinding parking\n");

	if (t->search_mode == GIRL_SEARCH)
		e = find_best_RL_parking_node(t);

	if (e == 0)
	{
		if (verbose >= 4)
			printf("could not form packing plan\n");
		h->parking_order_num = 0;
		return;
	}

	if (e->depth >= (MAX_SOL_LEN - global_boxes_in_level - 2))
	{
		if (verbose >= 4)
			printf("plan is too long\n");
		h->parking_order_num = 0;
		return;
	}

	get_parking_plan(t,e,h);
}






int mandatory_inits_missing(int *has_box, int init_num, helper *h)
{
	int i;
	park_order_data *parking_order = h->parking_order;

	for (i = 0; i < init_num; i++)
		if (parking_order[i].until == -1) // box is there from the init to the end
			if (has_box[i] == 0)
				return 1;
	return 0;
}

int get_number_of_mandatory_inits(int *has_box, int init_num, helper *h)
{
	int i;
	int n = 0;
	park_order_data *parking_order = h->parking_order;

	for (i = 0; i < init_num; i++)
		if (parking_order[i].until == -1)
			if (has_box[i])
				n++;
	return n;
}

void get_boxes_in_place(board b, int *has_box, helper *h)
{
	int i,y,x;
	park_order_data *parking_order = h->parking_order;

	for (i = 0; i < h->parking_order_num; i++)
	{
		index_to_y_x(parking_order[i].to, &y, &x);
		has_box[i] = (b[y][x] & BOX ? 1 : 0);
	}

}

int score_parked_boxes(board b, helper *h)
{
	int has_box[MAX_INNER];
	int i;
	int packed;
	int init_num = 0;
	park_order_data *parking_order = h->parking_order;
	int good_init = 0;

	get_boxes_in_place(b, has_box, h);

	for (i = 0 ; i < h->parking_order_num; i++)
		if (parking_order[i].from == -2)
			init_num++;

	if (mandatory_inits_missing(has_box, init_num,h))
		return get_number_of_mandatory_inits(has_box, init_num,h);


	packed = h->parking_order_num;

	/*
	We check if "packed" boxes are in place.
	Some boxes are allowed to missing if they were moved later on (in "until" move)
	*/

	for (i = h->parking_order_num - 1; i >= init_num; i--)
	{
		if (has_box[i]) // box is in place, "packed" not contradicted
			continue;

		// so box is not in place.

		if (parking_order[i].until == -1)
		{
			packed = i;
		}
		else
		{
			if (parking_order[i].until >= packed)
				packed = i;
		}
	}

	for (i = 0; i < init_num; i++)
	{
		if (has_box[i] || (parking_order[i].until < packed))
			good_init++;
	}

	packed -= (init_num - good_init);

	return packed;
}

int all_fixed_initials_on_place(board b, helper *h)
{
	int has_box[MAX_INNER];
	int i;
	park_order_data *parking_order = h->parking_order;

	get_boxes_in_place(b, has_box, h);

	for (i = 0; i < h->parking_order_num; i++)
	{
		if (parking_order[i].from == -2)
			if (parking_order[i].until == -1)
				if (has_box[i] == 0)
					return 0;
	}
	return 1;
}

int next_parking_place(board b, helper *h)
{
	int i;
	int has_box[MAX_INNER];
	int packed;
	int init_num = 0;
	park_order_data *parking_order = h->parking_order;

	if (all_fixed_initials_on_place(b, h) == 0)
		return -1;

	get_boxes_in_place(b, has_box, h);

	while (parking_order[init_num].from == -2)
		init_num++;

	packed = h->parking_order_num;

	for (i = h->parking_order_num - 1; i >= init_num; i--)
	{
		if (has_box[i]) // box is in place, "packed" not contradicted
			continue;

		if (parking_order[i].until == -1)
		{
			packed = i;
		}
		else
		{
			if (parking_order[i].until >= packed)
				packed = i;
		}
	}

	if (packed == h->parking_order_num)
		return -1;

	return parking_order[packed].to;
}


