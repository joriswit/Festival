// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include "board.h"
#include "moves.h"
#include "score.h"

int move_is_deadlocked(board b, move *m, int pull_mode, int search_mode);

int verify_simple_base_matching(board b, board board_with_bases);
int can_played_reversed(board b, move *m, int pull_mode, int search_mode);

int check_matching_deadlock(board b_in, int pull_mode, int search_mode);
int start_zone_is_deadlocked(board b);
