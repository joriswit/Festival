// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include "board.h"
#include "tree.h"
#include "helper.h"

void store_solution_in_helper(tree *t, expansion_data *e, helper *h);
void save_sol_moves(helper *h);

void save_level_log(int solved);

void write_log_header();
void write_log_footer();

void write_solution_header();
void save_level_to_solution_file(board b);
void save_times_to_solutions_file(int sol_time);

void init_log_file();
void write_tree_size_to_log();