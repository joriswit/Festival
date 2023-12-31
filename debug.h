// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include "tree.h"
#include "helper.h"
#include "dragonfly.h"

void print_node(tree* t, expansion_data* e, int print_moves, const char* message, helper* h);

void save_status(board b_in, int pull_mode);

void show_progress_tree(tree *t, expansion_data *last_node, expansion_data *best_so_far, int iter_num,
	expansion_data **last_best, helper *h);

void debug_tree(tree *t, expansion_data *e, helper *h);

void show_progress_dragonfly(dragonfly_node* best_so_far, int iter_num, helper* h);
