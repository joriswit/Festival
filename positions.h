// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#ifndef __POSITION
#define __POSITION

#include "score.h"
#include "moves.h"
#include "global.h"
#include "helper.h"

typedef struct position
{
	int moves_num;

	UINT_8 *board;

	move *moves;
	score_element *scores;

	struct position **place_in_positions;
	int *move_deadlocked;
	UINT_64 *hashes;

	struct position *father;

	int depth;

	score_element position_score;

	UINT_64 position_hash;
	int position_deadlocked;

	int weight;
	int pull_mode; 

	int has_corral;

	UINT_64 hash_indicators[64];
	
	int search_mode;

} position;

position *allocate_positions(int pos_num);

void print_position(position *p, int print_moves);
void set_first_position(board input_board, int pull_mode, position *positions, int search_mode, helper *h);
void expand_position(position *node, int son, int pull_mode, position *positions, int pos_num, helper *h);

void free_positions(position *p, int pos_num);

void mark_deadlocks(position *positions, int pos_num);

void direct_other_sons_to_position(position *pos, position *positions, int pos_num);

int position_is_solved(position *p);
void set_leaf_as_deadlock(position *positions, int pos_num, int pos, int son);

int estimate_uniqe_moves(position *positions, int pos_num);

void show_position_path(position* p);

void best_pos_and_son(position* positions, int pos_num, int* pos, int* son,
	int pull_mode, int search_mode);


#endif