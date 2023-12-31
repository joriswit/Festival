// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include <stdio.h>

#include "bfs.h"
#include "corral.h"
#include "distance.h"
#include "util.h"
#include "fixed_boxes.h"
#include "deadlock.h"

int debug_corral = 0;

#define MAX_CORRALS_NUM 70


typedef struct
{
	board corral;
	int_board corral_d;
	int corral_component_used[MAX_CORRALS_NUM];
	int corral_sokoban_comp;
	int corral_comps_num;
	int corral_candidates[MAX_CORRALS_NUM];
} corral_data;


void get_pusher_and_dest(int i, int j, int k,
	int *pusher_y, int *pusher_x, int *dest_y, int *dest_x)
{
	*pusher_y = i + delta_y[k];
	*pusher_x = j + delta_x[k];

	*dest_y = i - delta_y[k];
	*dest_x = j - delta_x[k];
}

int get_direction_from_pusher_dest(int pusher_y, int pusher_x, int dest_y, int dest_x,
	int *box_y, int *box_x)
{
	int i;
	for (i = 0; i < 4; i++)
	{
		if ((pusher_y + delta_y[i] * 2) == dest_y)
			if ((pusher_x + delta_x[i] * 2) == dest_x)
			{
				*box_y = pusher_y + delta_y[i];
				*box_x = pusher_x + delta_x[i];
				return i;
			}
	}
	exit_with_error("no direction");
	return -1;
}


void get_corral_boxes(board orig_b, board corral_boxes, corral_data *cd)
{
	// get all the boxes that touch the global "corral"
	// also mark the corral area as sokoban.
	int i, j, k;

	clear_boxes(orig_b, corral_boxes);
	clear_sokoban_inplace(corral_boxes);

	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			if ((cd->corral)[i][j] == 0) continue;

			corral_boxes[i][j] |= SOKOBAN;

			for (k = 0; k < 8; k++)
				if (orig_b[i + delta_y_8[k]][j + delta_x_8[k]] & BOX)
					corral_boxes[i + delta_y_8[k]][j + delta_x_8[k]] |= BOX;
		}
	}
}

int is_possible_push_into_corral(board b,
	int pusher_y, int pusher_x,
	int dest_y, int dest_x,
	corral_data *cd)
{
	int box_y, box_x, direction;

	if (b[pusher_y][pusher_x] == WALL) return 0;
	if (b[dest_y][dest_x] & OCCUPIED) return 0;

	// only look for pushes into the corral
	if ((cd->corral)[pusher_y][pusher_x]) return 0;
	if ((cd->corral)[dest_y][dest_x] == 0) return 0;

	direction = get_direction_from_pusher_dest(pusher_y, pusher_x, dest_y, dest_x, &box_y, &box_x);

	if (deadlock_in_direction(b, box_y, box_x, direction)) 
		return 0;

	return 1;
}

int is_possible_push_aside_corral(board b,
	int pusher_y, int pusher_x,
	int dest_y, int dest_x,
	corral_data *cd)
{
	int box_y, box_x, direction;

	// can't push if occupied
	if (b[pusher_y][pusher_x] & OCCUPIED) return 0;
	if (b[dest_y][dest_x] & OCCUPIED) return 0;

	if ((cd->corral)[pusher_y][pusher_x]) return 0;
	if ((cd->corral)[dest_y][dest_x]) return 0;

	direction = get_direction_from_pusher_dest(pusher_y, pusher_x, dest_y, dest_x, &box_y, &box_x);

	if (deadlock_in_direction(b, box_y, box_x, direction))
		return 0;

	return 1;
}



int is_corral_active(board b_in, corral_data *cd) 
// still work to do inside corral
{
	int i, j;
	board corral_boxes;
	
	get_corral_boxes(b_in, corral_boxes, cd);
	// gets the boxes around the corral, AND marks inner corral with sokoabn

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if ((cd->corral)[i][j])
				if (corral_boxes[i][j] == TARGET)
					exit_with_error("debug corral");

			if ((cd->corral)[i][j])
				if (corral_boxes[i][j] == (TARGET | SOKOBAN)) // a target that is not occupied
					return 1;

			if (corral_boxes[i][j] == BOX) return 1; // a box not on a target
		}
	return 0;
}



void set_corral_options(board b_in, board corral_options, corral_data *cd)
// mark the boxes that can be pushed
{
	int i, j, k;
	int pusher_y, pusher_x, dest_y, dest_x;
	board corral_boxes;

	get_corral_boxes(b_in, corral_boxes, cd);

	clear_boxes(b_in, corral_options);

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if ((corral_boxes[i][j] & BOX) == 0)
				continue;

			for (k = 0; k < 4; k++)
			{
				get_pusher_and_dest(i, j, k, &pusher_y, &pusher_x, &dest_y, &dest_x);

				if (is_possible_push_into_corral(corral_boxes, pusher_y, pusher_x, dest_y, dest_x, cd))
					if (b_in[pusher_y][pusher_x] & SOKOBAN)
						corral_options[i][j] |= BOX;
			}
		}	
}


void get_corral_candidates(board b, corral_data *cd)
{
	int y, x;
	int i, j, k;
	int other;

	// find areas that are touched by sokoban.

	cd->corral_comps_num = mark_connectivities(b, cd->corral_d);

	if (cd->corral_comps_num >= MAX_CORRALS_NUM)
		return;

	get_sokoban_position(b, &y, &x);

	cd->corral_sokoban_comp = (cd->corral_d)[y][x];
	if (debug_corral)
		printf("corral_sokoban_comp= %d\n", cd->corral_sokoban_comp);

	for (i = 0; i < cd->corral_comps_num; i++)
		(cd->corral_candidates)[i] = 0;

	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			if ((b[i][j] & BOX) == 0) continue;

			for (k = 0; k < 4; k++)
			{
				if ((cd->corral_d)[i + delta_y[k]][j + delta_x[k]] == cd->corral_sokoban_comp)
				{
					other = (cd->corral_d)[i - delta_y[k]][j - delta_x[k]];
					if ((other != 1000000) && (other != cd->corral_sokoban_comp))
						(cd->corral_candidates)[other] = 1;
				}
			}

		}
	}

	if (debug_corral)
		print_connectivity_comps(cd->corral_d);


	if (debug_corral)
	{
		printf("corral candidates: ");
		for (i = 0; i < cd->corral_comps_num; i++)
			if ((cd->corral_candidates)[i])
				printf("%d ", i);
		printf("\n");
	}
}


int init_corral_with_comp(int comp1, int comp2, board b_in, corral_data *cd)
{
	int i, j;

	zero_board(cd->corral);

	for (i = 0; i < cd->corral_comps_num; i++)
		(cd->corral_component_used)[i] = 0;

	(cd->corral_component_used)[comp1] = 1;
	(cd->corral_component_used)[comp2] = 1;
	
	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			if (((cd->corral_d)[i][j] == comp1) || ((cd->corral_d)[i][j] == comp2))
			{
				(cd->corral)[i][j] = 1;
			}
		}
	}

	return is_corral_active(b_in, cd);
}

void update_corral_with_comp(int comp1, corral_data *cd)
{
	int i, j;

	(cd->corral_component_used)[comp1] = 1;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if ((cd->corral_d)[i][j] == comp1)
				(cd->corral)[i][j] = 1;

}


int box_touches_corral_and_non_corral(board b, int y, int x, board corral)
{
	int i;

	int touches_corral = 0;
	int touches_non_corral = 0;

	for (i = 0; i < 4; i++)
	{
		if (corral[y + delta_y[i]][x + delta_x[i]]) touches_corral = 1;

		if ((b[y + delta_y[i]][x + delta_x[i]] & OCCUPIED) == 0)
			if (corral[y + delta_y[i]][x + delta_x[i]] == 0) 
				touches_non_corral = 1;
	}

	return touches_corral & touches_non_corral;

}

int should_add_component_to_corral(board corral_boxes, int *component, corral_data *cd)
{
	int i, j, k, d;
	int pusher_x, pusher_y;
	int dest_x, dest_y;
	int can_push_inside, can_push_aside;

	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			if ((corral_boxes[i][j] & BOX) == 0)
				continue;

			if (box_touches_corral_and_non_corral(corral_boxes, i, j, cd->corral) == 0)
				continue;

			for (k = 0; k < 4; k++)
			{
				get_pusher_and_dest(i, j, k, &pusher_y, &pusher_x, &dest_y, &dest_x);

				can_push_inside = is_possible_push_into_corral(corral_boxes, pusher_y, pusher_x, dest_y, dest_x, cd);
				if (corral_boxes[pusher_y][pusher_x] & OCCUPIED) can_push_inside = 0;

				can_push_aside = is_possible_push_aside_corral(corral_boxes, pusher_y, pusher_x, dest_y, dest_x, cd);
					
				if ((can_push_inside == 0) && (can_push_aside == 0))
					continue;

				// by previous check, we know that pusher is outside the corral

				if (can_push_inside)
				{
					d = (cd->corral_d)[pusher_y][pusher_x];

					if (d != 1000000) // could be a box in front of the corral
					if ((d != cd->corral_sokoban_comp) && ((cd->corral_component_used)[d] == 0))
					{
						*component = d;
						return 1;
					}
				}

				if (can_push_aside)
				{
					d = (cd->corral_d)[dest_y][dest_x];

					if (d == 1000000) exit_with_error("this is strange");
					if ((d != cd->corral_sokoban_comp) && ((cd->corral_component_used[d]) == 0))
					{
						*component = d;
						return 1;
					}
				}
			} // k
		}   // j
	}  // i
	return 0;
} 
	


int can_push_box_out_of_corral(board b, board corral)
{
	int i, j, k;
	int pusher_x, pusher_y;
	int dest_x, dest_y;
	int box_y, box_x, direction;

	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			if ((b[i][j] & BOX) == 0)
				continue;

			if (inner[i][j] == 0) continue;

			for (k = 0; k < 4; k++)
			{
				get_pusher_and_dest(i, j, k, &pusher_y, &pusher_x, &dest_y, &dest_x);
				direction = get_direction_from_pusher_dest(pusher_y, pusher_x, dest_y, dest_x, &box_y, &box_x);

				// can't push if occupied
				if (b[pusher_y][pusher_x] & OCCUPIED) continue;
				if (b[dest_y][dest_x] & OCCUPIED) continue;

				if (deadlock_in_direction(b, box_y, box_x, direction)) continue;

				if (corral[pusher_y][pusher_x] == 0)
					if (corral[dest_y][dest_x] == 0)
					{
						if (debug_corral)
							printf("can push aside box %d %d\n\n", i, j);
						return 1;
					}
			}
		}
	}

	return 0;
}



int check_pi_corral(board corral_boxes, board b_in, corral_data *cd)
{
	int i, j, k;
	int pusher_x, pusher_y;
	int dest_x, dest_y;

	// verify that all the boxes on the corral boundary can actually be pushed inside
	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			if ((corral_boxes[i][j] & BOX) == 0) continue;

			for (k = 0; k < 4; k++)
			{
				get_pusher_and_dest(i, j, k, &pusher_y, &pusher_x, &dest_y, &dest_x);

				if (is_possible_push_into_corral(corral_boxes, pusher_y, pusher_x, dest_y, dest_x, cd) == 0)
					continue;

				if (corral_boxes[pusher_y][pusher_x] & OCCUPIED) continue; // (i,j) is not on the corral boundary

				if ((cd->corral)[dest_y][dest_x] == 0) exit_with_error("not right1");
				if ((cd->corral)[pusher_y][pusher_x] == 1) exit_with_error("not right2");

				if ((b_in[pusher_y][pusher_x] & SOKOBAN) == 0)
				{
					if (debug_corral)
					{
						print_board(cd->corral);
						printf("box in front of corral: %d %d\n", i, j);
					}
					return 0;
				}
			}
		}
	}

	return 1;
}

int expand_corral(board b_in, corral_data *cd)
{
	// given the current corral, try to expand it by adding components, until it can
	// be proven to be a pi-corral.
	// Or report that it's not a pi-corral once the expansions stop.
	board corral_boxes;
	int res, d;

	do
	{
		get_corral_boxes(b_in, corral_boxes, cd);

		if (debug_corral)
		{
			printf("corral boxes:\n");
			print_board(corral_boxes);
			my_getch();
		}

		res = should_add_component_to_corral(corral_boxes, &d, cd);
		if (res)
		{
			if (debug_corral)
				printf("adding component %d\n", d);
			update_corral_with_comp(d, cd);
		}
	} while (res);

	if (can_push_box_out_of_corral(corral_boxes, cd->corral))
		return 0;

	if (check_pi_corral(corral_boxes, b_in, cd) == 0)
		return 0;

	if (debug_corral)
		printf("pi corral found!\n");
	return 1;
}




void init_touches_two_comps(board b, corral_data *cd, char touches_two_comps[MAX_CORRALS_NUM][MAX_CORRALS_NUM])
{
	int i, j, k, l;
	int d1, d2;

	if (cd->corral_comps_num >= MAX_CORRALS_NUM)
	{
		print_board(b);
		printf("corrals: %d\n", cd->corral_comps_num);
		exit_with_error("Too many corrals");
	}

	for (i = 0; i < MAX_CORRALS_NUM; i++)
		for (j = 0; j < MAX_CORRALS_NUM; j++)
			touches_two_comps[i][j] = 0;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if ((b[i][j] & BOX) == 0) continue;

			for (k = 0; k < 4; k++)
				for (l = 0; l < 4; l++)
				{
					d1 = (cd->corral_d)[i + delta_y[k]][j + delta_x[k]];
					d2 = (cd->corral_d)[i + delta_y[l]][j + delta_x[l]];

					if ((d1 != 1000000) && (d2 != 1000000))
						touches_two_comps[d1][d2] = 1;
				}
		}
}


int detect_corral(board b, board corral_options)
{
	int i,j,res;
	char touches_two_comps[MAX_CORRALS_NUM][MAX_CORRALS_NUM];

	corral_data cd;

	if (debug_corral)
	{
		printf("detecting corral in board:\n");
		print_board(b);
	}

	get_corral_candidates(b, &cd);

	if (cd.corral_comps_num >= MAX_CORRALS_NUM)
		return 0; // do not handle corrals when there are too many of them

	for (i = 0; i < cd.corral_comps_num; i++)
	{
		if (cd.corral_candidates[i] == 0) continue;

		if (debug_corral)
			printf("trying component %d\n", i);
		res = init_corral_with_comp(i, i, b, &cd);
		if (res == 0) continue;

		res = expand_corral(b, &cd);
		if (res == 1)
		{
			set_corral_options(b, corral_options, &cd);
			return 1;
		}
	}

	init_touches_two_comps(b, &cd, touches_two_comps);

	// look for two-rooms corral
	for (i = 0; i < cd.corral_comps_num; i++)
	{

		if (i == cd.corral_sokoban_comp) continue;

		for (j = i + 1; j < cd.corral_comps_num; j++)
		{
			if (j == cd.corral_sokoban_comp) continue;

			if (cd.corral_candidates[j] == 0) 
				if (cd.corral_candidates[i] == 0) continue;

			if ((touches_two_comps[i][j]) == 0) continue;

			if (debug_corral)
				printf("trying components %d %d\n", i, j);
			res = init_corral_with_comp(i, j, b, &cd);
			if (res == 0) continue;

			res = expand_corral(b, &cd);
			if (res == 1)
			{
				set_corral_options(b, corral_options, &cd);
				return 1;
			}
		}
	}
	
	return 0;
}