// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include "board.h"

void mark_diags(board b, board diags_right, board diags_left);
int wallify_diag(board b, int i, int j, int k, int diag_right);
int diag_has_no_targets(board b, int i, int j, int n, int diag_right);
int is_diag_deadlock(board b);

