// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include "board.h"
#include "moves.h"
#include "helper.h"

void check_deadlock_in_moves(board b, move *moves, int moves_num,
	int *valid, int pull_mode, int search_mode, helper *h);

void read_deadlock_patterns(int pull_mode);

void mark_deadlock_2x2(board c, board d);
int has_pull_moves(board b);

int check_all_patterns(board b_in, int box_y, int box_x, int pull_mode);
int deadlock_in_direction(board b, int y, int x, int d);

void set_forbidden_tunnel();
extern board forbidden_push_tunnel;
extern board forbidden_pull_tunnel;