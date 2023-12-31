// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#ifndef __PARK_ORDER
#define __PARK_ORDER

#include "board.h"
#include "request.h"
#include "tree.h"
#include "helper.h"

void show_parking_order_on_board(helper *h);
void show_parking_order(helper *h);
void verify_parking_order(helper *h);
void conclude_parking(tree *t, expansion_data *e, helper *h);
int score_parked_boxes(board b, helper *h);
void keep_only_perm(helper *h);
void reduce_parking_order(helper *h);
int next_parking_place(board b, helper *h);

#endif