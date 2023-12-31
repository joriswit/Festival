// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include "board.h"
#include "moves.h"

void turn_fixed_boxes_into_walls(board b, board w);

int is_box_2x2_frozen_at_index(board b_in, int index);
int is_box_2x2_frozen_after_move(board b, move *m);
