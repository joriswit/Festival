// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include "board.h"


void show_board(board b);

void save_board_to_file(board b, FILE *fp);
void save_debug_board(board b);

int load_level_from_file(board b, int level_number); // level number is one-based!

void print_in_color(const char *txt, const char *color);
