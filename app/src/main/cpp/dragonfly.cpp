#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "util.h"
#include "board.h"
#include "score.h"
#include "moves.h"
#include "match_distance.h"
#include "dragonfly.h"
#include "advanced_deadlock.h"
#include "perimeter.h"
#include "dragonfly_queue.h"
#include "io.h"
#include "bfs.h"
#include "corral.h"
#include "fixed_boxes.h"
#include "debug.h"

dragonfly_node* dragonfly_nodes;
int dragonfly_nodes_num;
int dragonfly_max_nodes;

#define MAX_DRAGONFLY_ROOTS 200

board dragonfly_roots[MAX_DRAGONFLY_ROOTS];
int dragonfly_roots_num = 0;

dragonfly_queue dragonfly_q ;

void init_dragonfly()
{
	size_t size;

	if (cores_num == 1) dragonfly_max_nodes = 1 << 22;
	if (cores_num == 2) dragonfly_max_nodes = 1 << 23;
	if (cores_num == 4) dragonfly_max_nodes = 1 << 24;
	if (cores_num == 8) dragonfly_max_nodes = 1 << 25;

	dragonfly_max_nodes *= (1 << extra_mem);

	size = dragonfly_max_nodes * sizeof(dragonfly_node);
	dragonfly_nodes = (dragonfly_node*)malloc(size);
	if (dragonfly_nodes == 0) exit_with_error("can't allocate nodes");

	if (verbose >= 4)
		printf("Allocating %12llu bytes for %12d dragonfly nodes\n", 
			(UINT_64)size, dragonfly_max_nodes);

	dragonfly_init_queue(&dragonfly_q, dragonfly_nodes);
}

	
int dist_from_bases(board b)
{
	move dummy_move;
	score_element base_score, dummy_score;

	compute_base_distance(b, &base_score, 0, &dummy_move, &dummy_score);
	return base_score.dist_to_targets;
}


void add_dragonfly_root(board b)
{
	dragonfly_node* e = dragonfly_nodes + dragonfly_nodes_num;

	copy_board(b, dragonfly_roots[dragonfly_roots_num]);
	dragonfly_roots_num++;

	if (dragonfly_roots_num >= MAX_DRAGONFLY_ROOTS) exit_with_error("too many roots");

	e->board = dragonfly_roots_num - 1;
	e->depth = 0;
	e->father = -1;
	e->move_from = -1;
	e->move_to = -1;
	e->player_position = 4;
	e->dist = dist_from_bases(b);
	e->packed = boxes_on_bases(b);
	dragonfly_nodes_num++;

	dragonfly_heap_insert(&dragonfly_q, dragonfly_nodes_num - 1);

	UINT_64 hash = get_board_hash(b);
	int visited = 0;
	dragonfly_update_perimeter(&hash, &visited, 1, 0);

}

void dragonfly_get_board(dragonfly_node* e, board b)
{
	move moves[MAX_SOL_LEN];
	int moves_num = 0;
	int i,y,x;
	int player = e->player_position;

	if (e->board != 0xff) // root
	{
		copy_board(dragonfly_roots[e->board], b);
		return;
	}

	while (e->board == 0xff) // not a root
	{
		if (moves_num >= MAX_SOL_LEN) exit_with_error("too deep");

		moves[moves_num].from = e->move_from;
		moves[moves_num].to   = e->move_to;
		moves_num++;
		e = dragonfly_nodes + e->father;

		if (moves_num >= MAX_SOL_LEN) exit_with_error("sol too long");
	}

	copy_board(dragonfly_roots[e->board], b);
	for (i = moves_num - 1; i >= 0; i--)
	{
		index_to_y_x(moves[i].from, &y, &x);
		if ((b[y][x] & BOX) == 0) exit_with_error("bad1");
		b[y][x] &= ~BOX;

		index_to_y_x(moves[i].to, &y, &x);
		if ((b[y][x] & OCCUPIED)) exit_with_error("bad2");
		b[y][x] |= BOX;
	}

	clear_sokoban_inplace(b);
	b[y + delta_y[player]][x + delta_x[player]] |= SOKOBAN;
	expand_sokoban_cloud(b);
}


void dragonfly_print_node(dragonfly_node* e)
{
	board b;

	dragonfly_get_board(e, b);

	//printf("hash= %016llx\n", get_board_hash(b));
	//	printf("node %d\n", (int)(e - dragonfly_nodes));
	
	print_board(b);

	if (verbose >= 4)
		print_with_bases(b);

	if (verbose >= 4)
	{
		printf("depth= %d\n", e->depth);
		printf("dist= %d packed= %d\n", e->dist, e->packed);
		printf("\n");
	}
}


void mark_moves_when_corral(board b, move* moves, int n, int last_move, int* possible)
{
	/*
	the objective of this function is to prune moves that pull boxes while
	there is an active corral on the board.
	Ideally we should have used "can_played_reversed" but it is quite heavy.
	Instead, we do the following:
	We first detect if position "A" has a corral.
	from "A" there is a pull move to position "B"
	If the reachability of corral boxes did not change in B, it means that B had
	an active corral; which was not dealt with when transitioning to "A", so this
	is a contradiction and the move is pruned.


	Some comments:
	The approach does not work when there are several active corrals on board.
	While dealing with one of them, other would seem to remain active and so all moves may
	be pruned. However, this situation is quite rare.

	For most levels, the extra corral-checking makes the program slower.
	For example, SokHard is solved in 72 minutes rather than 44 (some levels take 
	much longer because of the "double corral" issue).

	However, for many medium size levels using the corral pruning makes a huge difference.

	XSokoban     #29 : 1:38 vs 16:52
	Sven       #1722 : 38 sec vs unsolved (!)
	Grigr_2002   #27 : 7:38 vs 7:45
	
	*/

	int i, has_corral, y, x, j, delta;
	board corral_options,w,c;

	for (i = 0; i < n; i++)
		possible[i] = 1;

	if (get_connectivity(b) == 1) return;

	turn_fixed_boxes_into_walls(b, w);
	has_corral = detect_corral(w, corral_options);

	if (has_corral == 0) return;

	index_to_y_x(last_move, &y, &x);

	for (i = 0; i < n; i++)
	{
		if (moves[i].from == last_move)
		{
			possible[i] = 0; // another macro move will deal with it
			continue;
		}

		copy_board(b, c);
		apply_move(c, moves + i, NORMAL);

		if (board_is_solved(c, 1)) continue;

		for (j = 0; j < 4; j++)
		{
			delta  = c[y + delta_y[j]][x + delta_x[j]];
			delta ^= b[y + delta_y[j]][x + delta_x[j]];
			if (delta & SOKOBAN) break;
		}
		if (j == 4)
			possible[i] = 0;
		
	}
}



int dragonfly_expand_node(dragonfly_node* e, helper* h)
{
	board b, c;
	move m;
	int i, moves_num;
	int has_corral;
	score_element base_score;
	dragonfly_node new_node, * father;

	// using static - only one instance of dragonfly
	static move moves[MAX_MOVES];
	static UINT_64 hashes[MAX_MOVES];
	static int visited[MAX_MOVES];
	static score_element scores[MAX_MOVES];
	static int packed[MAX_MOVES];
	static int possible[MAX_MOVES];

	if (verbose >= 5)
	{
		dragonfly_get_board(e, b);
		printf("Expanding node:\n");
		print_board(b);
	}

	if (e->depth >= MAX_SOL_LEN) return 0;

	if (e->board == 0xff) // not a root
	{
		father = dragonfly_nodes + e->father;

		dragonfly_get_board(father, b);

		m.from = e->move_from;
		m.to = e->move_to;
		m.sokoban_position = e->player_position;
		m.kill = 0;
		m.base = 0;

		if (move_is_deadlocked(b, &m, 1, DRAGONFLY)) // 1 - pull mode
		{
			if (verbose >= 5) printf("node is deadlocked\n");
			return 0;
		}
	}
	
	// so the node seems to be valid

	dragonfly_get_board(e, b);

	moves_num = find_possible_moves(b, moves, 1, &has_corral, NORMAL, h);

	if (moves_num == 0) return 0;

	mark_moves_when_corral(b, moves, moves_num, e->move_to, possible);

	for (i = 0; i < moves_num; i++)
	{
		copy_board(b, c);
		apply_move(c, moves + i, NORMAL);
		hashes[i] = get_board_hash(c);
		packed[i] = boxes_on_bases(c);
	}

	dragonfly_mark_visited_hashes(hashes, moves_num, visited);

	compute_base_distance(b, &base_score, moves_num, moves, scores);

	if (e->dist >= 0)
		if (base_score.dist_to_targets != e->dist) exit_with_error("dist bug");

	for (i = 0; i < moves_num; i++)
	{
		if (visited[i] == 1) continue; // already in dragonfly nodes
		
		if (possible[i] == 0)
		{
			if (verbose >= 5)
			{
				printf("rejecting corral move ");
				print_move(moves + i);
				printf("\n");
			}

			visited[i] = 1; // do not insert this move to the list of explored nodes
			continue;
		}

		if (scores[i].dist_to_targets >= 1000000) // match deadlock with bases
		{
			visited[i] = 1;
			continue;
		}

		new_node.board = 0xff;
		new_node.depth = e->depth + 1;
		new_node.dist = scores[i].dist_to_targets;
		new_node.father = (int)(e - dragonfly_nodes);
		new_node.move_from = moves[i].from;
		new_node.move_to = moves[i].to;
		new_node.player_position = moves[i].sokoban_position;
		new_node.packed = packed[i];

		if (visited[i] < 0)
		{
			if (h->perimeter_found == 0)
			{
				h->perimeter_found = 1;

				if (verbose >= 4)
				{
					print_in_color("< Perimeter hit!!!>\n", "blue");
					dragonfly_print_node(e);
				}
			}
			new_node.dist = visited[i]; // make dist an attractive negative value
		}

		dragonfly_nodes[dragonfly_nodes_num++] = new_node;

		dragonfly_heap_insert(&dragonfly_q, dragonfly_nodes_num - 1);
	}

	dragonfly_update_perimeter(hashes, visited, moves_num, e->depth + 1);

	return 1;
}

int compute_start_zones(int_board start_zones);

int dragonfly_node_is_solved(dragonfly_node* e)
{
	board b;

	if (e->dist > 0) return 0;

	dragonfly_get_board(e, b);

	return board_is_solved(b, 1);
}

void dragonfly_show_path(dragonfly_node* e)
{
	board b;

	while (e->board == 0xff)
	{
		dragonfly_get_board(e, b);
		print_board(b);
		e = dragonfly_nodes + e->father;
	}
}


void dragonfly_search(board b_in, int time_allocation, helper *h)
{
	// TODO : time allocation, report nodes

	int start_zones_num;
	int_board start_zones;
	int i, node_index, res;
	board b,c;
	dragonfly_node* e, *best_so_far;
	int iter_num = 0;
	int start_time = (int)time(0);


	dragonfly_reset_heap(&dragonfly_q);
	dragonfly_nodes_num = 0;
	dragonfly_roots_num = 0;

	h->perimeter_found = 0;

	copy_board(b_in, b);
	enter_reverse_mode(b, 0);

	start_zones_num = compute_start_zones(start_zones);

	for (i = 0; i < start_zones_num; i++)
	{
		copy_board(b, c);
		put_sokoban_in_zone(c, start_zones, i);
		add_dragonfly_root(c);
	}

	best_so_far = dragonfly_nodes;

	while (1)
	{
		iter_num++;

		if (any_core_solved) break;
		
		if ((int)time(0) > (start_time + time_allocation))
		{
			if (verbose >= 4) printf("dragonfly time exceeded. %d nodes\n", dragonfly_nodes_num);
			break;
		}
		
		node_index = dragonfly_extarct_max(&dragonfly_q);

		if (node_index == -1)
		{
			if (verbose >= 4) printf("no more nodes\n");
			break;
		}

		e = dragonfly_nodes + node_index;
		
		

		if (dragonfly_node_is_solved(e))
		{
			if (verbose >= 4)
			{
				dragonfly_print_node(e);
				printf("%d nodes\n\n", dragonfly_nodes_num);
				printf("\ndragonfly SOLVED!\n\n");
				//	dragonfly_show_path(e);
			}
			break;
		}

		res = dragonfly_expand_node(e, h);

		// update best state if the node seems to be valid
		if ((res == 1) && (e->dist < best_so_far->dist) && (e->dist >= 0))
			best_so_far = e;

		show_progress_dragonfly(best_so_far, iter_num, h);

//		if (((iter_num % 9999) == 0) && (res == 1))
//			dragonfly_print_node(e);		

		if (dragonfly_nodes_num >= (dragonfly_max_nodes - MAX_MOVES))
		{
			if (verbose >= 4) printf("dragonfly max nodes reached\n");
			break;
		}
		
		if (perimeter_is_full())
		{
			if (verbose >= 4) printf("perimeter is full\n");
			break;
		}
	}

	dragonfly_reset_heap(&dragonfly_q);
}
