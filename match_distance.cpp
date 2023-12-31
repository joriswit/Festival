#include <stdlib.h>
#include <stdio.h>

#include "match_distance.h"
#include "board.h"
#include "util.h"
#include "hungarian.h"
#include "distance.h"
#include "imagine.h"
#include "fixed_boxes.h"

void compute_match_distance(board b,
	score_element *base_score, int moves_num, move *moves, score_element *scores)
{
	int i, j, t;
	int boxes_num, targets_num;
	int boxes_indices[MAX_BOXES];
	int targets_indices[MAX_BOXES];
	hungarian_solution sol;
	box_mat mat;
	box_mat mat2;
	int from, to;
	hungarian_cache *hc;

	hc   = allocate_hungarian_cache();

	boxes_num   = get_indices_of_boxes(b, boxes_indices);
	targets_num = get_indices_of_targets(b, targets_indices);

	if (boxes_num != targets_num) exit_with_error("box num mismatch1");

	for (i = 0; i < boxes_num; i++)
		for (j = 0; j < targets_num; j++)
			mat[i][j] = distance_from_to[boxes_indices[i]][targets_indices[j]];

	solve_hungarian(boxes_num, mat, &sol, hc);

	base_score->dist_to_targets = sol.weight;

	for (t = 0; t < moves_num; t++)
	{
		for (i = 0; i < boxes_num; i++)
			for (j = 0; j < boxes_num; j++)
				mat2[i][j] = mat[i][j];

		from = moves[t].from;
		to   = moves[t].to;

		if (from == to)
		{
			scores[t].dist_to_targets = base_score->dist_to_targets;
			continue;
		}

		// find row of moved box
		for (i = 0; i < boxes_num; i++)
			if (boxes_indices[i] == from)
				break;

		if (i == boxes_num) exit_with_error("internal error");

		for (j = 0; j < targets_num; j++)
			mat2[i][j] = distance_from_to[to][targets_indices[j]];

		solve_hungarian(boxes_num, mat2, &sol, hc);

		scores[t].dist_to_targets = sol.weight;
	}

	free(hc);
}



void compute_imagine_distance(board b,
	score_element *base_score, int moves_num, move *moves, score_element *scores, int search_mode, helper *h)
{
	int i, j, t;
	board imagined;
	int boxes_num, targets_num;
	int boxes_indices[MAX_BOXES];
	int targets_indices[MAX_BOXES];
	hungarian_solution sol;
	box_mat mat;
	box_mat mat2;
	int from, to;
	int relevant_board;

	hungarian_cache *hc;

	if ((search_mode == FORWARD_WITH_BASES) || (search_mode == GIRL_SEARCH)) return;

	hc = allocate_hungarian_cache();

	if (search_mode == NORMAL)
		get_imagined_board(b, imagined, &relevant_board, h);
	else
		copy_board(h->imagined_hf_board, imagined);


	boxes_num   = get_indices_of_boxes(b, boxes_indices);
	targets_num = get_indices_of_boxes(imagined, targets_indices);

	if (boxes_num != targets_num)
		exit_with_error("box num mismatch2");

	for (i = 0; i < boxes_num; i++)
	{
		from = boxes_indices[i];
		for (j = 0; j < targets_num; j++)
			mat[i][j] = distance_from_to[from][targets_indices[j]];

		// add special treatment for frozen 2x2 boxes
		if (is_box_2x2_frozen_at_index(b, from))
			for (j = 0; j < targets_num; j++)
				mat[i][j] = (targets_indices[j] == from ? 0 : 1000000);

	}

	solve_hungarian(boxes_num, mat, &sol, hc);

	base_score->dist_to_imagined = sol.weight;

	for (t = 0; t < moves_num; t++)
	{
		for (i = 0; i < boxes_num; i++)
			for (j = 0; j < boxes_num; j++)
				mat2[i][j] = mat[i][j];

		from = moves[t].from;
		to = moves[t].to;

		if (from == to)
		{
			scores[t].dist_to_imagined = base_score->dist_to_imagined;
			continue;
		}

		// find row of moved box
		for (i = 0; i < boxes_num; i++)
			if (boxes_indices[i] == from)
				break;

		if (i == boxes_num) exit_with_error("internal error");

		for (j = 0; j < targets_num; j++)
			mat2[i][j] = distance_from_to[to][targets_indices[j]];

		// add special treatment if the box becomes 2x2 frozen
		if (is_box_2x2_frozen_after_move(b, moves + t))
		{
			for (j = 0; j < targets_num; j++)
				mat2[i][j] = (targets_indices[j] == to ? 0 : 1000000);
		}

		solve_hungarian(boxes_num, mat2, &sol, hc);

		scores[t].dist_to_imagined = sol.weight;
	}

	free(hc);
}


void compute_base_distance(board b,
	score_element *base_score, int moves_num, move *moves, score_element *scores)
{
	int i, j, t;
	int boxes_num, bases_num;
	int boxes_indices[MAX_BOXES];
	int bases_indices[MAX_BOXES];
	hungarian_solution sol;
	box_mat *mat;
	box_mat *mat2;
	int from, to;
	hungarian_cache *hc;

	hc = allocate_hungarian_cache();
	mat = (box_mat*) malloc(sizeof(box_mat));
	mat2 = (box_mat*)malloc(sizeof(box_mat));

	if ((mat == 0) || (mat2 == 0)) exit_with_error("can't allocate mat\n");

	boxes_num = get_indices_of_boxes(b, boxes_indices);
	bases_num = get_indices_of_initial_bases(b, bases_indices);

	if (boxes_num != bases_num) exit_with_error("box num mismatch3");

	for (i = 0; i < boxes_num; i++)
		for (j = 0; j < bases_num; j++)
			(*mat)[i][j] = distance_from_to[bases_indices[j]][boxes_indices[i]];
			// note reversed j,i . mat[i][j] is the distance from base j to box i

	solve_hungarian(boxes_num, *mat, &sol, hc);

	base_score->dist_to_targets = sol.weight;

	for (t = 0; t < moves_num; t++)
	{
		for (i = 0; i < boxes_num; i++)
			for (j = 0; j < boxes_num; j++)
				(*mat2)[i][j] = (*mat)[i][j];

		from = moves[t].from;
		to = moves[t].to;

		if (from == to)
		{
			scores[t].dist_to_targets = base_score->dist_to_targets;
			continue;
		}

		// find row of moved box
		for (i = 0; i < boxes_num; i++)
			if (boxes_indices[i] == from)
				break;

		if (i == boxes_num) exit_with_error("internal error");

		for (j = 0; j < bases_num; j++)
			(*mat2)[i][j] = distance_from_to[bases_indices[j]][to];

		solve_hungarian(boxes_num, *mat2, &sol, hc);

		scores[t].dist_to_targets = sol.weight;
	}

	free(hc);
	free(mat);
	free(mat2);
}



void compute_rev_distance(board b,
	score_element* base_score, int moves_num, move* moves, score_element* scores,
	board base_board, int from_to_mat[MAX_INNER][MAX_INNER])
{
	/*
	This is a similar code to "compute_base_distance" , but the base position
	is not the initial board, but the board reached after a forward search.
	This also means that the distance matrix might be different, due to frozen boxes.
	*/

	int i, j, t;
	int boxes_num, bases_num;
	int boxes_indices[MAX_BOXES];
	int bases_indices[MAX_BOXES];
	hungarian_solution sol;
	box_mat mat;
	box_mat mat2;
	int from, to;
	hungarian_cache* hc;

	hc = allocate_hungarian_cache();

	boxes_num = get_indices_of_boxes(b, boxes_indices);
	bases_num = get_indices_of_boxes(base_board, bases_indices);

	if (boxes_num != bases_num) exit_with_error("box num mismatch4");

	for (i = 0; i < boxes_num; i++)
		for (j = 0; j < bases_num; j++)
			mat[i][j] = from_to_mat[bases_indices[j]][boxes_indices[i]];
	// note reversed j,i . mat[i][j] is the distance from base j to box i

	solve_hungarian(boxes_num, mat, &sol, hc);

	base_score->dist_to_imagined = sol.weight;

	for (t = 0; t < moves_num; t++)
	{
		for (i = 0; i < boxes_num; i++)
			for (j = 0; j < boxes_num; j++)
				mat2[i][j] = mat[i][j];

		from = moves[t].from;
		to = moves[t].to;

		if (from == to)
		{
			scores[t].dist_to_imagined = base_score->dist_to_imagined;
			continue;
		}

		// find row of moved box
		for (i = 0; i < boxes_num; i++)
			if (boxes_indices[i] == from)
				break;

		if (i == boxes_num) exit_with_error("internal error");

		for (j = 0; j < bases_num; j++)
			mat2[i][j] = from_to_mat[bases_indices[j]][to];

		solve_hungarian(boxes_num, mat2, &sol, hc);

		scores[t].dist_to_imagined = sol.weight;
	}

	free(hc);
}
