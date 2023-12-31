// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#ifndef __SCORE
#define __SCORE

#include "board.h"
#include "score.h"
#include "moves.h"
#include "helper.h"

typedef struct
{
	short connectivity;
	short rooms_score;
	short packed_boxes;
	short boxes_on_targets;
	short boxes_in_level;
	int dist_to_targets;
	int dist_to_imagined;
	short hotspots;
	short out_of_plan;
	short overlap;
	short imagine;
	float value;
	short biconnect;
} score_element;


void init_scored_distance(board b, int pull_mode, helper *h);
void print_score(score_element *s, int pull_mode, int search_mode);
int is_better_score(score_element *new_score, score_element* old_score, int pull_mode, int search_mode);
void score_board(board b, score_element *s, int pull_mode, int search_mode, helper *h);
int get_scored_distance(board b, helper* h);

#endif