#include <stdio.h>

#include "mini_search.h"
#include "positions.h"
#include "util.h"
#include "helper.h"

int debug_mini_search = 0;


int get_move_with_most_packing(position *positions, int pos_num, int *son)
{
	int i, j;
	position *p;
	int max_boxes_on_targets = -1;
	int best_pos = -1;
	int best_connectivity = 1000000;

	for (i = 0; i < pos_num; i++)
	{
		p = positions + i;
		if (p->position_deadlocked) continue;

		for (j = 0; j < p->moves_num; j++)
		{
			if (p->move_deadlocked[j]) continue;
			if (p->place_in_positions[j]) continue;

			if (p->scores[j].boxes_on_targets < max_boxes_on_targets)
				continue;

			if (p->scores[j].boxes_on_targets > max_boxes_on_targets)
			{
				max_boxes_on_targets = p->scores[j].boxes_on_targets;
				best_pos = i;
				*son = j;
				best_connectivity = p->scores[j].connectivity;
				continue;
			}

			if (p->scores[j].boxes_on_targets != max_boxes_on_targets)
				exit_with_error("bad flow");

			if (p->scores[j].connectivity < best_connectivity)
			{
				best_pos = i;
				*son = j;
				best_connectivity = p->scores[j].connectivity;
			}
		}
	}
	return best_pos;
}

int has_a_move_with_all_boxes_packed(position *p)
{
	// this routine is needed because the winning position can be the first position.
	// But get_move_with_most_packing can't choose an existing position.

	int i;
	board b;
	int boxes_in_search;

	bytes_to_board(p->board, b);
	boxes_in_search = boxes_in_level(b);

	for (i = 0; i < p->moves_num; i++)
		if (p->scores[i].boxes_on_targets == boxes_in_search)
			return 1;
	return 0;
}


int can_put_all_on_targets(board b, int max_size)
{
	position *positions;
	int pos_num = 0;
	int res = 0;
	int pos, son;
	board c;
	int total_leaves;
	helper h;

	init_helper(&h);

	positions = allocate_positions(max_size);
	set_first_position(b, 0, positions, MINI_SEARCH, &h);
	pos_num = 1;
	total_leaves = positions[0].moves_num;

	while (1)
	{
		// for large levels the search may explode even with a limited "max_size".
		if (positions[pos_num - 1].moves_num > 1000)
		{
			res = -1;
			break;
		}

		if (has_a_move_with_all_boxes_packed(positions + pos_num - 1))
		{
			res = 1;
			break;
		}

		pos = get_move_with_most_packing(positions, pos_num, &son);

		if (pos == -1)
		{
			res = 0;
			break;
		}

		if (debug_mini_search)
		{
			printf("mini search: position to expand:\n");
			print_position(positions + pos, 1);
		}

		expand_position(positions + pos, son, 0, positions, pos_num, &h);
		pos_num++;

		if (debug_mini_search)
		{
			printf("mini search: position after expansion\n");
			print_position(positions + pos_num - 1, 1);
			my_getch();
		}

		bytes_to_board(positions[pos_num - 1].board, c);
		if (all_boxes_in_place(c, 0))
		{
			res = 1;
			break;
		}

		if (pos_num == max_size)
		{
			res = -1;
			break;
		}
		
	}

	free_positions(positions, max_size);

	return res;
}
