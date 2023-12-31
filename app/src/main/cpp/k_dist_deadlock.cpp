#include <stdlib.h>
#include <stdio.h>

#ifdef THREADS
#include <pthread.h>
#endif

#include "k_dist_deadlock.h"
#include "fixed_boxes.h"
#include "util.h"
#include "positions.h"
#include "bfs.h"
#include "distance.h"
#include "rooms.h"
#include "holes.h"

#define K_DIST_MAX_POSITIONS 600

int debug_k_dist_deadlock = 0;

typedef struct k_dist_entry
{
	UINT_64 hash;
	board b;
	board forced;
	board adj_4; // lower bound on the number of adjacent boxes
	board adj_9;
} k_dist_entry;

#define MAX_K_DIST_HASH_SIZE 500

k_dist_entry k_dist_hash[MAX_K_DIST_HASH_SIZE];

int k_dist_hash_num;

void clear_k_dist_hash()
{
	k_dist_hash_num = 0;
}

int find_in_k_dist_hash(UINT_64 hash)
{
	int i;

	for (i = 0; i < k_dist_hash_num; i++)
		if (k_dist_hash[i].hash == hash)
			return i;
	return -1;
}

void display_adj(board b, board value)
{
	int i, j;

	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			if (b[i][j] == WALL)
			{
				printf("#");
				continue;
			}
			if (value[i][j] == 0)
				printf(" ");
			else
				printf("%d", value[i][j]);
		}
		printf("\n");
	}
}


#ifdef THREADS
pthread_mutex_t k_dist_insert_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

void insert_to_k_dist_hash(UINT_64 hash, board w, board forced, board adj_4, board adj_9)
{
	int i, j;
	int n = k_dist_hash_num;

	if (n >= MAX_K_DIST_HASH_SIZE)
		return;

#ifdef THREADS
	if (cores_num > 1)
		pthread_mutex_lock(&k_dist_insert_mutex);
#endif

	k_dist_hash[n].hash = hash;
	copy_board(w, k_dist_hash[n].b);
	copy_board(forced, k_dist_hash[n].forced);
	copy_board(adj_4, k_dist_hash[n].adj_4);
	copy_board(adj_9, k_dist_hash[n].adj_9);

	if (debug_k_dist_deadlock)
	{
		printf("forced is:\n");
		show_on_initial_board(forced);

		printf("adj_4:\n");
		display_adj(w, adj_4);
		printf("adj_9:\n");
		display_adj(w, adj_9);
	}

	if ((board_popcnt(forced) > 0) && (verbose >= 4))
	{
		printf("\nAdded k-dist entry\n");
		for (i = 0; i < height; i++)
			for (j = 0; j < width; j++)
				if (forced[i][j])
					w[i][j] |= DEADLOCK_ZONE; // we don't use w later
		print_board(w);
	}

	k_dist_hash_num++;

#ifdef THREADS
	if (cores_num > 1)
		pthread_mutex_unlock(&k_dist_insert_mutex);
#endif

}

int sokoban_touches_a_filled_hole(board b)
{
	int sok_y, sok_x, y,x,i;
	// this is used when the sokoban cloud is of size 1.
	// although the player seems surrounded, a pull move might be possible!
	// if one of the surrounding walls is a filled garage
	get_sokoban_position(b, &sok_y, &sok_x);

	for (i = 0; i < 4; i++)
	{
		y = sok_y + delta_y[i];
		x = sok_x + delta_x[i];
		if (b[y][x] == WALL)
			if (initial_board[y][x] != WALL)
				return 1;
	}
	return 0;
}

void count_adjacent_boxes(board b, board adj_4, board adj_9)
{
	// count how many boxes are around each place
	int i,j,k,y,x;

	zero_board(adj_4);
	zero_board(adj_9);

	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			if ((b[i][j] & BOX) == 0) continue;

			adj_4[i][j]++;
			adj_9[i][j]++;

			for (k = 0; k < 8; k++)
			{
				y = i + delta_y_8[k];
				x = j + delta_x_8[k];

				adj_9[y][x]++;
				if ((k & 1) == 0)
					adj_4[y][x]++;
			}
		}
	}
}

int is_better_k_dist_score(score_element* new_score, score_element* old_score)
{
	if (new_score->boxes_in_level < old_score->boxes_in_level) return 1;
	if (new_score->boxes_in_level > old_score->boxes_in_level) return 0;

	if (new_score->connectivity < old_score->connectivity) return 1;
	if (new_score->connectivity > old_score->connectivity) return 0;

	return 0;
}

void get_k_dist_pos_and_son(position *positions, int pos_num, int *pos, int *son)
{
	// return the move with the best connectivity
	int i, j;
	*pos = -1;

	score_element best_score;
	

	for (i = 0; i < pos_num; i++)
	{
		if (positions[i].position_deadlocked) continue;

		for (j = 0; j < positions[i].moves_num; j++)
		{
			if (positions[i].move_deadlocked[j]) continue;
			if (positions[i].place_in_positions[j]) continue;

			if ((*pos == -1) || (is_better_score(positions[i].scores + j, &best_score, 1, K_DIST_SEARCH)))
			{
				best_score = positions[i].scores[j];

				*pos = i;
				*son = j;
			}
		}
	}
}

int check_for_k_dist_moves(board b, move *moves, int moves_num, helper *h)
{
	int i, from, to;
	int y, x;
	board c;

	for (i = 0; i < moves_num; i++)
	{
		from = moves[i].from;
		to = moves[i].to;

		if (bfs_distance_from_to[to][from] >= h->k_dist_value)
		{
			// allow a killing move only if the player can return to the pulled box 
			// (not in a corral)
			copy_board(b, c);
			apply_move(c, moves + i, NORMAL);
			index_to_y_x(from, &y, &x);

			if (c[y][x] & SOKOBAN)
			{
				moves[0] = moves[i];
				moves[0].kill = 1;
				return 1;
			}
		}
	}
	return moves_num;
}

int get_k_dist_forced_boxes(board b_in, board forced_boxes, board adj_4, board adj_9, int k_dist_val)
{
	board b,c;
	int_board z;
	int i, j;
	position *positions, *new_pos;
	int pos_num;
	int start_zones_num;
	int pos, son;
	int removed_all = 0;
	board local_adj_4;
	board local_adj_9;
	int search_terminated;
	helper h;

	init_helper(&h);

	h.k_dist_value = k_dist_val;

	// go to final position
	copy_board(b_in, b);
	go_to_final_position_with_given_walls(b);

	positions = allocate_positions(K_DIST_MAX_POSITIONS);

	start_zones_num = mark_connectivities(b, z);

	pos_num = 0;
	for (i = 0; i < start_zones_num; i++)
	{
		copy_board(b, c);

		put_sokoban_in_zone(c, z, i);

		set_first_position(c, 1, positions + pos_num, K_DIST_SEARCH, &h); // 1 = pull mode

		pos_num++;
	}

	for (i = 0; i < pos_num; i++)
		direct_other_sons_to_position(positions + i, positions, pos_num);

	while (pos_num < K_DIST_MAX_POSITIONS)
	{
		get_k_dist_pos_and_son(positions, pos_num, &pos, &son);

		if (pos == -1)
			break;

		if (debug_k_dist_deadlock)
		{
			printf("\nposition to expand:\n");
			print_position(positions + pos, 1);
			printf("with move: ");
			print_move(positions[pos].moves + son);
			printf("\n");
		}

		expand_position(positions + pos, son, 1, positions, pos_num, &h); // 1 = pull_mode
		pos_num++;

		new_pos = positions + pos_num - 1;

		if (debug_k_dist_deadlock)
		{
			printf("\nposition after expansion:\n");
			print_position(new_pos, 1);
			my_getch();
		}

		if (new_pos->position_score.boxes_in_level == 0)
		{
			removed_all = 1;
			if (debug_k_dist_deadlock)
			{
				printf("removed all boxes\n");
				my_getch();
			}
			break;
		}

	}

	zero_board(forced_boxes);
	zero_board(adj_4);
	zero_board(adj_9);

	search_terminated = (pos_num >= K_DIST_MAX_POSITIONS ? 1 : 0);

	if (search_terminated || removed_all)
	{
		free_positions(positions, pos_num);
		return search_terminated;
	}

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			forced_boxes[i][j] = 1;
			adj_4[i][j] = 10;
			adj_9[i][j] = 10;
		}

	for (pos = 0; pos < pos_num; pos++)
	{
		bytes_to_board(positions[pos].board, b);
		
		count_adjacent_boxes(b, local_adj_4, local_adj_9);

		for (i = 0; i < height; i++)
			for (j = 0; j < width; j++)
			{
				if ((b[i][j] & BOX) == 0)
					forced_boxes[i][j] = 0;

				if (local_adj_4[i][j] < adj_4[i][j])
					adj_4[i][j] = local_adj_4[i][j];

				if (local_adj_9[i][j] < adj_9[i][j])
					adj_9[i][j] = local_adj_9[i][j];
			}

	}

	free_positions(positions, pos_num);

	return search_terminated;
}

int check_deadlock_using_k_dist_cache(board b)
{
	UINT_64 hash;
	int i,j,n;
	board forced, adj_4, adj_9, b_wallified, w;
	int search_terminated;

	turn_fixed_boxes_into_walls(b, b_wallified);

	turn_target_holes_into_walls(b_wallified);
	// there could be several holes, resulting in an exponential number of k-dist cases.
	// But blocking a hole should not interfere with the pulls we'll make. 

	// get the canonical hash of wallified board, without boxes and sokoban
	clear_boxes(b_wallified, w);
	clear_sokoban_inplace(w);

	hash = get_board_hash(w);

	n = find_in_k_dist_hash(hash);

	if (n == -1) // currently not in hash
	{
		if (debug_k_dist_deadlock)
		{
			printf("\nAdding k-dist entry\n");
			print_board(w);
		}

		search_terminated = get_k_dist_forced_boxes(w, forced, adj_4, adj_9, 5);
		if (search_terminated)
		{
			// run a more aggressive box removal
			get_k_dist_forced_boxes(w, forced, adj_4, adj_9, 3);
		}

		insert_to_k_dist_hash(hash, w, forced, adj_4, adj_9);
	}

	// now check if b matches the expected forced boxes/adjacent counts

	n = find_in_k_dist_hash(hash);

	if (n == -1) // probably table is full
		return 0;
		
	if (debug_k_dist_deadlock)
		printf("found k-dist in hash\n");

	count_adjacent_boxes(b_wallified, adj_4, adj_9);

	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			if (k_dist_hash[n].forced[i][j])
				if ((b[i][j] & BOX) == 0)
				{
					if (debug_k_dist_deadlock)
					{
						printf("found deadlock in hash\n");
						print_board(b);
						show_on_initial_board(k_dist_hash[n].forced);
					}
					return 1;
				}

			if (adj_4[i][j] < k_dist_hash[n].adj_4[i][j])
			{
				if (debug_k_dist_deadlock)
				{
					printf("\nfound adj-4 deadlock in hash\n");
					display_adj(w, k_dist_hash[n].adj_4);
					print_board(b);
				}
				return 1;
			}

			if (adj_9[i][j] < k_dist_hash[n].adj_9[i][j])
			{
				if (debug_k_dist_deadlock)
				{
					printf("\nfound adj-9 deadlock in hash\n");
					display_adj(w, k_dist_hash[n].adj_9);
					print_board(b);
				}
				return 1;
			}
		}
	}
	
	return 0;
}


int is_k_dist_deadlock(board b)
{
	int res;

	if (all_boxes_in_place(b, 0))
		return 0;

	res = check_deadlock_using_k_dist_cache(b);

	return res;

}

void clear_k_dist_boxes(board b, int pull_mode, int k_dist_val)
{
	int moves_num,y,x;
	int corral;
	move moves[MAX_MOVES];

	helper h;
	init_helper(&h);

	h.k_dist_value = k_dist_val;

	while (1)
	{
		moves_num = find_possible_moves(b, moves, pull_mode, &corral, K_DIST_SEARCH, &h);

		if ((moves_num > 0) && (moves[0].kill))
		{
			index_to_y_x(moves[0].from, &y, &x);
			b[y][x] &= ~BOX;
			expand_sokoban_cloud(b);
		}
		else
			return;
	}
}

int actual_k_dist_search(board b, int pull_mode, int k_dist_val, int search_size)
{
	board c;
	position* positions, *new_pos;
	int pos_num, pos, son;
	int solved = 0, search_terminated = 0;

	helper h;
	init_helper(&h);

	h.k_dist_value = k_dist_val;

	positions = allocate_positions(search_size);

	set_first_position(b, pull_mode, positions, K_DIST_SEARCH, &h); 
	pos_num = 1;

	while (pos_num < search_size)
	{
		get_k_dist_pos_and_son(positions, pos_num, &pos, &son);

		if (pos == -1)
			break;

		if (debug_k_dist_deadlock)
		{
			printf("\nposition to expand:\n");
			print_position(positions + pos, 1);
			printf("with move: ");
			print_move(positions[pos].moves + son);
			printf("\n");
		}

		expand_position(positions + pos, son, pull_mode, positions, pos_num, &h); 
		pos_num++;

		new_pos = positions + pos_num - 1;

		if (debug_k_dist_deadlock)
		{
			printf("\nposition after expansion:\n");
			print_position(new_pos, 1);
			my_getch();
		}

		bytes_to_board(new_pos->board, c);

		if (all_boxes_in_place(c, pull_mode))
		{
			solved = 1;
			if (debug_k_dist_deadlock)
			{
				printf("solved\n");
				my_getch();
			}
			break;
		}
	}

	search_terminated = (pos_num >= search_size ? 1 : 0);

	free_positions(positions, pos_num);

	if (solved) return 0;
	if (search_terminated) return 2;

	return 1;
}
