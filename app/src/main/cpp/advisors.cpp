// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include <stdio.h>

#include "advisors.h"
#include "hotspot.h"
#include "opener.h"
#include "fixed_boxes.h"
#include "packing_search.h"
#include "util.h"
#include "graph.h"
#include "park_order.h"
#include "oop.h"
#include "imagine.h"
#include "heuristics.h"
#include "holes.h"
#include "hf_search.h"

#define OPTIMIZE_PACKED        0
#define OPTIMIZE_CONNECTIVITY  1
#define OPTIMIZE_ROOMS         2
#define OPTIMIZE_HOTSPOTS      3
#define OPTIMIZE_OUT_OF_PLAN   4


int choose_advisor(score_element *base_score, int moves_num, score_element *scores, int criterion, int pull_mode)
{
	int i;
	int best_move = -1;
	score_element best_score;

	for (i = 0; i < moves_num; i++)
	{
		if (scores[i].packed_boxes < base_score->packed_boxes) continue;

		switch (criterion)
		{
		case OPTIMIZE_PACKED:
			if (scores[i].packed_boxes <= base_score->packed_boxes) continue;
			break;
		case OPTIMIZE_CONNECTIVITY:
			if (scores[i].connectivity >= base_score->connectivity) continue;
			break;
		case OPTIMIZE_ROOMS:
			if (scores[i].rooms_score >= base_score->rooms_score) continue;
			break;
		case OPTIMIZE_HOTSPOTS:
			if (scores[i].hotspots >= base_score->hotspots) continue;
			break;
		case OPTIMIZE_OUT_OF_PLAN:
			if (scores[i].out_of_plan >= base_score->out_of_plan) continue;
			break;
		default:
			exit_with_error("bad optimizer\n");
		}

		if (best_move == -1)
		{
			best_move = i;
			best_score = scores[i];
		}

		if (is_better_score(scores + i, &best_score, pull_mode, NORMAL)) 
		{
			best_score = scores[i];
			best_move = i;
		}
	}
	return best_move;
}

void set_default_advisor(score_element *base_score, int moves_num, move *moves, 
	score_element *scores, int pull_mode, 
	int search_mode, int *already_expanded, advisors_element *l)
{
	int i,y,x;
	score_element s;

	s = *base_score;
	for (i = 0; i < moves_num; i++)
	{
		if (pull_mode) // don't pull from holes
		{
			index_to_y_x(moves[i].from, &y, &x);
			if (target_holes[y][x]) continue;
		}

		if (already_expanded[i]) continue;

		if (is_better_score(scores + i, &s, pull_mode, search_mode))
		{
			s = scores[i];
			l->pack = i;
		}
	}
}


void discard_advisors_in_base_search(move *moves, advisors_element *l)
{
	// there is a lot of knowledge in the packing advisor.
	// if it can remove a box, prioritize it over all other advisors

	if (l->pack == -1) return;

	if (moves[l->pack].base)
	{
		l->connectivity = -1;
		l->rooms = -1;
		l->hotspots = -1;
		l->opener = -1;
		l->out_of_plan = -1;
		l->explorer = -1;
		l->imagine = -1;
	}
}

void set_advisors_inner(board b, score_element *base_score, 
	int moves_num, move *moves, score_element *scores, int pull_mode, int search_mode,
	advisors_element *l, int *already_expanded, helper *h)
{
	l->pack = -1;
	l->connectivity = -1;
	l->rooms = -1;
	l->hotspots = -1;
	l->opener = -1;
	l->out_of_plan = -1;
	l->explorer = -1;
	l->imagine = -1;

	if (search_mode == GIRL_SEARCH) return;
	if (search_mode == NAIVE_SEARCH) return;
	if (search_mode == SNAIL_SEARCH) return;

	if ((search_mode == HF_SEARCH) || 
		(search_mode == BICON_SEARCH) || 
		(search_mode == MAX_DIST_SEARCH) ||
		(search_mode == MAX_DIST_SEARCH2) ||
		(search_mode == NETLOCK_SEARCH))
	{
		set_default_advisor(base_score, moves_num, moves, scores, 
			pull_mode, search_mode, already_expanded, l);
		return;
	}

	if (search_mode == REV_SEARCH) // prioritize pushing to holes
	{
		set_rev_advisor(b, base_score, moves_num, moves, scores,
			pull_mode, search_mode, already_expanded, l);
		return;
	}

	if (pull_mode)
	{
		l->pack = packing_search_advisor(b, base_score, moves_num, moves, scores, search_mode);
		l->connectivity = choose_advisor(base_score, moves_num, scores, OPTIMIZE_CONNECTIVITY, 1);
		l->rooms = choose_advisor(base_score, moves_num, scores, OPTIMIZE_ROOMS, 1);			
		l->explorer = explorer(b, base_score, moves_num, moves, scores, pull_mode);

		if (search_mode == BASE_SEARCH) // if a box can be removed, prioritize it over other options
			discard_advisors_in_base_search(moves, l);

		return;
	}


	l->pack         = choose_advisor(base_score, moves_num, scores, OPTIMIZE_PACKED, 0);
	l->connectivity = choose_advisor(base_score, moves_num, scores, OPTIMIZE_CONNECTIVITY, 0);
	l->rooms        = choose_advisor(base_score, moves_num, scores, OPTIMIZE_ROOMS, 0);	
	l->hotspots     = choose_advisor(base_score, moves_num, scores, OPTIMIZE_HOTSPOTS, 0);
	
	l->imagine      = choose_imagine_advisor(base_score, moves_num, scores, search_mode);
	l->out_of_plan  = new_oop_advisor(b, base_score, moves_num, moves, scores, pull_mode, h);
	l->opener       = pick_opener(b, base_score, moves_num, moves, scores, pull_mode, h);
	l->explorer     = explorer(b, base_score, moves_num, moves, scores, pull_mode);


}


void print_move_if_available(move *moves, score_element *scores, int son, int pull_mode, int search_mode)
{
	if (son == -1)
	{
		printf(" - \n");
		return;
	}

	print_move(moves + son);
	print_score(scores + son, pull_mode, search_mode);
}

void print_advisors(advisors_element *l, move *moves, score_element *scores, int pull_mode, int search_mode)
{
	printf("ADVISORS:\n");
	if (pull_mode)
	{
		printf("pack:  "); print_move_if_available(moves, scores, l->pack,         pull_mode, search_mode);
		printf(" exp:  "); print_move_if_available(moves, scores, l->explorer,     pull_mode, search_mode);
		printf("conn:  "); print_move_if_available(moves, scores, l->connectivity, pull_mode, search_mode);
		printf("room:  "); print_move_if_available(moves, scores, l->rooms,        pull_mode, search_mode);

		return;
	}

	printf("pack: "); print_move_if_available(moves, scores, l->pack,         pull_mode, search_mode);
	printf("conn: "); print_move_if_available(moves, scores, l->connectivity, pull_mode, search_mode);
	printf("room: "); print_move_if_available(moves, scores, l->rooms,        pull_mode, search_mode);
	printf("  hs: "); print_move_if_available(moves, scores, l->hotspots,     pull_mode, search_mode);
	printf("open: "); print_move_if_available(moves, scores, l->opener,       pull_mode, search_mode);
	printf(" exp: "); print_move_if_available(moves, scores, l->explorer,     pull_mode, search_mode);
	printf(" oop: "); print_move_if_available(moves, scores, l->out_of_plan,  pull_mode, search_mode);
	printf(" img: "); print_move_if_available(moves, scores, l->imagine,      pull_mode, search_mode);
}


int is_an_advisor(advisors_element *a, int son)
{
	if (a->pack == son) return 1;
	if (a->connectivity == son) return 1;
	if (a->rooms == son) return 1;
	if (a->hotspots == son) return 1;
	if (a->opener == son) return 1;
	if (a->out_of_plan == son) return 1;
	if (a->explorer == son) return 1;
	if (a->imagine == son) return 1;

	return 0;
}