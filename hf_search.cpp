#include <stdlib.h>
#include <stdio.h>

#include "hf_search.h"
#include "util.h"
#include "holes.h"
#include "match_distance.h"
#include "distance.h"
#include "rooms.h"
#include "biconnect.h"
#include "fixed_boxes.h"
#include "io.h"
#include "score.h"
#include "deadlock.h"
#include "bfs.h"


int can_be_accessed_from_some_direction(board b, int y, int x)
{
	int i;

	for (i = 0; i < 4; i++)
	{
		if (b[y + delta_y[i]][x + delta_x[i]] == WALL) continue;
		if (b[y + delta_y[i] * 2][x + delta_x[i] * 2] & OCCUPIED) continue;

		return 1;
	}

	return 0;
}


int hf_hotspots(board b, int pull_mode, helper* h)
{
	int i, j, k;
	int sum = 0;

	if (pull_mode)
	{
		for (i = 0; i < height; i++)
			for (j = 0; j < width; j++)
			{
				if (b[i][j] & BOX)
					if (can_be_accessed_from_some_direction(b, i, j) == 0)
						sum++;
			}
		return sum;
	}


	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if ((b[i][j] & BOX) && ((h->imagined_hf_board[i][j] & BOX) == 0))
			{
				for (k = 0; k < 4; k++)
				{
					if (b[i + delta_y[k]][j + delta_x[k]] & OCCUPIED)
					{
						sum++;
						break;
					}
				}
			}

			if ((h->imagined_hf_board[i][j] & BOX) && ((b[i][j] & BOX) == 0))
				if (can_be_accessed_from_some_direction(b, i, j) == 0)
					sum++;
		}

	return sum;
}


int is_better_hf_score(score_element* new_score, score_element* old_score, int pull_mode)
{

	if (pull_mode == 0)
	{
		if ((new_score->dist_to_imagined < 1000000) && (old_score->dist_to_imagined >= 1000000)) return 1;
		if ((new_score->dist_to_imagined >= 1000000) && (old_score->dist_to_imagined < 1000000)) return 0;

		if (new_score->connectivity < old_score->connectivity) return 1;
		if (new_score->connectivity > old_score->connectivity) return 0;

		if (new_score->imagine > old_score->imagine) return 1;
		if (new_score->imagine < old_score->imagine) return 0;

		if (new_score->hotspots < old_score->hotspots) return 1;
		if (new_score->hotspots > old_score->hotspots) return 0;

		if (new_score->dist_to_imagined < old_score->dist_to_imagined) return 1;
		if (new_score->dist_to_imagined > old_score->dist_to_imagined) return 0;

		return 0;
	}
	
	if (new_score->connectivity < old_score->connectivity) return 1;
	if (new_score->connectivity > old_score->connectivity) return 0;

	if (new_score->rooms_score < old_score->rooms_score) return 1;
	if (new_score->rooms_score > old_score->rooms_score) return 0;

	if ((new_score->connectivity == 1) && (old_score->connectivity == 1))
	{
		if (new_score->hotspots < old_score->hotspots) return 1;
		if (new_score->hotspots > old_score->hotspots) return 0;

		if (new_score->biconnect < old_score->biconnect) return 1;
		if (new_score->biconnect > old_score->biconnect) return 0;
	}

	if (new_score->dist_to_targets > old_score->dist_to_targets) return 1;
	if (new_score->dist_to_targets < old_score->dist_to_targets) return 0;

	return 0;
}

int is_better_rev_score(score_element* new_score, score_element* old_score, int pull_mode)
{
	if (pull_mode)
	{
		if ((new_score->dist_to_imagined < 1000000) && 
			(old_score->dist_to_imagined >= 1000000)) return 1;

		if ((new_score->dist_to_imagined >= 1000000) &&
			(old_score->dist_to_imagined < 1000000)) return 0;

		if (new_score->connectivity < old_score->connectivity) return 1;
		if (new_score->connectivity > old_score->connectivity) return 0;

		if (new_score->connectivity > 1)
		{
			if (new_score->dist_to_targets > old_score->dist_to_targets) return 1;
			if (new_score->dist_to_targets < old_score->dist_to_targets) return 0;
		}

		// this is actually boxes on imagined. Using boxes_on_targets for the pack cell.
		if (new_score->boxes_on_targets > old_score->boxes_on_targets) return 1;
		if (new_score->boxes_on_targets < old_score->boxes_on_targets) return 0;

		if (new_score->dist_to_imagined < old_score->dist_to_imagined) return 1;
		if (new_score->dist_to_imagined > old_score->dist_to_imagined) return 0;

		return 0;
	}

	if (new_score->connectivity < old_score->connectivity) return 1;
	if (new_score->connectivity > old_score->connectivity) return 0;

	if (new_score->rooms_score < old_score->rooms_score) return 1;
	if (new_score->rooms_score > old_score->rooms_score) return 0;

	if (new_score->hotspots < old_score->hotspots) return 1;
	if (new_score->hotspots > old_score->hotspots) return 0;

	// actually dist from hotspots, when hotspots on board
	if (new_score->dist_to_targets > old_score->dist_to_targets) return 1;
	if (new_score->dist_to_targets < old_score->dist_to_targets) return 0;

	return 0;
}

void set_rev_advisor(board b, score_element* base_score, int moves_num, move* moves,
	score_element* scores, int pull_mode,
	int search_mode, int* already_expanded, advisors_element* l)
{
	int i, y, x;
	score_element s;

	s = *base_score;
	for (i = 0; i < moves_num; i++)
	{
		if (pull_mode == 0) // push to holes
		{
			index_to_y_x(moves[i].to, &y, &x);

			if (target_holes[y][x])
			{
				l->pack = i;
				return;
			}
		}

		if (already_expanded[i]) continue;

		if (is_better_score(scores + i, &s, pull_mode, search_mode))
		{
			s = scores[i];
			l->pack = i;
		}
	}
}

void setup_rev_board(expansion_data *e, helper *h)
{
	board w;
	int i, j;
/*
	FILE* fp;
	fp = fopen("c:\\sokoban\\rev.sok", "a");
	board tt;
	bytes_to_board(e->b, tt);
	get_sokoban_position(tt, &i, &j);
	clear_sokoban_inplace(tt);
	tt[i][j] |= SOKOBAN;
	save_board_to_file(tt, fp);
	fprintf(fp, "\n");
	fclose(fp);
*/

	bytes_to_board(e->b, h->imagined_hf_board);

	if (verbose >= 4)
	{
		printf("imagined rev:\n");
		print_board(h->imagined_hf_board);
	}

	copy_board(h->imagined_hf_board, w);
	turn_fixed_boxes_into_walls(h->imagined_hf_board, w);

	zero_board(h->wallified);
	for (i = 0 ; i < height ; i++)
		for (j = 0; j < width; j++)
		{
			if (w[i][j] == WALL)
				if (inner[i][j])
					h->wallified[i][j] = 1;
		}

//	my_getch();

}


void set_rev_distance(tree *t, expansion_data* e, helper *h,
	move *moves, score_element *scores)
{
	// moves and scores are allocated by caller to avoid stack overflow

	move_hash_data* mh;
	int i, place;
	board b;

	mh = t->move_hashes + e->move_hash_place;
	for (i = 0; i < e->moves_num; i++)
	{
		moves[i] = mh[i].move;
		get_score_of_hash(t, mh[i].hash, scores + i);
	}

	bytes_to_board(e->b, b);

	compute_rev_distance(b, &(e->node->score), e->moves_num, moves, scores,
						h->imagined_hf_board, distance_from_to); // TODO: dist

	for (i = 0; i < e->moves_num; i++)
	{
		place = find_node_by_hash(t, mh[i].hash);
		if (place == -1) exit_with_error("missing node");
		t->nodes[place].score.dist_to_imagined = scores[i].dist_to_imagined;
	}
}


int can_be_pushed_to_direction(board b, int y, int x, int d)
{
	if (b[y + delta_y[d]][x + delta_x[d]] & OCCUPIED) return 0;
	if (b[y + delta_y[(d + 2) & 3]][x + delta_x[(d + 2) & 3]] & OCCUPIED) return 0;

	if (deadlock_in_direction(b, y, x, d)) return 0;

	return 1;
}

void get_rev_hotspots(board b, board hs)
{
	// count boxes that can't be pushed
	zero_board(hs);

	int i, j, k;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (b[i][j] & BOX)
			{
				if (b[i][j] & TARGET) continue; // no need to be pushed

				for (k = 0; k < 4; k++)
					if (can_be_pushed_to_direction(b, i, j, k))
						break;

				if (k == 4)
					hs[i][j] = 1;
			}
}

int rev_hotspots_distance(board b, helper *h)
{
	board hs, b2;
	int_board dist;
	int i, j, sum = 0;

	get_rev_hotspots(b, hs);

	if (board_popcnt(hs) == 0)
		return get_scored_distance(b, h); 
	// no hotspots. With nothing better to do, push away from targets

	// find BFS distance from hotspots
	clear_boxes(initial_board, b2);
	expand_group(b2, hs, dist);

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (b[i][j] & BOX)
				sum += dist[i][j];
	return sum;
}

int rev_hotspots(board b)
{
	board hs;

	get_rev_hotspots(b, hs);

	return board_popcnt(hs);
}

void set_score_in_rev_mode(board b, score_element* s, int pull_mode, helper *h)
{
	if (pull_mode == 0)
	{
		s->rooms_score = score_rooms(b);
		s->hotspots = rev_hotspots(b);
		s->dist_to_targets = rev_hotspots_distance(b, h);

		return;
	}

	s->boxes_on_targets = compare_to_imagined_hf(b, h);
	s->dist_to_targets = get_scored_distance(b, h);
}


int compare_to_imagined_hf(board b, helper* h)
{
	int i, j;
	int sum = 0;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (b[i][j] & BOX)
				if (h->imagined_hf_board[i][j] & BOX)
					sum++;
	return sum;
}


int get_distance_in_hf_search(board b, score_element *s, helper *h)
{
	board untouched, tmp;
	int_board dist;
	int i, j, sum = 0;
	
//	if (s->connectivity == 1) // TODO : help hotspots
		return get_scored_distance(b, h);

	get_untouched_areas(b, untouched);

	clear_boxes(initial_board, tmp);
	expand_group(tmp, untouched, dist);

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (b[i][j] & BOX)
				sum += dist[i][j];
	return sum;
}

void fill_hf_scores(board b, score_element* s, int pull_mode, int search_mode, helper* h)
{
	s->boxes_on_targets = 0;
	// we don't use this feauture in hf, so don't partition cells by it

	s->hotspots = hf_hotspots(b, pull_mode, h);

	if (pull_mode == 0)
	{
		s->imagine = compare_to_imagined_hf(b, h);
		return;
	}

	if ((search_mode == BICON_SEARCH) || (search_mode == MAX_DIST_SEARCH))
		s->biconnect = biconnect_score(b, 0);

	s->rooms_score = score_rooms(b);

	//	s->dist_to_targets = get_scored_distance(b, h);
	s->dist_to_targets = get_distance_in_hf_search(b, s, h);
}