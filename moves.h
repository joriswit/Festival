// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#ifndef __MOVE
#define __MOVE

#include "helper.h"

#define MAX_MOVES 11500

int find_possible_moves(board b, move *moves, int pull_mode, int *has_corral, int search_mode, helper *h);
void apply_move(board b, move *m, int search_mode);

void print_move(move *m);

int attr_to_weight(move_attr *a);

#endif