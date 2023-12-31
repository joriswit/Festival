// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include <stdlib.h>
#include <stdio.h>

#include "heuristics.h"
#include "graph.h"
#include "util.h"
#include "advanced_deadlock.h"
#include "rooms.h"
#include "distance.h"
#include "holes.h"
#include "bfs.h"
#include "rooms_deadlock.h"

#define BIG_DISTANCE 6

int boxes_in_different_rooms(int index1, int index2)
{
	int box1_x, box1_y, box2_x, box2_y;
	int index1_has_room = 0;
	int index2_has_room = 0;
	int r1, r2;

	extern board room_with_corridors[MAX_ROOMS];

	if (bfs_distance_from_to[index1][index2] >= BIG_DISTANCE)
		return 1;

	index_to_y_x(index1, &box1_y, &box1_x);
	index_to_y_x(index2, &box2_y, &box2_x);

	r1 = rooms_board[box1_y][box1_x];
	r2 = rooms_board[box2_y][box2_x];

	if ((r1 != 1000000) && (r2 != 1000000))
		if (r1 != r2)
			return 1;

	if (r1 != r2)
	{
		if (r1 != 1000000)
			if (room_with_corridors[r1][box2_y][box2_x])
				return 0;

		if (r2 != 1000000)
			if (room_with_corridors[r2][box1_y][box1_x])
				return 0;

		if ((box1_y != box2_y) && (box1_x != box2_x))
			return 1;
	}
	
	return 0;
}


void get_kill_zone(board bases, board kill_zone)
{
	int bases_list[MAX_INNER];
	int bases_list_num = 0;
	int i, j, y, x;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (bases[i][j])
				bases_list[bases_list_num++] = y_x_to_index(i, j);

	zero_board(kill_zone);

	for (i = 0; i < index_num; i++)
	{
		// check if place i can be a kill square

		for (j = 0; j < bases_list_num; j++)
		{
			if (bfs_distance_from_to[i][bases_list[j]] < BIG_DISTANCE)
				break;
		}

		if (j == bases_list_num)
		{
			index_to_y_x(i, &y, &x);
			kill_zone[y][x] = 1;
			break;
		}
	}
}



void get_bases_as_board(board b, board bases)
{
	int i, j;
	zero_board(bases);
	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (b[i][j] & BASE)
				bases[i][j] = 1;
}



void compute_possible_from_bases(board b, board possible_from_bases)
{
	// this routine is used by the moves generator.
	// A move that puts a box in a place unreachable from bases should be pruned.

	board bases;
	board board_without_boxes;

	clear_boxes(initial_board, board_without_boxes);
	get_bases_as_board(b, bases);
	find_targets_of_a_board_input(board_without_boxes, bases, possible_from_bases);
}



int test_base_matching(board b, int from, int to)
{
	int from_y, from_x, to_y, to_x;
	board c;
	int res;

	index_to_y_x(from, &from_y, &from_x);
	index_to_y_x(to, &to_y, &to_x);

	if ((b[from_y][from_x] & BOX) == 0) exit_with_error("no box");
	if ((b[to_y][to_x] & BASE) == 0) exit_with_error("no base in matching");

	copy_board(b, c);
	c[from_y][from_x] &= ~BOX;
	c[to_y][to_x] &= ~BASE;

	res = check_matching_deadlock(c, 1, BASE_SEARCH); // 1 = pull mode

	if (res == 1)
	{
		//		printf("detected pull match deadlock\n");
		//		print_board(c);
		//		my_getch();
		return 0;
	}

	return 1;
}

int unreachable_area_contains_elim(board b, board elim)
{
	int i, j;
	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if (elim[i][j])
				if ((b[i][j] & SOKOBAN) == 0)
					return 1;
		}
	return 0;
}



void compute_bases_scc(board b_in, board scc_board)
{
	box_mat mat;
	int bases_num = 0;
	int i, j, y, x;
	int bases[MAX_BOXES];
	int scc[MAX_BOXES];
	int scc_size[MAX_BOXES];
	board b;
	board targets;
	int current_scc;
	int max_size;

	graph_data *gd;

	copy_board(b_in, b);
	turn_boxes_into_walls(b);

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (b[i][j] & BASE)
				bases[bases_num++] = y_x_to_index(i, j);

	for (i = 0; i < bases_num; i++)
		for (j = 0; j < bases_num; j++)
			mat[i][j] = 0;


	gd = allocate_graph();
	build_graph(b, 0, gd); // 0 - push mode

	for (i = 0; i < bases_num; i++)
	{
		index_to_y_x(bases[i], &y, &x);

		find_targets_of_place_without_init_graph(y, x, targets, gd);

		if (targets[y][x] == 0) exit_with_error("no self");

		//	show_on_initial_board(targets);
		//	my_getch();

		for (j = 0; j < bases_num; j++)
		{
			index_to_y_x(bases[j], &y, &x);
			if (targets[y][x])
				mat[i][j] = 1;
		}
	}
	free(gd);

	for (i = 0; i < bases_num; i++)
		if (mat[i][i] != 1)
			exit_with_error("bad diag");

	//	for (i = 0; i < bases_num; i++)
	//	{
	//		for (j = 0; j < bases_num; j++)
	//			printf("%d", mat[i][j]);
	//		printf("\n");
	//	}


	// compute the scc

	for (i = 0; i < bases_num; i++)
		scc[i] = -1;

	current_scc = 0;
	for (i = 0; i < bases_num; i++)
	{
		if (scc[i] != -1) continue; // node already associated with an SCC

		scc[i] = current_scc;

		for (j = i; j < bases_num; j++)
		{
			if (mat[i][j] & mat[j][i])
				scc[j] = current_scc;
		}

		current_scc++;
	}

	//	for (i = 0; i < bases_num; i++)
	//		printf("%d ", scc[i]);
	//	printf("\n");


	// find the biggest scc

	for (i = 0; i < bases_num; i++)
	{
		scc_size[i] = 0;
		for (j = i; j < bases_num; j++)
			if (scc[j] == scc[i])
				scc_size[i]++;
	}

	max_size = 0;
	for (i = 0; i < bases_num; i++)
		if (scc_size[i] > max_size)
			max_size = scc_size[i];

	zero_board(scc_board);

	for (i = 0; i < bases_num; i++)
	{
		if (scc_size[i] != max_size) continue;

		for (j = i; j < bases_num; j++)
		{
			if (scc[j] == scc[i])
			{
				index_to_y_x(bases[j], &y, &x);
				scc_board[y][x] = 1;
			}
		}
	}
}

int mark_sink_squares_moves(board b, move *moves, int moves_num)
{
	int i;
	int has_base_move = 0;
	move tmp[MAX_MOVES];
	int n = 0;
	board elim;
	int move_tested[MAX_MOVES];

	int from_y, from_x, to_y, to_x;

	compute_bases_scc(b, elim);

	if (unreachable_area_contains_elim(b, elim))
		return moves_num;

//	printf("ELIM:\n");
//	show_on_initial_board(elim);
//	my_getch();

	// first step: find if there is a sink move. 

	for (i = 0; i < moves_num; i++)
		move_tested[i] = 0;

	for (i = 0; i < moves_num; i++)
	{
		move_tested[i] = 1;

		index_to_y_x(moves[i].from, &from_y, &from_x);
		index_to_y_x(moves[i].to, &to_y, &to_x);

		if (elim[to_y][to_x] == 0) continue;

		if (boxes_in_different_rooms(moves[i].from, moves[i].to) == 0) continue;
		if (test_base_matching(b, moves[i].from, moves[i].to) == 0) continue;

		moves[i].base = 1;
		has_base_move = 1;
		break;
	}

	if (has_base_move == 0)
	{
		for (i = 0; i < moves_num; i++)
			moves[i].base = 0;
		return moves_num;
	}

	// second step: find only one legitimate sink move for each box
	// keep only eliminating moves. keep just one source 

	int box_was_pulled[MAX_INNER];
	for (i = 0; i < index_num; i++)
		box_was_pulled[i] = 0;

	for (i = 0; i < moves_num; i++)
	{
		if (move_tested[i])
			if (moves[i].base == 0) continue; // already checked and found bad

		if (moves[i].base == 1) // already tested and found good
		{
			tmp[n++] = moves[i];
			box_was_pulled[moves[i].from] = 1;
			continue;
		}

		if (box_was_pulled[moves[i].from]) continue;

		index_to_y_x(moves[i].from, &from_y, &from_x);
		index_to_y_x(moves[i].to, &to_y, &to_x);

		if (elim[to_y][to_x] == 0) continue;
		if (boxes_in_different_rooms(moves[i].from, moves[i].to) == 0) continue;

		// this test is the heaviest, so we keep it last
		if (test_base_matching(b, moves[i].from, moves[i].to) == 0) continue;

		moves[i].base = 1;
		tmp[n++] = moves[i];
		box_was_pulled[moves[i].from] = 1;
	}
	for (i = 0; i < n; i++)
		moves[i] = tmp[i];
	return n;

}


typedef struct pull_candidate
{
	int kill_zone;
	int connectivity;
	int room_connectivity;
	int pull_len;
	int target_hole;
} pull_candidate;


int is_better_pull_candidate(pull_candidate *old_opt, pull_candidate *new_opt)
{
	if (new_opt->target_hole > old_opt->target_hole) return 0;
	if (new_opt->target_hole < old_opt->target_hole) return 1;

	if (new_opt->kill_zone > old_opt->kill_zone) return 1;
	if (new_opt->kill_zone < old_opt->kill_zone) return 0;

	if (new_opt->connectivity < old_opt->connectivity) return 1;
	if (new_opt->connectivity > old_opt->connectivity) return 0;

	if (new_opt->room_connectivity < old_opt->room_connectivity) return 1;
	if (new_opt->room_connectivity > old_opt->room_connectivity) return 0;

	if (new_opt->pull_len > old_opt->pull_len) return 1;
	if (new_opt->pull_len < old_opt->pull_len) return 0;

	return 0;
}

int pick_pull_advisor_normal(board b_in, score_element *base_score,
	int moves_num, move *moves, score_element *scores, int search_mode)

{
	board b;
	int i;
	int best_move = -1;
	int from_y, from_x, to_y, to_x;
	board kill_zone, scc, w, targets;
	pull_candidate best_pull, current_pull;

	// there are no base moves. This routine tries to:
	// 1. pull a box to a different room than the one(s) with the scc
	// 2. If that's not possible, make a reasonable pull.

	copy_board(b_in, b);

	// compute a zone in which boxes can be killed - in the next move

	if (search_mode == BASE_SEARCH)
	{
		compute_bases_scc(b, scc);
		get_kill_zone(scc, kill_zone); // a zone for which SCC squares are in other rooms

		copy_board(b, w);
		turn_boxes_into_walls(w);
		find_targets_of_a_board_input(w, scc, targets); // zone should be reachable by scc

		and_board(kill_zone, targets);

		if (verbose >= 6)
		{
			printf("scc:\n");
			show_on_initial_board(scc);

			printf("targets:\n");
			show_on_initial_board(targets);

			printf("kill zone:\n");
			show_on_initial_board(kill_zone);
		}
	}
	else // no killing is possible anyway
		zero_board(kill_zone);

	best_pull.target_hole = 2; // anything is better than that
	best_pull.kill_zone = -1; 
	best_pull.connectivity = 0; // not needed, just to avoid compiler warnings
	best_pull.room_connectivity = 0;
	best_pull.pull_len = 0;

	for (i = 0; i < moves_num; i++)
	{
		if (scores[i].connectivity > base_score->connectivity)	continue;
		if (scores[i].rooms_score  > base_score->rooms_score)	continue;
		if (scores[i].dist_to_targets <= base_score->dist_to_targets) continue;

		index_to_y_x(moves[i].from, &from_y, &from_x);
		index_to_y_x(moves[i].to, &to_y, &to_x);
		if ((b[from_y][from_x] & TARGET) == 0) continue;

		current_pull.kill_zone = kill_zone[to_y][to_x];
		current_pull.connectivity = scores[i].connectivity;
		current_pull.room_connectivity = scores[i].rooms_score;
		current_pull.pull_len = distance_from_to[moves[i].to][moves[i].from];
		current_pull.target_hole = target_holes[from_y][from_x];

		if (is_better_pull_candidate(&best_pull, &current_pull))
		{
			best_move = i;
			best_pull = current_pull;
		}
	}

	return best_move;
}

typedef struct
{
	int hole;
	int connectivity;
	int dist;
} base_priority;

int is_better_base_prioirity(base_priority *new_bp, base_priority *old_bp)
{
	if (new_bp->hole < old_bp->hole) return 1;
	if (new_bp->hole > old_bp->hole) return 0;

	if (new_bp->connectivity < old_bp->connectivity) return 1;
	if (new_bp->connectivity > old_bp->connectivity) return 0;

	if (new_bp->dist > old_bp->dist) return 1;
	if (new_bp->dist < old_bp->dist) return 0;

	return 0;
}

int connectivity_after_move(board b_in, move *m)
{
	board b;
	int y, x;

	copy_board(b_in, b);

	index_to_y_x(m->from, &y, &x);
	b[y][x] &= ~BOX;

	index_to_y_x(m->to, &y, &x);
	b[y][x] |= BOX;

	return get_connectivity(b);
}


int find_best_base_pull(board b_in, score_element *base_score,
	int moves_num, move *moves)
{
	int i, y, x;
	int best_move = -1;
	base_priority best_pull, current_pull;
	board b;

	current_pull.hole = 2; // everything is better than that

	copy_board(b_in, b);


	// find a floater box
	for (i = 0; i < moves_num; i++)
	{
		if (moves[i].base == 0) continue;

		index_to_y_x(moves[i].from, &y, &x);
		if (b[y][x] == BOX)
			return i;
	}

	// so there is no floater. prioritize pulls

	best_move = -1;
	for (i = 0; i < moves_num; i++)
	{
		if (moves[i].base == 0) continue;

		index_to_y_x(moves[i].from, &y, &x);
		current_pull.hole = target_holes[y][x];

		current_pull.connectivity = connectivity_after_move(b, moves + i);

		index_to_y_x(moves[i].to, &y, &x);
		current_pull.dist = bfs_distance_from_to[moves[i].to][moves[i].from];


		if ((best_move == -1) || (is_better_base_prioirity(&current_pull, &best_pull)))
		{
			best_move = i;
			best_pull = current_pull;
		}
	}

	if (best_move == -1) exit_with_error("base bug\n");

	return best_move;
}


int packing_search_advisor(board b, score_element *base_score,
	int moves_num, move *moves, score_element *scores, int search_mode)
{
	int i;
	int has_elim = 0;

	for (i = 0; i < moves_num; i++)
		has_elim |= moves[i].base;

	if (has_elim == 0)
	{
		return pick_pull_advisor_normal(b, base_score, moves_num, moves, scores, search_mode);
	}

	// so find best eliminating move
	return find_best_base_pull(b, base_score, moves_num, moves);
}
