// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include "board.h"

void init_hotspots(board b);

int hotspots_score(board b);

void find_hotspots(board b, board h, board weights);

int double_blocking(int index1, int index2);
