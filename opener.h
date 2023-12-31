// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#ifndef __OPENER
#define __OPENER

#include "moves.h"
#include "score.h"
#include "helper.h"

int pick_opener(board b, score_element *base_score,
	int moves_num, move *moves, score_element *scores, int pull_mode, helper *h);

int explorer(board b, score_element *base_score,
	int moves_num, move *moves, score_element *scores, int pull_mode);

#endif
