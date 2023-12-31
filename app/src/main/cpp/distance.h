// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include "board.h"

void set_distances(board b);

extern int distance_from_to[MAX_INNER][MAX_INNER];
extern int impossible_place[MAX_INNER];
extern int bfs_distance_from_to[MAX_INNER][MAX_INNER];

void get_distance_from_targets(board b, int_board dist);

