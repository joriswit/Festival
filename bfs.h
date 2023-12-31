// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include "board.h"

void bfs(board b, int_board dist, int start_y, int start_x,
		int ignore_boxes);

int get_connectivity(board b);
int mark_connectivities(board b, int_board dist);

void mark_connectivities_around_place(board b, int_board dist,
	int start_y, int start_x);

void print_connectivity_comps(int_board dist);
void get_connectivity_sizes(int_board dist, int n, int *sizes);

void init_dist(int_board dist);
void bfs_from_place(board b, int_board dist, int start_y, int start_x,
	int ignore_boxes);

void get_srcs_of_all_targets(board b, board srcs);

void expand_group(board b, board group, int_board dist);
