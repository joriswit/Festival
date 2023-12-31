// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include <stdio.h>

#include "score.h"
#include "distance.h"
#include "hungarian.h"
#include "moves.h"
#include "board.h"
#include "bfs.h"
#include "rooms.h"
#include "park_order.h"
#include "hotspot.h"
#include "deadlock_utils.h"
#include "util.h"
#include "girl.h"
#include "oop.h"
#include "overlap.h"
#include "imagine.h"
#include "mini_search.h"
#include "match_distance.h"
#include "wobblers.h"
#include "rooms_deadlock.h"
#include "naive.h"
#include "hf_search.h"
#include "biconnect.h"
#include "max_dist.h"
#include "snail.h"
#include "k_dist_deadlock.h"

void init_scored_distance(board b, int pull_mode, helper *h)
{
	int from, to, from_x, from_y, to_x, to_y;

	if (pull_mode == 0)
	{
		get_distance_from_targets(b, h->scored_distance);
		return;
	}

	zero_int_board(h->scored_distance);

	for (from = 0; from < index_num; from++)
	{
		index_to_y_x(from, &from_y, &from_x);

		for (to = 0; to < index_num; to++)
		{
			index_to_y_x(to, &to_y, &to_x);

			if (b[to_y][to_x] & TARGET)
				if (distance_from_to[from][to] != 1000000)
					(h->scored_distance)[from_y][from_x] += distance_from_to[from][to];
		}
	}
}


int get_scored_distance(board b, helper *h)
{
	int sum = 0;
	int i, j;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (b[i][j] & BOX)
				sum += (h->scored_distance)[i][j];
	return sum;
}

void print_score(score_element *s, int pull_mode, int search_mode)
{
	if (search_mode == GIRL_SEARCH)
	{
		print_score_in_girl_mode(s, pull_mode);
		return;
	}

	if (search_mode == NETLOCK_SEARCH)
	{
		print_score_in_netlock_mode(s);
		return;
	}

	if (search_mode == SNAIL_SEARCH)
	{
		print_score_in_snail_mode(s, pull_mode);
		return;
	}


	if (search_mode == NAIVE_SEARCH)
	{
		printf("packed: %2d cnct: %2d dist: %d\n",
			s->packed_boxes,
			s->connectivity,
			s->dist_to_targets);
		return;
	}

	if (search_mode == REV_SEARCH)
	{
		if (pull_mode == 0)
		{
			printf("cnct: %d room: %d bicon: %d dist: %d hotspots: %d\n",
				s->connectivity, s->rooms_score, s->biconnect, s->dist_to_targets, s->hotspots);
		}
		else
		{
			printf("cnct: %d targets: %d im_dist: %d\n",
				s->connectivity, s->boxes_on_targets, s->dist_to_imagined);
		}
		return;
	}

	if (pull_mode)
	{
		printf(" trgts: %2d connect: %d dist: %d room: %d hs: %d bicon: %d\n",
			s->boxes_on_targets,
			s->connectivity,
			s->dist_to_targets,
			s->rooms_score,
			s->hotspots,
			s->biconnect);
		return;
	}
	

	printf(" cnct: %d rm: %d hs: %d oop: %d pck: %2d img: %2d idst: %d",
		s->connectivity,
		s->rooms_score,
		s->hotspots,
		s->out_of_plan,
		s->packed_boxes,
		s->imagine,
		s->dist_to_imagined
		);

	printf("\n");
}

int is_better_score(score_element *new_score, score_element* old_score, int pull_mode, int search_mode)
{
	double val_new, val_old;

	if ((search_mode == HF_SEARCH) || 
		(search_mode == BICON_SEARCH) || 
		(search_mode == MAX_DIST_SEARCH) ||
		(search_mode == MAX_DIST_SEARCH2)
		)
		return is_better_hf_score(new_score, old_score, pull_mode);

	if (search_mode == SNAIL_SEARCH) 
		return is_better_snail_score(new_score, old_score, pull_mode);

	if (search_mode == NETLOCK_SEARCH)
		return is_better_netlock_score(new_score, old_score);

	if (search_mode == NAIVE_SEARCH)
		return is_better_naive_score(new_score, old_score, pull_mode);

	if (search_mode == REV_SEARCH)
		return is_better_rev_score(new_score, old_score, pull_mode);

	if (search_mode == K_DIST_SEARCH)
		return is_better_k_dist_score(new_score, old_score);

	if (search_mode == GIRL_SEARCH)
	{
		// can't rely on score.value, because of backpropagation from its subtree
		val_new = score_to_value(new_score, pull_mode);
		val_old = score_to_value(old_score, pull_mode);

		if (val_new > val_old) return 1;
		return 0;
	}

	if (search_mode != NORMAL)
		if (search_mode != BASE_SEARCH)
			if (search_mode != FORWARD_WITH_BASES)
				exit_with_error("verify search mode");

	if (new_score->boxes_in_level < old_score->boxes_in_level) return 1;
	if (new_score->boxes_in_level > old_score->boxes_in_level) return 0;


	if (pull_mode == 0)
	{
		if (new_score->packed_boxes > old_score->packed_boxes) return 1;
		if (new_score->packed_boxes < old_score->packed_boxes) return 0;

		if (new_score->connectivity < old_score->connectivity) return 1;
		if (new_score->connectivity > old_score->connectivity) return 0;

		if (new_score->rooms_score < old_score->rooms_score) return 1;
		if (new_score->rooms_score > old_score->rooms_score) return 0;

		if (new_score->out_of_plan < old_score->out_of_plan) return 1;
		if (new_score->out_of_plan > old_score->out_of_plan) return 0;

		if (new_score->hotspots < old_score->hotspots) return 1;
		if (new_score->hotspots > old_score->hotspots) return 0;

		if (new_score->dist_to_targets > old_score->dist_to_targets) return 1;
		if (new_score->dist_to_targets < old_score->dist_to_targets) return 0;

		return 0;
	}

	// so this is pull mode

	
	if (new_score->connectivity < old_score->connectivity) return 1;
	if (new_score->connectivity > old_score->connectivity) return 0;

	if (new_score->boxes_on_targets < old_score->boxes_on_targets) return 1;
	if (new_score->boxes_on_targets > old_score->boxes_on_targets) return 0;
	
	if (new_score->rooms_score < old_score->rooms_score) return 1;
	if (new_score->rooms_score > old_score->rooms_score) return 0;

	if (new_score->dist_to_targets > old_score->dist_to_targets) return 1;
	if (new_score->dist_to_targets < old_score->dist_to_targets) return 0;

	return 0;
}


void score_board(board b, score_element *s, int pull_mode, int search_mode, helper *h)
{
	s->boxes_on_targets = 0; 
	s->dist_to_targets = 0;
	s->dist_to_imagined = 0;
	s->boxes_in_level = global_boxes_in_level;
	s->connectivity = 0;
	s->rooms_score = 0;
	s->packed_boxes = 0;
	s->hotspots = 0;
	s->out_of_plan = 0;
	s->overlap = 0;
	s->imagine = 0;
	s->rooms_score = 0;
	s->biconnect = 0;

	if (search_mode == NAIVE_SEARCH)
	{
		fill_naive_scores(b, s, pull_mode);
		return;
	}

	if (search_mode == SNAIL_SEARCH)
	{
		fill_snail_scores(b, s, h->alg_type);
		return;
	}

	if (search_mode == NETLOCK_SEARCH)
	{
		fill_netlock_scores(b, s, h->alg_type);
		return;
	}

	s->boxes_on_targets = boxes_on_targets(b);
	s->connectivity     = get_connectivity(b);
	s->boxes_in_level   = boxes_in_level(b);

	if (search_mode == K_DIST_SEARCH)
		return;

	if (search_mode == REV_SEARCH)
	{
		set_score_in_rev_mode(b, s, pull_mode, h);
		return;
	}

	if (search_mode == XY_SEARCH)
		return;

	if (search_mode == MINI_SEARCH)
		return;

	if (search_mode == DEADLOCK_SEARCH)
	{
		s->hotspots = size_of_deadlock_zone(b);
		return;
	}

	if (
		(search_mode == HF_SEARCH) || 
		(search_mode == BICON_SEARCH) || 
		(search_mode == MAX_DIST_SEARCH) ||
		(search_mode == MAX_DIST_SEARCH2))
	{
		fill_hf_scores(b, s, pull_mode, search_mode, h);
		return;
	}


	if ((search_mode == WOBBLERS_SEARCH) || (search_mode == ROOMS_DEADLOCK_SEARCH))
	{
		fill_wobblers_scores(b, s, pull_mode);
		return;
	}

	if (search_mode == GIRL_SEARCH)
	{
		fill_girl_scores(b, s, pull_mode, h);
		return;
	}

	if (search_mode != NORMAL)
		if (search_mode != BASE_SEARCH)
			if (search_mode != FORWARD_WITH_BASES)
				exit_with_error("verify search mode");

	s->rooms_score = score_rooms(b);
	s->dist_to_targets = get_scored_distance(b, h);

	if (pull_mode == 0)
	{
		s->packed_boxes = score_parked_boxes(b,h);
		s->hotspots     = hotspots_score(b);
		s->out_of_plan  = get_out_of_plan_score(b, s->packed_boxes, h);
		s->imagine      = imagine_score(b, search_mode, h);
	}
}
