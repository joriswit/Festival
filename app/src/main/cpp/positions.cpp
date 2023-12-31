// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include <stdlib.h>
#include <stdio.h>

#include "positions.h"
#include "util.h"
#include "deadlock_utils.h"


position *allocate_positions(int pos_num)
{
	position *p;
	p = (position*)malloc(sizeof(position)* pos_num);

	if (p == 0)
		exit_with_error("could not allocate positions");

	
	for (int i = 0; i < pos_num; i++)
	{
		p[i].board = 0;
		p[i].moves_num = 0; // so we can free it later with no problems
	}

	return p;
}


int position_is_solved(position *p)
{
	board b;
	bytes_to_board(p->board, b);

	if ((p->search_mode == DEADLOCK_SEARCH) && (p->pull_mode == 0))
		return work_left_in_deadlock_zone(b) ^ 1; // also check that targets are occupied

	if ((p->search_mode == DEADLOCK_SEARCH) && (p->pull_mode))
		return 0; // in this mode we only check if we can exit/enlarge the deadlock zone.
	// other routines will verify if there is work left outside the deadlock zone.


	return board_is_solved(b, p->pull_mode);
}

void mark_deadlocks(position *positions, int pos_num)
{
	int changed;
	int i, j;
	position *t;

	do
	{
		changed = 0;

		// a son became deadlocked ?
		for (i = 0; i < pos_num; i++)
		{

			if (positions[i].position_deadlocked) // already detected
				continue;

			// a son became deadlocked?
			for (j = 0; j < positions[i].moves_num; j++)
			{
				t = positions[i].place_in_positions[j];
				if (t != 0)
				if (positions[i].move_deadlocked[j] == 0)
				if (t->position_deadlocked)
				{
					positions[i].move_deadlocked[j] = 1;
					changed = 1;

					if (verbose >= 6)
					{
						printf("discovered a deadlocked son (%d)\n", j);
						print_position(positions + i, 1);
					}					
				}
			}

			// all sons became deadlocked?
			for (j = 0; j < positions[i].moves_num; j++)
			if (positions[i].move_deadlocked[j] == 0)
				break;

			if (j == positions[i].moves_num)
			{
				if (verbose >= 6)
				{
					printf("all sons became deadlocked\n");
					print_position(positions + i, 1);
				}

				if (position_is_solved(positions + i) == 0)
				{
					positions[i].position_deadlocked = 1;
					changed = 1;
				}
			}
		}

	} while (changed == 1);
}



void print_position(position *p, int print_moves)
{
	board c;
	int moves_num;
	int i;

	bytes_to_board(p->board, c);
	print_board(c);

	if (verbose < 4) return;

	printf("weight: %d\n", p->weight);
	printf("depth: %d\n", p->depth);

	if (p->has_corral)
		printf("position has corral\n");

	if ((p->search_mode == NORMAL) || (p->search_mode == BASE_SEARCH))
	{
		print_score(&p->position_score, p->pull_mode, p->search_mode);
	}

	if (p->position_deadlocked)
		if (board_contains_boxes(c))
			printf("#position is deadlocked.#\n");

	if (print_moves)
	{

		moves_num = p->moves_num;
		if (p->search_mode == DEADLOCK_SEARCH)
			printf("(deadlock search mode)\n");
		printf("position has %d moves\n", moves_num);

		for (i = 0; i < moves_num; i++)
		{
			printf("%3d: ", i);
			print_move(p->moves + i);

			if (p->place_in_positions[i])
				printf("+");
			else
				printf("-");

			if (p->move_deadlocked[i])
			{
				printf("#deadlock#\n");
				continue;
			}

			print_score(&p->scores[i], p->pull_mode, p->search_mode);

		}
	}
}

int find_hash_in_positions(UINT_64 hash, position *positions, int pos_num)
{
	int i;

	for (i = 0; i < pos_num; i++)
	if (positions[i].position_hash == hash)
		return i;

	return -1;
}

void assert_board_after_move(position *p, int move_index, position *next)
{
	board b;
	UINT_8 data[MAX_SIZE * MAX_SIZE];

	bytes_to_board(p->board, b);
	apply_move(b, p->moves + move_index, p->search_mode);
	board_to_bytes(b, data);

	if (is_same_seq(data, next->board, height*width) == 0)
		exit_with_error("hash error");
}

void set_place_in_positions(position *p, position *positions, int pos_num)
{
	// direct new children to existing nodes
	int i, j;
	unsigned int upper;	
	UINT_64 hash_indicators[256];
	// use a 14 bit map hash of hashes...

	for (i = 0; i < 256; i++)
		hash_indicators[i] = 0;

	// project hashes of new moves
	for (i = 0; i < p->moves_num; i++)
	{
		upper = p->hashes[i] >> 50;
		hash_indicators[upper >> 6] |= (1ULL << (upper & 0x3f));
	}

	for (i = 0; i < p->moves_num; i++)
		p->place_in_positions[i] = 0;

	for (i = 0; i < pos_num; i++)
	{
		upper = positions[i].position_hash >> 50;

		if (((hash_indicators[upper >> 6] >> (upper & 0x3f)) & 1) == 0)
			continue;

		for (j = 0; j < p->moves_num; j++)
		{
			if (p->hashes[j] == positions[i].position_hash)
			{
				p->place_in_positions[j] = positions + i;
				assert_board_after_move(p, j, positions + i);

				if (positions[i].position_deadlocked)
					p->move_deadlocked[j] = 1;
			}
		}

	}

}



void allocate_position_structures(position *p)
{
	int moves_num = p->moves_num;

	if (p->moves_num == 0)
	{
		p->moves = 0;
		p->hashes = 0;
		p->scores = 0;
		p->place_in_positions = 0;
		p->move_deadlocked = 0;
		return;
	}


	p->moves = (move*)malloc(sizeof(move)* moves_num);
	if (p->moves == 0) exit_with_error("can't alloc moves\n");

	p->hashes = (UINT_64*)malloc(8 * moves_num);
	if (p->hashes == 0) exit_with_error("can't alloc hashes");

	p->scores = (score_element *)malloc(sizeof(score_element)* moves_num);
	if (p->scores == 0) exit_with_error("can't alloc scores");

	p->place_in_positions = (position **)malloc(sizeof(position *)* moves_num);
	if (p->place_in_positions == 0) exit_with_error("can't alloc places");

	p->move_deadlocked = (int *)malloc(sizeof(int)* moves_num);
	if (p->move_deadlocked == 0) exit_with_error("can't alloc deadlocks");

}

void free_position_structures(position *p)
{
	if (p->board)
		free(p->board);

	if (p->moves_num == 0) return;
	free(p->moves);
	free(p->hashes);
	free(p->scores);
	free(p->place_in_positions);
	free(p->move_deadlocked);
}

void free_positions(position *p, int pos_num)
{
	int i;
	for (i = 0; i < pos_num; i++)
		free_position_structures(p + i);
	free(p);
}



void set_hash_indicators(position* p)
{
	int i;
	unsigned int upper;

	// prepare a bitmap showing which hashes are possible

	for (i = 0; i < 64; i++)
		p->hash_indicators[i] = 0;

	for (i = 0; i < p->moves_num; i++)
	{
		upper = p->hashes[i] >> 52;
		p->hash_indicators[upper >> 6] |= (1ULL << (upper & 0x3f));
	}
}


void set_scores_to_moves(board b, move *moves, int moves_num, score_element *scores,
	UINT_64 *hashes,
	int pull_mode, int search_mode, helper *h)
{
	int i;
	board c;

	for (i = 0; i < moves_num; i++)
	{
		copy_board(b, c);
		apply_move(c, moves + i, search_mode);

		hashes[i] = get_board_hash(c);

		score_board(c, scores + i, pull_mode, search_mode, h);
	}
}


void fill_position_structures(position *p, board b, move *moves, int pull_mode,
		position *positions, int pos_num, helper *h)
{
	int i;
	int moves_num = p->moves_num;
	position *q;

	for (i = 0; i < moves_num; i++)
	{
		p->moves[i] = moves[i];
		p->move_deadlocked[i] = 0;
	}

	set_scores_to_moves(b, moves, moves_num, p->scores, p->hashes, pull_mode, p->search_mode, h);

	set_hash_indicators(p);

	set_place_in_positions(p, positions, pos_num);

	// if a son points to a deadlocked node, mark it now
	for (i = 0; i < moves_num; i++)
	{
		q = p->place_in_positions[i];
		if (q)
		{ 
			if (q->position_deadlocked)
				p->move_deadlocked[i] = 1;
		}
	}
}



void set_first_position(board input_board, int pull_mode, position *positions, int search_mode, helper *h)
{
	move moves[MAX_MOVES];
	int moves_num;
	board b;
	position *p;

	if (positions == 0)
	{
		exit_with_error("positions should be allocated\n");
	}

	copy_board(input_board, b);

	p = positions;

	p->father = NULL;
	p->depth = 0;
	p->weight = 0;
	p->pull_mode = pull_mode;
	p->search_mode = search_mode;

	p->board = (UINT_8*)malloc(width*height);
	board_to_bytes(b, p->board);

	moves_num = find_possible_moves(b, moves, pull_mode, &(p->has_corral), search_mode, h);
	p->moves_num = moves_num;

	score_board(b, &p->position_score, pull_mode, search_mode, h);

	allocate_position_structures(p);
	fill_position_structures(p, b, moves, pull_mode, positions, 0, h);

	p->position_hash = get_board_hash(b);

	p->position_deadlocked = 0;

	if (p->moves_num == 0)
	{
		if (position_is_solved(p) == 0)
		{
			if (verbose >= 5)
				printf("first position has no moves... marking as deadlock\n");
			p->position_deadlocked = 1;
		}
	}

}


void son_to_seq(position *p, int son, UINT_8 *seq)
{
	board b;

	bytes_to_board(p->board, b);
	apply_move(b, p->moves + son, p->search_mode);
	board_to_bytes(b, seq);
}


void direct_other_sons_to_position(position *pos, position *positions, int pos_num)
{
	UINT_64 hash;
	int i, j;
	position *p;
	unsigned int upper, lower;

	hash = pos->position_hash;

	upper = hash >> 52;
	lower = upper & 0x3f;
	upper >>= 6;

	for (i = 0; i < pos_num; i++)
	{
		p = positions + i;

		if (((p->hash_indicators[upper] >> lower) & 1) == 0) continue;

		for (j = 0; j < p->moves_num; j++)
		{
			if (p->hashes[j] != hash) continue;

			if (p->place_in_positions[j] != NULL)
				continue; // son already directed

			p->place_in_positions[j] = pos;

			assert_board_after_move(p, j, pos);
		}
	}
}

int all_sons_are_deadlocked(position *p)
{
	int i;
	position *q;

	for (i = 0; i < p->moves_num; i++)
	{
		if (p->move_deadlocked[i]) continue;

		q = p->place_in_positions[i];
		if (q == 0) return 0; // move not explored yet

		if (q->position_deadlocked)
			exit_with_error("move_deadlocked error");

		if (q->position_deadlocked == 0)
			return 0;
	}
	return 1;
}

void expand_position(position *node, int son, int pull_mode, position *positions, int pos_num, helper *h)
{
	board b;
	position *p;
	move moves[MAX_MOVES];
	int moves_num;
	UINT_64 hash;

	if (find_hash_in_positions(node->hashes[son], positions, pos_num) != -1)
	{
		printf("trying to expand known position\n");
		find_hash_in_positions(node->hashes[son], positions, pos_num);
		exit_with_error("exiting");
	}

	bytes_to_board(node->board, b);

	apply_move(b, node->moves + son, node->search_mode);

	hash = get_board_hash(b);
	if (hash != node->hashes[son])
		exit_with_error("different hash than expected");
	if (find_hash_in_positions(hash, positions, pos_num) != -1)
		exit_with_error("position already exists?!");


	p = &positions[pos_num];

	p->father = node;
	p->pull_mode = pull_mode;
	p->search_mode = node->search_mode;

	p->board = (UINT_8*)malloc(width*height);
	board_to_bytes(b, p->board);

	moves_num = find_possible_moves(b, moves, pull_mode, &(p->has_corral), node->search_mode, h);
	p->moves_num = moves_num;

	p->position_score = node->scores[son];

	allocate_position_structures(p);
	fill_position_structures(p, b, moves, pull_mode, positions, pos_num, h);

	p->weight = node->weight + node->moves[son].attr.weight;


	p->position_hash = get_board_hash(b);
	p->depth = node->depth + 1;
	node->place_in_positions[son] = p;

	direct_other_sons_to_position(p, positions, pos_num);

	p->position_deadlocked = 0;

	if (all_sons_are_deadlocked(p))
//	if (p->moves_num == 0)
	{ 
		if (position_is_solved(p) == 0)
		{
			{
				if (verbose >= 6)
				{
					printf("position has no moves... marking as deadlock\n");
					print_board(b);
				}
				p->position_deadlocked = 1;
				mark_deadlocks(positions, pos_num + 1);
			}
		}
	}
}

void set_leaf_as_deadlock(position *positions, int pos_num, int pos, int son)
{
	int i, j, n,k;	
	UINT_64 hash;
	int node_changed = 0;
	unsigned int upper, lower;

	hash = positions[pos].hashes[son];
	upper = hash >> 52;
	lower = upper & 0x3f;
	upper >>= 6;

	for (i = 0; i < pos_num; i++)
	{
		if (positions[i].position_deadlocked) continue;

		if (((positions[i].hash_indicators[upper] >> lower) & 1) == 0) continue;

		n = positions[i].moves_num;
		for (j = 0; j < n; j++)
		{
			if (positions[i].hashes[j] != hash) continue;

			// we should very rarely be here

			positions[i].move_deadlocked[j] = 1;

			if (i != pos)
				if (positions[i].place_in_positions[j])
					exit_with_error("leaf should not be expanded");

			// did all sons become deadlock?
			for (k = 0; k < n; k++)
				if (positions[i].move_deadlocked[k] == 0)
					break;

			if (k == n)
				node_changed = 1;
		}
	}

	if (node_changed)
		mark_deadlocks(positions, pos_num);
}

int all_moves_deadlocked(position *p)
{
	int i;

	for (i = 0; i < p->moves_num; i++)
		if (p->move_deadlocked[i] == 0)
			return 0;
	return 1;
}


int estimate_uniqe_moves(position *positions, int pos_num)
{
	int hash[1024];
	int i,j;
	int sum = 0;

	for (i = 0; i < 1024; i++)
		hash[i] = 0;

	for (i = 0; i < pos_num; i++)
		for (j = 0; j < positions[i].moves_num; j++)
			hash[positions[i].hashes[j] & 0x3ff] = 1;

	for (i = 0; i < 1024; i++)
		sum += hash[i];

	return sum;
}

void show_position_path(position* p)
{
	while (p != 0)
	{
		print_position(p, 0);
		p = p->father;
	}
	//	my_getch();
}

void best_pos_and_son(position* positions, int pos_num, int* pos, int* son, 
	int pull_mode, int search_mode)
{
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

			if ((*pos == -1) || (is_better_score(positions[i].scores + j, 
				&best_score, pull_mode, search_mode)))
			{
				best_score = positions[i].scores[j];

				*pos = i;
				*son = j;
			}
		}
	}
}
