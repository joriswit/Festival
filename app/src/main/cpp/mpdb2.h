// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include "board.h"
#include "moves.h"

void build_mpdb2();
int is_mpdb_deadlock(board b, int pull_mode);

void build_pull_mpdb2();
