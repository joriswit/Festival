#include <stdio.h>

#include "corral_deadlock.h"
#include "positions.h"
#include "deadlock_cache.h"
#include "deadlock_utils.h"
#include "util.h"
#include "fixed_boxes.h"
#include "bfs.h"
#include "wobblers.h"
#include "mini_search.h"
#include "stuck.h"

#define MAX_DEADLOCK_SEARCH 50

#define NOT_DEADLOCK 0
#define IS_DEADLOCK  1
#define SEARCH_EXCEEDED 2

int debug_corral_deadlock = 0;


int check_for_deadlock_zone_moves(board b, move* moves, int moves_num)
{
	// if a box can be sent outside the deadlock zone, it is better than other moves

	int i, y, x;

	for (i = 0; i < moves_num; i++)
	{
		index_to_y_x(moves[i].to, &y, &x);
		if (b[y][x] & DEADLOCK_ZONE) continue;

		moves[0] = moves[i];
		moves[0].kill = 1;
		return 1;
	}

	return moves_num;
}


int board_contains_more_deadlocks(board new_board, board old_board)
{
	int i, j;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (new_board[i][j] & DEADLOCK_ZONE)
				if ((old_board[i][j] & DEADLOCK_ZONE) == 0)
					return 1;
	return 0;

}

int deadlock_zone_is_bigger(position *old_pos, position *new_pos)
{
	board old_board, new_board;
	
	bytes_to_board(old_pos->board, old_board);
	bytes_to_board(new_pos->board, new_board);

	return board_contains_more_deadlocks(new_board, old_board);
}

void insert_positions_to_deadlock_cache(position *positions, int pos_num, int result)
{
	int i;
	position *p;

	// insert positions that are surely deadlocked
	// note: in case of cycles it is possible that some positions are not marked as deadlock
	// even though the entire search is deadlocked.
	for (i = 0; i < pos_num; i++)
	{
		if ((positions[i].position_deadlocked) || (result == IS_DEADLOCK))
			insert_to_deadlock_cache(positions[i].position_hash, IS_DEADLOCK,
									 positions[i].pull_mode, DEADLOCK_SEARCH);
	}

	if (result == NOT_DEADLOCK)
	{
		// mark positions that lead to a non-deadlocked state
		p = positions + pos_num - 1; // the search stops when the last position is not deadlocked
		do
		{
//			print_position(p, 0);
			insert_to_deadlock_cache(p->position_hash, NOT_DEADLOCK, p->pull_mode, DEADLOCK_SEARCH);
			p = p->father;
		} while (p);
	}
}

int get_components_around_place(int y, int x, int_board d, int *list)
{
	int i, j, k;
	int n = 0;

	for (k = 0; k < 4; k++)
	{
		i = y + delta_y[k];
		j = x + delta_x[k];

		if (d[i][j] == 1000000)
			continue;

		list[n++] = d[i][j];
	}
	return n;
}



void keep_two_components(board b, int_board d, int comp1, int comp2, int pull_mode)
{
	// keep only boxes that touch components comp1, comp2
	int i, j;
	board group;

	zero_board(group);

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if ((d[i][j] == comp1) || (d[i][j] == comp2))
				group[i][j] = 1;

	init_deadlock_zone_from_group(b, group, pull_mode);

}

int is_better_deadlock_score(score_element *new_score, score_element *old_score)
{
	// prioritize connectivity, because this could end the search

	if (new_score->connectivity < old_score->connectivity) return 1;
	if (new_score->connectivity > old_score->connectivity) return 0;

	if (new_score->boxes_in_level < old_score->boxes_in_level) return 1;
	if (new_score->boxes_in_level > old_score->boxes_in_level) return 0;

	// prioritize smaller zones, which can serve as blocks even if the full search terminates
	if (new_score->hotspots < old_score->hotspots) return 1;
	if (new_score->hotspots > old_score->hotspots) return 0;


	return 0;
}

int choose_deadlock_son(position *deadlock_positions, int deadlock_pos_num, int *son)
{
	int i, j;
	int res = -1;
	score_element best_score;

	for (i = 0; i < deadlock_pos_num; i++)
	{
		if (deadlock_positions[i].position_deadlocked) continue;

		for (j = 0; j < deadlock_positions[i].moves_num; j++)
		{
			if (deadlock_positions[i].move_deadlocked[j]) continue;
			if (deadlock_positions[i].place_in_positions[j]) continue;

			if ((res == -1) || (is_better_deadlock_score(deadlock_positions[i].scores + j, &best_score)))
			{
				res = i;
				*son = j;
				best_score = deadlock_positions[i].scores[j];
			}
		}
	}

	return res;
}


/*
In this example the deadlock zone gets bigger (in pull mode) after pulling the box.
########
# $    #
#$     #
# $    #
#  $@  #
########
*/


int son_can_break_free(position *p, int pull_mode)
{
	int i;
	board b, c;

	bytes_to_board(p->board, b);

	// check if the new position is actually solved
	if (pull_mode == 0)
		if (work_left_in_deadlock_zone(b) == 0)
			return 1;

	for (i = 0; i < p->moves_num; i++)
	{ 
		if (p->move_deadlocked[i]) continue;

		if (p->scores[i].hotspots == 0)
			return 1; // no deadlock zone

		if (p->scores[i].connectivity == 1)
			return 1;

		if (get_from_deadlock_cache(p->hashes[i], pull_mode, DEADLOCK_SEARCH) == NOT_DEADLOCK)
			return 1;

		if (pull_mode) // in push mode the DZ never gets bigger (TODO: verify more carefully)
		{
			copy_board(b, c);
			apply_move(c, p->moves + i, DEADLOCK_SEARCH);

			if (board_contains_more_deadlocks(c, b))
				return 1; // deadlock zone got bigger

			if (exited_deadlock_zone(c))
				return 1;
		}
	}

	return 0;
}



void mark_deadlocked_sons_using_cache(position *deadlock_positions, int deadlock_pos_num)
{
	int i;
	int node = deadlock_pos_num - 1;
	position *p = deadlock_positions + node;

	for (i = 0; i < p->moves_num; i++)
		if (get_from_deadlock_cache(p->hashes[i], p->pull_mode, DEADLOCK_SEARCH) == IS_DEADLOCK)
			if (p->place_in_positions[i] == 0) // a leaf, not a visited node
				set_leaf_as_deadlock(deadlock_positions, deadlock_pos_num, node, i);
}

/*
int corral_deadlock_search_actual_2(board b, int pull_mode, int search_size)
{
	int result = -1;
	int node, son;
	int total_leaves;
	position* deadlock_positions;
	int deadlock_pos_num;
	board c;

	helper h;
	init_helper(&h);

	deadlock_positions = allocate_positions(search_size);

	set_first_position(b, pull_mode, deadlock_positions, MINI_SEARCH, &h);
	deadlock_pos_num = 1;

	if (debug_corral_deadlock)
	{
		printf("root position:\n");
		print_position(deadlock_positions, 1);
	}

	while (1)
	{
		total_leaves = estimate_uniqe_moves(deadlock_positions, deadlock_pos_num);
		if (total_leaves > search_size)
		{
			result = SEARCH_EXCEEDED;
			break;
		}

		node = choose_deadlock_son(deadlock_positions, deadlock_pos_num, &son);

		if (node == -1)
		{
			if (debug_corral_deadlock)
				printf("no more nodes - proved deadlock\n");
			// all tree is deadlocked

			result = IS_DEADLOCK;
			break;
		}

		if (debug_corral_deadlock)
		{
			printf("expanding position in deadlock search:\n");
			print_position(deadlock_positions + node, 1);
			printf("expanding with move: ");
			print_move(deadlock_positions[node].moves + son);
			printf("\n");
			//			my_getch();
		}

		expand_position(deadlock_positions + node, son, pull_mode, deadlock_positions, deadlock_pos_num, &h);
		deadlock_pos_num++;

		if (debug_corral_deadlock)
		{
			printf("after expansion in deadlock search:\n");
			print_position(deadlock_positions + deadlock_pos_num - 1, 1);
			//			my_getch();
		}

		bytes_to_board(deadlock_positions[deadlock_pos_num - 1].board, c);
		if (exited_deadlock_zone(c))
		{
			result = NOT_DEADLOCK;
			break;
		}


		if (deadlock_pos_num >= search_size)
		{
			result = SEARCH_EXCEEDED;
			break;
		}
	}

	free_positions(deadlock_positions, search_size);

	return result;
}
*/


int corral_deadlock_search_actual(board b, int pull_mode, int search_size)
{
	int result = -1;
	int node, son;
	int total_leaves;
	position *deadlock_positions;
	int deadlock_pos_num;
	helper h;

//	if (pull_mode)
//		return corral_deadlock_search_actual_2(b, pull_mode, search_size);

	init_helper(&h);

	if (pull_mode == 0)
	{
		if (work_left_in_deadlock_zone(b) == 0)
			return NOT_DEADLOCK;
	}

	deadlock_positions = allocate_positions(search_size);

	set_first_position(b, pull_mode, deadlock_positions, DEADLOCK_SEARCH, &h);
	deadlock_pos_num = 1;

	if (debug_corral_deadlock)
	{
		printf("root position:\n");
		print_position(deadlock_positions, 1);
	}

	while (1)
	{
		total_leaves = estimate_uniqe_moves(deadlock_positions, deadlock_pos_num);
		if (total_leaves > search_size)
		{
			result = SEARCH_EXCEEDED;
			break;
		}


		// check if the last position has a move that can open the corral
		if (son_can_break_free(deadlock_positions + deadlock_pos_num - 1, pull_mode))
		{
			if (debug_corral_deadlock) 
			{
				printf("can break free %016llx\n", deadlock_positions[deadlock_pos_num - 1].position_hash);
				print_position(deadlock_positions + deadlock_pos_num - 1, 1);
				my_getch();
			}

			result = NOT_DEADLOCK;
			break;
		}

		// update deadlocked sones of last node using cache info
		mark_deadlocked_sons_using_cache(deadlock_positions, deadlock_pos_num);
		// this has to be done AFTER son_can_break_free to keep the invariant that
		// the deadlock zone only gets smaller

		node = choose_deadlock_son(deadlock_positions, deadlock_pos_num, &son);

		if (node == -1)
		{
			if (debug_corral_deadlock) 
				printf("no more nodes - proved deadlock\n");
			// all tree is deadlocked

			result = IS_DEADLOCK;
			break;
		}

		if (debug_corral_deadlock)
		{
			printf("expanding position in deadlock search:\n");
			print_position(deadlock_positions + node, 1);
			printf("expanding with move: ");
			print_move(deadlock_positions[node].moves + son);
			printf("\n");
//			my_getch();
		}

		expand_position(deadlock_positions + node, son, pull_mode, deadlock_positions, deadlock_pos_num, &h);
		deadlock_pos_num++;

		if (debug_corral_deadlock)
		{
			printf("after expansion in deadlock search:\n");
			print_position(deadlock_positions + deadlock_pos_num - 1, 1);
//			my_getch();
		}

		/*
		if (pull_mode == 0)
		{
			// check if the new position is actually solved
			bytes_to_board(deadlock_positions[deadlock_pos_num - 1].board, c);

			if (work_left_in_deadlock_zone(c) == 0)
			{
				result = NOT_DEADLOCK;
				break;
			}
		}
		*/

		if (deadlock_pos_num >= search_size)
		{
			result = SEARCH_EXCEEDED;
			break;
		}
	}

	insert_positions_to_deadlock_cache(deadlock_positions, deadlock_pos_num, result);

	free_positions(deadlock_positions, search_size);
	
	return result;
}


int mini_corral(board b)
{
	UINT_64 hash;
	int res;
	board c;

	if (boxes_in_level(b) > 5) return NOT_DEADLOCK;

	hash = get_board_hash(b);

	res = get_from_deadlock_cache(hash, 0, MINI_CORRAL);

	if ((res == IS_DEADLOCK) || (res == NOT_DEADLOCK)) return res;

	if (res == -1)
	{
		insert_to_deadlock_cache(hash, SEARCH_EXCEEDED, 0, MINI_CORRAL);
		return SEARCH_EXCEEDED;
	}

	update_queries_counter(hash, 0, MINI_CORRAL);

	if (get_queries_num(hash, 0, MINI_CORRAL) != 100)
		return SEARCH_EXCEEDED;

	// so there have been 100 queries. time to actually check the pattern

	if (all_boxes_in_place(b, 0))
	{
		insert_to_deadlock_cache(hash, NOT_DEADLOCK, 0, MINI_CORRAL);
		return NOT_DEADLOCK;
	}

	res = can_put_all_on_targets(b, 1000);

	if (res == 0)
	{
		if (verbose >= 4)
		{
			printf("mini corral\n");
			clear_deadlock_zone(b, c);
			print_board(c);
			register_mini_corral(c);
		}

		insert_to_deadlock_cache(hash, IS_DEADLOCK, 0, MINI_CORRAL);
		return IS_DEADLOCK;
	}
	else
		insert_to_deadlock_cache(hash, NOT_DEADLOCK, 0, MINI_CORRAL);

	return NOT_DEADLOCK;

}


int should_reevaluate(int queries_num)
{
	if (queries_num < (MAX_DEADLOCK_SEARCH * 2))
		return 0;

	if (queries_num & (queries_num - 1))
		return 0; // not a power of 2

	return 1;
}


int advanced_deadlock_search(board b, int pull_mode)
{
	// this is a cache wrapper for advanced_deadlock_search_actual
	int res;
	UINT_64 hash;
	int queries;

	hash = get_board_hash(b);

	if (debug_corral_deadlock)
	{
		print_board(b);
		printf("BOARD HASH= %016llx\n", hash);
		my_getch();
	}

	res = get_from_deadlock_cache(hash, pull_mode, DEADLOCK_SEARCH);

	update_queries_counter(hash, pull_mode, DEADLOCK_SEARCH);
	// we take the risk that two threads will simultaneously update the counter
	// thereby risking "should_reevaluate". 
	// Getting to reevaluation is rare, more so exactly in the same time.

	if (res == SEARCH_EXCEEDED)
	{
		queries = get_queries_num(hash, pull_mode, DEADLOCK_SEARCH);

		if (should_reevaluate(queries))
		{
			if (verbose >= 5)
			{
				printf("Reevaluating board:");
				printf(pull_mode ? " (pull mode)\n" : "\n");
				print_board(b);
				printf("After %d queries\n", queries);
			}

			res = corral_deadlock_search_actual(b, pull_mode, queries);
			if (res != SEARCH_EXCEEDED)
				insert_to_deadlock_cache(hash, res, pull_mode, DEADLOCK_SEARCH);

			if (verbose >= 5)
				printf("res= %d\n\n", res);
		}
	}

	if ((res >= 0) && (res <= 2))
		return res;

	// so the corral is not in the cache

	res = corral_deadlock_search_actual(b, pull_mode, MAX_DEADLOCK_SEARCH);

	insert_to_deadlock_cache(hash, res, pull_mode, DEADLOCK_SEARCH);

	return res;
}

/*
int learn_pull_deadlock(board b_in)
{
	board b;
	UINT_64 hash;
	int res;

	copy_board(b_in, b);
	clear_bases_inplace(b);

	set_deadlock_zone_in_pull_mode(b);
	clear_boxes_out_of_deadlock_zone(b);

	hash = get_board_hash(b) ^ 0x7301730173017301;

	res = get_from_deadlock_cache(hash, 1);

	if ((res == 1) || (res == 0))
	{
		printf("already in hash: %d\n", res);
		return 1;
	}

	printf("learning pattern:\n");
	print_board(b);

	res = corral_deadlock_search_actual_2(b, 1, MAX_DEADLOCK_SEARCH * 100);

	printf("res= %d\n", res);

	my_getch();

	insert_to_deadlock_cache(hash, res, 1);

	return (res == IS_DEADLOCK ? 1 : 0);
}
*/


int advanced_deadlock_pull_mode(board b_in, int without_wobblers)
{
	board b, c;
	int res;

	if (debug_corral_deadlock)
	{
		printf("debugging board:\n");
		print_board(b_in);
	}

	copy_board(b_in, b);
	clear_bases_inplace(b);
	if (get_connectivity(b) == 1) return 0;

	if (without_wobblers)
		remove_wobblers(b, 1);

	set_deadlock_zone_in_pull_mode(b);

	copy_board(b, c);

	clear_boxes_out_of_deadlock_zone(c);

	if (boxes_in_level(c) == global_boxes_in_level)
		return NOT_DEADLOCK; // will be explored by the search tree

	if (debug_corral_deadlock)
	{
		printf("the pull corral is:\n");
		print_board(c);
		my_getch();
	}


	res = advanced_deadlock_search(c, 1); // this also handles cache

	if ((res == NOT_DEADLOCK) || (res == SEARCH_EXCEEDED))
		return 0;

	if (work_left_outside_deadlock_zone(b) == 0) // can't exit the zone, but no need to
		return NOT_DEADLOCK; // note the usage of b rather than c

	return 1;
}

int corral_deadlock(board b_in, int pull_mode)
{
	int_board dist;
	int connect_num;
	int i, j;
	board c;
	int list[4];
	int list_size, res;
	int c1, c2, k1, k2;
	int_board checked;
	int y, x, sokoban_comp;
	int search_exceeded[MAX_SIZE * MAX_SIZE];
	board b;

	if (board_is_solved(b_in, pull_mode)) return 0;

	if (pull_mode)
	{
		res = advanced_deadlock_pull_mode(b_in, 0);
		return res;
	}

	turn_fixed_boxes_into_walls(b_in, b);

	connect_num = mark_connectivities(b, dist);

	if (connect_num >= MAX_SIZE) // too many corrals 
		return 0;

	for (i = 0; i < connect_num; i++)
		for (j = 0; j < connect_num; j++)
			checked[i][j] = 0;

	for (i = 0; i < connect_num; i++)
		search_exceeded[i] = 0;

	get_sokoban_position(b, &y, &x);
	sokoban_comp = dist[y][x];


	if (debug_corral_deadlock)
	{
		print_2d_array(dist);
		my_getch();
	}


	// first try one color

	for (c1 = 0; c1 < connect_num; c1++)
	{
		if (c1 == sokoban_comp) continue;

		if (debug_corral_deadlock)
			printf("checking %d\n", c1);

		checked[c1][c1] = 1;

		copy_board(b, c);
		keep_two_components(c, dist, c1, c1, pull_mode);
		clear_boxes_out_of_deadlock_zone(c);
		clear_frozen_boxes_away_from_deadlock_zone(c);


		if (debug_corral_deadlock)
		{
			print_board(c);
			my_getch();
		}

		res = advanced_deadlock_search(c, pull_mode);

		if (res == IS_DEADLOCK)
			return 1;

		if (res == SEARCH_EXCEEDED)
			search_exceeded[c1] = 1;

		res = mini_corral(c);
		if (res == IS_DEADLOCK) return res;
	}

	// check two componenents if they touch a box
	
	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			if ((b[i][j] & BOX) == 0)
				continue;

			list_size = get_components_around_place(i, j, dist, list);

			for (k1 = 0; k1 < list_size; k1++)
			{
				for (k2 = k1; k2 < list_size; k2++)
				{
					c1 = list[k1];
					c2 = list[k2];

					if (c1 == sokoban_comp) continue;
					if (c2 == sokoban_comp) continue;

					if (checked[c1][c2]) continue;

					if (search_exceeded[c1]) continue;
					if (search_exceeded[c2]) continue;

					if (debug_corral_deadlock)
						printf("checking %d %d ", c1, c2);
					checked[c1][c2] = 1;
					checked[c2][c1] = 1;

					copy_board(b, c);
					keep_two_components(c, dist, c1, c2, pull_mode);
					clear_boxes_out_of_deadlock_zone(c);
					clear_frozen_boxes_away_from_deadlock_zone(c);


					if (debug_corral_deadlock)
					{
						print_board(c);
						my_getch();
					}

					res = advanced_deadlock_search(c, pull_mode);

					if (res == IS_DEADLOCK)
						return 1;

					res = mini_corral(c);
					if (res == IS_DEADLOCK) return res;

				} // k2
			} // k1
		} // j
	} //i
	
	return 0;
}

