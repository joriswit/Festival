#include <stdlib.h>
#include <stdio.h>

#include "tree.h"
#include "util.h"
#include "deadlock_utils.h"
#include "match_distance.h"
#include "queue.h"
#include "perimeter.h"
#include "advanced_deadlock.h"
#include "imagine.h"
#include "naive.h"
#include "hf_search.h"
#include "max_dist.h"

void init_tree(tree *t, int log_max_nodes)
{
	int i;
	size_t size; 

	// hash_array
	t->hash_size = 1 << (log_max_nodes + 2);
	size = sizeof(int) * (size_t)t->hash_size;
	if (verbose >= 4)
		printf("Allocating %12llu bytes for hash_array\n", (UINT_64)size);
	t->hash_array = (int*)malloc(size);
	t->mask = t->hash_size - 1;

	if (t->hash_array == 0) exit_with_error("can't allocate hash_array");
	for (i = 0; i < t->hash_size; i++)
		t->hash_array[i] = -1;

	// nodes
	t->max_nodes = 1 << log_max_nodes;
	size = sizeof(node_element) * (size_t)t->max_nodes;
	if (verbose >= 4)
		printf("Allocating %12llu bytes for %12d nodes\n", (UINT_64)size, t->max_nodes);
	t->nodes = (node_element*)malloc(size);
	t->nodes_num = 0;
	if (t->nodes == 0) exit_with_error("can't allocate nodes");

	// expansions
	t->max_expansions = 1 << (log_max_nodes - 3);
	size = sizeof(expansion_data) * (size_t)t->max_expansions;
	if (verbose >= 4)
		printf("Allocating %12llu bytes for %12d expansions\n", (UINT_64)size, t->max_expansions);
	t->expansions = (expansion_data *)malloc(size);
	if (t->expansions == 0) exit_with_error("can't allocate expansions list");
	t->expansions_num = 0;

	// boards
	t->boards_size = size * 2;
	if (verbose >= 4)
		printf("Allocating %12llu bytes for    ~ %7d boards\n", (UINT_64)t->boards_size, 
			(int)(t->boards_size / 225)); // assume 15x15 boards
	t->boards = (UINT_8 *)malloc((size_t)t->boards_size);
	if (t->boards == 0) exit_with_error("can't allocate boards");

	// move-shashes
	t->max_move_hashes = 1 << (log_max_nodes + 1);
	size = sizeof(move_hash_data) * (size_t)t->max_move_hashes;
	if (verbose >= 4)
		printf("Allocating %12llu bytes for %12d move-hashes\n", (UINT_64)size, t->max_move_hashes);
	t->move_hashes = (move_hash_data *)malloc(size);
	if (t->move_hashes == 0) exit_with_error("can't allocate move-hashes");
	t->move_hashes_num = 0;

	// label queues
	t->queues = (queue*)malloc(sizeof(queue) * MAX_TREE_LABELS);
	t->labels_num = 0;

	t->pull_mode = -1;
	t->search_mode = -1;
}

void free_tree(tree *t)
{
	int i;

	free(t->hash_array);
	free(t->nodes);
	free(t->expansions);
	free(t->move_hashes);
	free(t->boards);

	for (i = 0; i < t->labels_num; i++)
		free_heap(t->queues + i);
	free(t->queues);
}

void reset_tree(tree *t)
{
	int i;

	for (i = 0; i < t->hash_size; i++)
		t->hash_array[i] = -1;
	t->nodes_num = 0;
	t->expansions_num = 0;
	t->move_hashes_num = 0;

	for (i = 0; i < t->labels_num; i++)
		reset_heap(t->queues + i);
	t->labels_num = 0;
	
}

int tree_nearly_full(tree *t)
{
	if (t->expansions_num > (t->max_expansions - 2))
	{
		if (verbose >= 4) printf("max tree expansions reached\n");
		return 1;
	}
	if (t->nodes_num > (t->max_nodes - MAX_MOVES))
	{
		if (verbose >= 4) printf("max tree nodes reached\n");
		return 1;
	}
	if (t->move_hashes_num > (t->max_move_hashes - MAX_MOVES))
	{
		if (verbose >= 4) printf("max tree move-hashes reached\n");
		return 1;
	}

	if (((t->expansions_num + 1) * height * width) >= (t->boards_size))
	{
		if (verbose >= 4) printf("max boards reached\n");
		return 1;
	}

	return 0;
}


int find_node_by_hash(tree *t, UINT_64 hash)
{
	int index, place;
	index = hash & t->mask;

	place = t->hash_array[index];
	while (place != -1)
	{
		if (t->nodes[place].hash == hash)
			return place;

		index = (index + 1) & t->mask;
		place = t->hash_array[index];
	}

	return -1;
}

void add_entry_to_hash_array(tree *t, UINT_64 hash, int node_place)
{
	int index,place;
	index = hash & t->mask;

	place = t->hash_array[index];
	while (place != -1)
	{
		if (t->nodes[place].hash == hash) exit_with_error("already in array");
		index = (index + 1) & t->mask;
		place = t->hash_array[index];
	}
	t->hash_array[index] = node_place;
}

void get_score_of_hash(tree *t, UINT_64 hash, score_element *s)
{
	int place;

	place = find_node_by_hash(t, hash);
	if (place == -1) exit_with_error("missing node");
	*s = t->nodes[place].score;
}

void update_subtree_size(tree* t, expansion_data* e)
{
	if (e == 0) return;
	if (e->node->deadlocked) return;

	e->subtree_size++;
	update_subtree_size(t, e->father);
}

void add_node_to_tree(tree *t, UINT_64 hash, score_element *s)
{
	int place;
	node_element *node;

	place = find_node_by_hash(t, hash);
	if (place != -1) 
		exit_with_error("adding exisitng hash");

	place = t->nodes_num;
	node = t->nodes + place;
	node->expansion = -1;
	node->hash = hash;
	node->score = *s;
	node->deadlocked = 0;

	t->nodes_num++;

	if (t->nodes_num >= t->max_nodes)
		exit_with_error("max tree nodes reached");

	add_entry_to_hash_array(t, hash, place);
}


int node_is_solved(tree *t, expansion_data *e)
{
	board b;

	bytes_to_board(e->b, b);

	if (e->depth == 0) return 0;
	// a poistion can't be solved without making any moves
	// this is for cyclic levels

	return board_is_solved(b, t->pull_mode);
}

void set_deadlock_status(tree *t, expansion_data *e)
{
	int i,place;
	node_element *node;
	move_hash_data *mh;

	if (e->node->deadlocked)
		return;

	e->node->deadlocked = 0;

	if (node_is_solved(t,e))
		return;
	
	// update the mh[i].deadlock values
	mh = t->move_hashes + e->move_hash_place;
	for (i = 0; i < e->moves_num; i++)
	{
		if (mh[i].deadlocked) continue;

		place = find_node_by_hash(t, mh[i].hash);
		if (place == -1) exit_with_error("missing node");
		node = t->nodes + place;

		if (node->deadlocked)
			mh[i].deadlocked = 1;
	}

	// update node itself

	for (i = 0; i < e->moves_num; i++)
		if (mh[i].deadlocked == 0)
			return;

	e->node->deadlocked = 1;

	if (e->father)
		set_deadlock_status(t, e->father);
}



void set_advisors_in_node(tree *t, expansion_data *e, helper *h)
{
	move moves[MAX_MOVES];
	score_element scores[MAX_MOVES];
	int already_expanded[MAX_MOVES];
	move_hash_data *mh;
	int i,place;
	board b;

	mh = t->move_hashes + e->move_hash_place;
	for (i = 0; i < e->moves_num; i++)
	{
		moves[i] = mh[i].move;
		get_score_of_hash(t, mh[i].hash, scores + i);
	}

	// determine which children are already in the tree
	// the advisors should not suggest them.
	for (i = 0; i < e->moves_num; i++)
	{
		place = find_node_by_hash(t, mh[i].hash);
		if (place == -1) exit_with_error("bad tree");
		already_expanded[i] = (t->nodes[place].expansion == -1 ? 0 : 1);
	}


	bytes_to_board(e->b, b);
	set_advisors_inner(b, &(e->node->score), e->moves_num, moves, scores, 
		t->pull_mode, t->search_mode, &(e->advisors), already_expanded, h);
}



void set_leaf_direction(tree* t, expansion_data* e)
{
	int i,place;
	move_hash_data* mh;
	mh = t->move_hashes + e->move_hash_place;

	e->leaf_direction = -1;

	if (e->node->deadlocked) return;

	for (i = 0; i < e->moves_num; i++)
	{
		place = find_node_by_hash(t, mh[i].hash);
		if (place == -1) exit_with_error("missing node");
	
		if (t->nodes[place].deadlocked) continue;

		if (t->nodes[place].expansion == -1)
		{
			e->leaf_direction = i;
			return;
		}
	}
}

void fill_expansion_structures(tree *t, expansion_data *e, helper *h)
{
	move moves[MAX_MOVES];
	UINT_64 hash;
	score_element s;
	board b,c;
	int moves_num;
	int i;

	bytes_to_board(e->b, b);
	moves_num = find_possible_moves(b, moves, t->pull_mode, &(e->has_corral), t->search_mode, h);

	e->moves_num = moves_num;

	e->move_hash_place = t->move_hashes_num;
	for (i = 0; i < moves_num; i++)
	{
		bytes_to_board(e->b, c);
		apply_move(c, moves + i, t->search_mode);
		hash = get_board_hash(c);

		t->move_hashes[t->move_hashes_num + i].move = moves[i];
		t->move_hashes[t->move_hashes_num + i].hash = hash;
		t->move_hashes[t->move_hashes_num + i].deadlocked = 0;

		if (find_node_by_hash(t, hash) == -1) // new node in tree
		{
			score_board(c, &s, t->pull_mode, t->search_mode, h);
			add_node_to_tree(t, hash, &s);
		}
	}
	t->move_hashes_num += moves_num;

	set_leaf_direction(t, e);

	if (t->move_hashes_num > t->max_move_hashes)
		exit_with_error("max move-hashes reached");
}

void set_best_move(tree *t, expansion_data *e)
{
// find the best move that was not explored yet

	int i;
	int node_place;
	move_hash_data *mh;
	node_element *n;
	int best_weight = 1000000;
	score_element best_score;

	e->best_move = -1;
	e->best_weight = 1000000;

	if (e->node->deadlocked)
		return;

	mh = t->move_hashes + e->move_hash_place;

	for (i = 0; i < e->moves_num; i++)
	{
		if (mh[i].deadlocked) continue;

		node_place = find_node_by_hash(t, mh[i].hash);
		if (node_place == -1) exit_with_error("internal error1");
		n = t->nodes + node_place;

		if (n->expansion != -1) continue;
		if (n->deadlocked)
		{
			mh[i].deadlocked = 1;
			continue;
		}

		if (mh[i].move.attr.weight > best_weight) 
			continue;

		if (mh[i].move.attr.weight < best_weight)
		{
			best_weight = mh[i].move.attr.weight;
			e->best_move = i;
			best_score = n->score;
			continue;
		}
		
		if (t->search_mode == GIRL_SEARCH) continue;
		// there's no reason to be here in girl mode unless perimeter was fonund and we switched to FESS.
		// in this case the next move is marked by a weight of -1.


		if (is_better_score(&(n->score), &best_score, t->pull_mode, t->search_mode))
		{
			best_weight = mh[i].move.attr.weight;
			e->best_move = i;
			best_score = n->score;
			continue;
		}
	}

	if (best_weight != 1000000)
	{
		e->best_score = best_score;
		e->best_weight = e->weight + best_weight;
	}
	else
		set_deadlock_status(t, e);
}

void block_existing_sons_tree(tree *t, expansion_data *e, int nodes_before_expansion)
{
	int i, place;
	node_element *node;
	move_hash_data *mh = t->move_hashes + e->move_hash_place;

	for (i = 0; i < e->moves_num; i++)
	{
		place = find_node_by_hash(t, mh[i].hash);
		if (place == -1) exit_with_error("missing node");
		node = t->nodes + place;

		if (place < nodes_before_expansion)
			mh[i].deadlocked = 1;
	}

	set_deadlock_status(t, e);
}

int board_is_in_tree(board b, tree *t)
{
	UINT_64 hash;
	int node_place;

	hash = get_board_hash(b);
	node_place = find_node_by_hash(t, hash);
	
	return (node_place == -1 ? 0 : 1);
}

void add_board_to_tree(tree *t, board b, helper *h)
{
	// this routine is used to declare a forest's roots
	UINT_64 hash;
	score_element s;

	hash = get_board_hash(b);
	score_board(b, &s, t->pull_mode, t->search_mode, h);
	add_node_to_tree(t, hash, &s);
}

void set_root(tree *t, board b, helper *h)
{
	UINT_64 hash;
	score_element s;
	expansion_data *e;
	int node_place;
	int nodes_before_expansion;

	if (t->search_mode == -1) exit_with_error("search mode not initialized");

	nodes_before_expansion = t->nodes_num;

	hash = get_board_hash(b);
	score_board(b, &s, t->pull_mode, t->search_mode, h);

	node_place = find_node_by_hash(t, hash);
	if (node_place == -1)
	{
		add_node_to_tree(t, hash, &s);
	}
	else
	{
		if (t->nodes[node_place].expansion != -1)
			exit_with_error("root is already in tree");
	}

	node_place = find_node_by_hash(t, hash);

	t->nodes[node_place].expansion = t->expansions_num;
	e = t->expansions + t->expansions_num;
	t->expansions_num++;

	e->node = t->nodes + node_place;
	e->b = t->boards + height*width*(t->expansions_num - 1);
	board_to_bytes(b, e->b);
	e->best_past = &(e->node->score);
	e->label = -1;
	e->weight = 0;
	e->depth = 0;
	e->father = NULL;
	e->best_move = -1;
	e->best_weight = 1000000;

	fill_expansion_structures(t, e, h);

	set_deadlock_status(t, e);

	// some initializations for girl mode
	e->subtree_size = 1;
	e->next_son_to_check = 0;

	if (t->search_mode == GIRL_SEARCH)
		block_existing_sons_tree(t, e, nodes_before_expansion);
}


void expand_node(tree *t, expansion_data *e, int son, helper *h)
{
	board b;
	move_hash_data *mh;
	UINT_64 hash;
	int node_place;
	node_element *n;
	expansion_data *next;
	int nodes_before_expansion;
	move move_to_play;

	nodes_before_expansion = t->nodes_num;

	mh = t->move_hashes + e->move_hash_place;
	hash = mh[son].hash;

	node_place = find_node_by_hash(t, hash);
	if (node_place == -1) exit_with_error("missing node");
	n = t->nodes + node_place;

	if (n->expansion != -1) exit_with_error("already expanded");
	if (n->deadlocked) exit_with_error("expanding a leaf which is known to be deadlocked");

	n->expansion = t->expansions_num;
	next = t->expansions + t->expansions_num;
	t->expansions_num++;

	if (t->expansions_num >= t->max_expansions)
		exit_with_error("max expansions reached");

	bytes_to_board(e->b, b);
	move_to_play = mh[son].move;
	apply_move(b, &move_to_play, t->search_mode);
	if (get_board_hash(b) != mh[son].hash) exit_with_error("hash error");

	next->node = n;
	next->b = t->boards + height*width*(t->expansions_num - 1);
	board_to_bytes(b, next->b);

	next->best_past = e->best_past;
	if (is_better_score(&(n->score), e->best_past, t->pull_mode, t->search_mode))
		next->best_past = &(n->score);

	next->label = -1;
	next->weight = e->weight + mh[son].move.attr.weight;
	next->depth  = e->depth + 1;
	next->best_weight = 1000000;
	next->best_move = -1;
	next->father = e;
	next->subtree_size = 0;

	fill_expansion_structures(t, next, h);

	set_deadlock_status(t, next);

	update_subtree_size(t, next);

	// some initializations for girl mode
	next->next_son_to_check = 0;

	if (t->search_mode == GIRL_SEARCH)
		block_existing_sons_tree(t, next, nodes_before_expansion);
}


void set_imagine_data_in_node(tree *t, expansion_data *e, helper *h)
{
	move moves[MAX_MOVES];
	score_element scores[MAX_MOVES];
	move_hash_data *mh;
	int i;
	int place;
	board b;

	if ((t->search_mode == REV_SEARCH) && (t->pull_mode))
	{
		set_rev_distance(t, e, h, moves, scores); 
		// moves and scores are passed to avoid stack overflow 
		// (no need to allocate them again in set_rev_distance)
		return;
	}

	if (t->pull_mode) return;
	if (t->search_mode == GIRL_SEARCH) return;
	if (t->search_mode == NAIVE_SEARCH) return;
	if (t->search_mode == REV_SEARCH) return;

	mh = t->move_hashes + e->move_hash_place;
	for (i = 0; i < e->moves_num; i++)
	{
		moves[i] = mh[i].move;
		get_score_of_hash(t, mh[i].hash, scores + i);
	}
	
	bytes_to_board(e->b, b);
	compute_imagine_distance(b, &(e->node->score), e->moves_num, moves, scores, t->search_mode, h);

	for (i = 0; i < e->moves_num; i++)
	{
		place = find_node_by_hash(t, mh[i].hash);
		if (place == -1) exit_with_error("missing node");
		t->nodes[place].score.dist_to_imagined = scores[i].dist_to_imagined;
	}
}


void set_weight_to_node_moves(tree *t, expansion_data *e, helper *h)
{
	int i;
	move_hash_data *mh;

	mh = t->move_hashes + e->move_hash_place;

	for (i = 0; i < e->moves_num; i++)
	{
		mh[i].move.attr.advisor = is_an_advisor(&(e->advisors), i);
		mh[i].move.attr.weight = attr_to_weight(&mh[i].move.attr);
	}

	if (h->weighted_search == 0)
		for (i = 0; i < e->moves_num; i++)
			mh[i].move.attr.weight = 0;

	if (e->moves_num <= 2) 
		mh[0].move.attr.weight = 0;

	if (h->perimeter_found)
	{
		for (i = 0; i < e->moves_num; i++)
			mh[i].move.attr.weight = 1;
		// when loosing the perimeter, prefer early nodes
	}

}


void set_advisors_and_weights_to_last_node(tree *t, helper *h)
{
	expansion_data *e;
	
	if (t->expansions_num <= 0) exit_with_error("internal error");
	e = t->expansions + t->expansions_num - 1;

	if (t->search_mode == NAIVE_SEARCH) 
		set_naive_data_in_node(t, e, h);

	if ((t->search_mode == MAX_DIST_SEARCH) || 
		(t->search_mode == MAX_DIST_SEARCH2) ||
		(t->search_mode == SNAIL_SEARCH))
		set_max_dist_data_in_node(t, e, h);

	set_imagine_data_in_node(t, e, h);
	// need to do it here before computing imagine advisor

	set_advisors_in_node(t, e, h);
	set_weight_to_node_moves(t, e, h);

	set_best_move(t,e);
}

int best_move_is_available(tree *t, expansion_data *e)
{
	// check that the move was not deadlocked or expanded
	move_hash_data *mh;
	int place;
	node_element *n;
	int son;

	// verify that the move is free
	mh = t->move_hashes + e->move_hash_place;

	son = e->best_move;

	if (son == -1) return 0;

	if (mh[son].deadlocked)
		return 0;

	place = find_node_by_hash(t, mh[son].hash);
	if (place == -1) exit_with_error("missing node");
	n = t->nodes + place;

	if (n->deadlocked)
	{
		mh[son].deadlocked = 1;
		return 0;
	}

	if (n->expansion != -1)
		return 0;

	return 1;
}

int best_move_in_tree(tree *t, int label, int *best_pos, int *best_son)
{
	expansion_data *e;
	*best_son = -1;
	*best_pos = -1;
	int q_move;
	int weight = 1000000;

	do	
	{
		q_move = extarct_max(t->queues + label);

		if (q_move == -1)
			return 1000000; // no moves - weight is 1000000

		e = t->expansions + q_move;

		if (e->best_move == -1)
			exit_with_error("should have a move");

		if (best_move_is_available(t, e))
		{
			*best_pos = q_move;
			*best_son = e->best_move;
			weight = e->best_weight;
		}

		set_best_move(t, e);

		if (e->best_move != -1)
		{
//			printf("ADDING    node %d to   queue %d\n", q_move, label);
			heap_insert(t->queues + label, q_move);
		}

//		verify_heap(t->queues + label);

	} 
	while (*best_son == -1);

	return weight;
}



void set_node_as_deadlocked(tree *t, int pos, int son)
{
	expansion_data *e;
	move_hash_data *mh;
	UINT_64 hash;
	int place;
	node_element *node;

	e = t->expansions + pos;
	mh = t->move_hashes + e->move_hash_place;
	hash = mh[son].hash;
	place = find_node_by_hash(t, hash);

	if (place == -1) exit_with_error("missing node");
	node = t->nodes + place;

	if (node->deadlocked) exit_with_error("node is already deadlocked");
	node->deadlocked = 1;

	mh[son].deadlocked = 1;

	set_deadlock_status(t, e);
}

move get_move_to_node(tree *t, expansion_data *e)
{
	expansion_data *f;
	move_hash_data *mh;
	int i;

	if (e->father == 0) exit_with_error("no father");
	f = e->father;
	mh = t->move_hashes + f->move_hash_place;

	for (i = 0; i < f->moves_num; i++)
		if (mh[i].hash == e->node->hash)
			return mh[i].move;

	exit_with_error("missing move");
	return mh[0].move;
}

void remove_son_from_tree(tree *t, expansion_data *e, int son)
{
	move_hash_data *mh;

	mh = t->move_hashes + e->move_hash_place;
	mh[son].deadlocked = 1;
	set_deadlock_status(t, e);
}


void add_label_to_last_node(tree *t, int label, helper *h)
{
	int last_node = t->expansions_num - 1;

	if ((label < 0) || (label >= MAX_TREE_LABELS))
		exit_with_error("label overflow");

	t->expansions[last_node].label = label;

	if (label >= t->labels_num)
	{
		if (label > t->labels_num)
			exit_with_error("label jumped?");

		if (label == t->labels_num)
		{
//			printf("Adding new queue: %d\n", label);
			init_queue(t->queues + label, t);
			t->labels_num++;
		}
	}

	if ((t->search_mode == GIRL_SEARCH) && (h->perimeter_found == 0))
		return;

	if (h->perimeter_found)
		set_best_move(t, t->expansions + last_node); 
	// mainly for girl mode, advisors,weights and computing best move are not called

	if (t->expansions[last_node].best_move != -1)
		heap_insert(t->queues + label, last_node);
}

void insert_all_tree_nodes_to_queues(tree *t)
{
	int i;
	expansion_data *e;

	// this routine is called once the perimeter is hit and we switch from girl to FESS 

	if (t->search_mode != GIRL_SEARCH) exit_with_error("shouldn't be here");

	for (i = 0; i < t->labels_num; i++)
		if (t->queues[i].heap_size != 0)
			exit_with_error("queue is active");

	for (i = 0; i < t->expansions_num; i++)
	{
		e = t->expansions + i;
		set_best_move(t, e);
		if (e->best_move != -1)
			heap_insert(t->queues + e->label, i);
	}
}
	

void replace_in_queue(tree *t, expansion_data *e)
{
	// this routine is called when the node changes by perimeter.cpp
	int label = e->label;

	remove_value_from_queue(t->queues + label, (int)(e - t->expansions));

	if (e->best_move != -1)
		heap_insert(t->queues + label, (int)(e - t->expansions));
}

expansion_data *last_expansion(tree *t)
{
	return t->expansions + t->expansions_num - 1;
}

move *get_node_move(tree *t, expansion_data *e, int son)
{
	move_hash_data *mh = t->move_hashes + e->move_hash_place;
	return &(mh[son].move);
}

UINT_64 get_node_hash(tree *t, expansion_data *e, int son)
{
	move_hash_data *mh = t->move_hashes + e->move_hash_place;
	return mh[son].hash;
}


int update_best_so_far(expansion_data **best_so_far, expansion_data *new_node, int pull_mode, int search_mode)
{
	expansion_data *bsf;

	if (*best_so_far == 0)
	{
		*best_so_far = new_node;
		return 1;
	}

	bsf = *best_so_far;

	if (search_mode == GIRL_SEARCH)
	{
		if (is_better_score(&new_node->node->score, &(bsf->node->score), pull_mode, search_mode))
		{
			*best_so_far = new_node;
			return 1;
		}

		return 0;
	}

	if (is_better_score(new_node->best_past, bsf->best_past, pull_mode, search_mode))
	{
		*best_so_far = new_node;
		return 1;
	}

	return 0;
}

expansion_data *best_node_in_tree(tree *t)
{
	int i;
	expansion_data *e = t->expansions;
	expansion_data *res = 0;

	for (i = 0; i < t->expansions_num; i++)
	{
		if (e[i].node->deadlocked) continue;

		if (res == 0)
		{
			res = e + i;
			continue;
		}

		if (e[i].depth == 0) // do at least one move
			continue;

		if ((res->depth == 0) && (e[i].depth != 0))
		{
			res = e + i;
			continue;
		}

		// prefer nodes without a corral tension
		if (e[i].has_corral)
			continue;

		if ((res->has_corral) && (e[i].has_corral == 0))
		{
			res = e + i;
			continue;
		}


		if (is_better_score(&(e[i].node->score), &(res->node->score), t->pull_mode, t->search_mode))
			res = e + i;
	}
	return res;
}

int tree_has_unexpanded_nodes(tree *t)
{
	int i,j,place;
	expansion_data *e;
	move_hash_data *mh;
	node_element *n;

	for (i = 0; i < t->expansions_num; i++)
	{
		e = t->expansions + i;
		mh = t->move_hashes + e->move_hash_place;

		if (e->node->deadlocked) continue;

		for (j = 0; j < e->moves_num; j++)
		{
			place = find_node_by_hash(t, mh[j].hash);
			if (place == -1) exit_with_error("missing node");
			n = t->nodes + place;

			if (n->deadlocked) continue;
			if (n->expansion == -1) return 1;
		}
	}
	return 0;
}


void set_subtree_as_deadlocked(tree* t, expansion_data* e)
{
	move_hash_data* mh;
	int i,place;
	node_element* n;

	mh = t->move_hashes + e->move_hash_place;

	if (e->node->deadlocked) return;
	e->node->deadlocked = 1; // prevent loops in the recursion

	for (i = 0; i < e->moves_num; i++)
	{
		mh[i].deadlocked = 1;

		place = find_node_by_hash(t, mh[i].hash);
		if (place == -1) exit_with_error("missing node");
		n = t->nodes + place;

		if (n->expansion == -1)
			n->deadlocked = 1;
		else
			set_subtree_as_deadlocked(t, t->expansions + n->expansion);
	}

	e->node->deadlocked = 0; // otherwise set_deadlock_status returns without updating father
	set_deadlock_status(t, e);
	
	if (e->node->deadlocked == 0) exit_with_error("dl bug");
}