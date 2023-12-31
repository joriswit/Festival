#include <math.h>
#include <string.h>
#include <stdio.h>

#include "girl.h"
#include "util.h"
#include "greedy.h"
#include "park_order.h"
#include "overlap.h"
#include "bfs.h"
#include "deadlock_utils.h"
#include "tree.h"
#include "distance.h"

int_board girl_distance;
int max_push_dist;

double gamma_table[MAX_BOXES + 1];
double inv_n_boxes;
double dist_normalizer;

void init_girl_distance(board b)
{
	int i, j;
	get_distance_from_targets(b, girl_distance);

	if (verbose >= 5)
		show_ints_on_initial_board(girl_distance);

	// init max_push_dist:
	max_push_dist = 0;
	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if (girl_distance[i][j] == 1000000) continue;
			if (girl_distance[i][j] > max_push_dist)
				max_push_dist = girl_distance[i][j];
		}
}

int get_girl_distance(board b)
{
	int sum = 0;
	int i, j;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (b[i][j] & BOX)
				sum += girl_distance[i][j];
	return sum;
}

void init_gamma_table()
{
	int i;
	double gamma_val = 0.95;

	for (i = 0; i <= global_boxes_in_level; i++)
		gamma_table[i] = pow(gamma_val, i);
}

void init_girl_variables(board b)
{
	init_girl_distance(b);
	init_gamma_table();	
	
	inv_n_boxes = 1 / (double)global_boxes_in_level;
	dist_normalizer = 1 / (double)(global_boxes_in_level * max_push_dist);

	set_inv_epsilon();
}


int prepare_vector_of_basic_features(score_element *s, int pull_mode, double *features)
{
	int n = 0;
	int packed, tgt, connectivity, overlap, dist;

	packed = s->packed_boxes;
	tgt = s->boxes_on_targets;
	connectivity = s->connectivity;
	dist = s->dist_to_targets;
	overlap = s->overlap;

	if (pull_mode == 0)
	{
		features[n++] = packed * inv_n_boxes;
		features[n++] = tgt * inv_n_boxes;
		features[n++] = connectivity * inv_n_boxes;
		features[n++] = dist * dist_normalizer;
		features[n++] = overlap * inv_n_boxes;
		features[n++] = gamma_table[global_boxes_in_level - tgt];
		features[n++] = gamma_table[global_boxes_in_level];
		features[n++] = 1;
		return n;
	}

	features[n++] = tgt * inv_n_boxes;
	features[n++] = connectivity * inv_n_boxes;
	features[n++] = dist * dist_normalizer;
	features[n++] = gamma_table[tgt];
	features[n++] = gamma_table[global_boxes_in_level];
	features[n++] = 1;
	return n;
}

// constants for the next routine
double pull_weights[6] = { -0.185353, -0.217472, 0.154112, 0.382115, 0.471096, 0.195508 };
double push_weights[8] = { 0.468630, -0.177816, -0.233347, 0.052064, 0.326019, 0.164538, 0.348762, -0.141263 };

double score_to_value(score_element *s, int pull_mode)
{
	int i, n;
	double features[10];
	double sum = 0;

	n = prepare_vector_of_basic_features(s, pull_mode, features);

	if (pull_mode)
	{
		for (i = 0; i < n; i++)
			sum += features[i] * pull_weights[i];
	}
	else
	{
		for (i = 0; i < n; i++)
			sum += features[i] * push_weights[i];
	}

	return sum;
}

void set_value_to_position_tree(tree *t, expansion_data *e)
{
	e->node->score.value = (float)score_to_value(&(e->node->score), t->pull_mode);
}


void set_values_to_moves_tree(tree *t, expansion_data *e)
{
	int i;
	int node_place;
	node_element *node;
	move_hash_data *mh = t->move_hashes + e->move_hash_place;

	set_value_to_position_tree(t,e);

	for (i = 0; i < e->moves_num; i++)
	{
		if (mh[i].deadlocked) continue;

		node_place = find_node_by_hash(t, mh[i].hash);
		if (node_place == -1) exit_with_error("missing node");
		node = t->nodes + node_place;

		node->score.value = (float)score_to_value(&(node->score), t->pull_mode);
	}
}


void handle_new_pos_in_girl_mode_tree(tree *t, expansion_data *e)
{
	set_values_to_moves_tree(t,e);
	back_propagate_values_tree(t,e);
}


void print_score_in_girl_mode(score_element *s, int pull_mode)
{
	if (pull_mode)
	{
		printf(" tgts: %2d connect: %2d dist: %4d  value= %lf\n",
			s->boxes_on_targets,
			s->connectivity,
			s->dist_to_targets,
			s->value);
		return;
	}

	printf(" pack: %2d tgt: %2d cnct: %2d ovlp: %2d dist: %3d value= %lf\n",
		s->packed_boxes,
		s->boxes_on_targets,
		s->connectivity,
		s->overlap,
		s->dist_to_targets,
		s->value
		);
}


expansion_data* find_best_RL_parking_node(tree *t)
{
	int i;
	double value;
	double max_value = -1000000;
	int min_depth = 1000000;
	expansion_data *best_node = 0;
	int max_allowed_depth;
	expansion_data *expansions = t->expansions;
	expansion_data *e;
	double dist;

	max_allowed_depth = MAX_SOL_LEN - global_boxes_in_level;

	for (i = 0; i < t->expansions_num; i++)
	{
		if (expansions[i].depth >= max_allowed_depth) continue;
		if (expansions[i].node->deadlocked) continue;

		value = score_to_value(&(expansions[i].node->score), 1);
		if (value > max_value)
		{
			max_value = value;
			best_node = expansions + i;
		}
	}

	dist = 0.1;

	e = best_node;
	while (e != 0)
	{
		value = score_to_value(&(e->node->score), 1);

		if (value > (max_value - dist))
			best_node = e;

		e = e->father;
	}

	if (best_node == 0) exit_with_error("could not find chain");

	return best_node;
}


void fill_girl_scores(board b, score_element *s, int pull_mode, helper *h)
{
	s->dist_to_targets = get_girl_distance(b);

	if (pull_mode == 0)
	{
		s->packed_boxes = score_parked_boxes(b, h);
		s->overlap = overlap_score(b, h);
	}
}
