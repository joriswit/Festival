// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#ifndef __LEADERS
#define __LEADERS

#include "moves.h"
#include "score.h"

typedef struct advisors_element
{
	short pack;
	short connectivity;
	short rooms;
	short hotspots;
	short opener;
	short out_of_plan;
	short explorer;
	short imagine;
} advisors_element;

void set_advisors_inner(board b, score_element *base_score,
	int moves_num, move *moves, score_element *scores, int pull_mode, int search_mode,
	advisors_element *l, int *already_expanded, helper *h);

void print_advisors(advisors_element *l, move *moves, score_element *scores, int pull_mode, int search_mode);

int is_an_advisor(advisors_element *a, int son);

#endif