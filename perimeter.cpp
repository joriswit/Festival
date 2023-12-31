// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include <stdio.h>
#include <stdlib.h>

#ifdef THREADS
#include <pthread.h>
#endif

#include "global.h"
#include "util.h"
#include "perimeter.h"
#include "io.h"

typedef struct
{
	UINT_64 hash;
	UINT_16 depth;
	UINT_8  side;
	UINT_8  alg;
} perimeter_entry;

perimeter_entry *perimeter;
int log_perimeter_size;
unsigned int perimeter_mask;
int total_perimeter_entries;


void set_perimeter_size()
{
	if (cores_num == 1) log_perimeter_size = 25;
	if (cores_num == 2) log_perimeter_size = 26;
	if (cores_num == 4) log_perimeter_size = 27;
	if (cores_num == 8) log_perimeter_size = 28;

#ifdef VISUAL_STUDIO
	log_perimeter_size = 24;
#endif

	log_perimeter_size += extra_mem;
}

void allocate_perimeter()
{
	size_t size;

	set_perimeter_size();

	size = sizeof(perimeter_entry) * (1ULL << log_perimeter_size);

	if (verbose >= 4)
	{
		printf("Allocating %12llu bytes for perimeter. ", (UINT_64)size);
		printf("%d entries\n", 1 << log_perimeter_size);
	}
	perimeter = (perimeter_entry *)malloc((size_t)size);

	perimeter_mask = (1 << log_perimeter_size) - 1;

	if (perimeter == 0)
		exit_with_error("can't allocate perimeter");
}

void free_perimeter()
{
	free(perimeter);
}


void clear_perimeter()
{
	int i;
	perimeter_entry* p;

	for (i = 0; i < (1 << log_perimeter_size); i++)
	{
		p = perimeter + i;
		p->hash = 0;
		p->depth = 0;
		p->side = 2;
		p->alg = 255;
	}
	total_perimeter_entries = 0;
}

perimeter_entry* get_entry_for_hash(UINT_64 hash)
{
	UINT_64 index;

	index = hash >> (64 - log_perimeter_size);

	while (perimeter[index].hash != 0)
	{
		if (perimeter[index].hash == hash) break;
		index = (index + 1) & perimeter_mask;
	}
	return perimeter + index;
}



#ifdef THREADS
pthread_mutex_t perimeter_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

UINT_64 get_hash_without_bases(tree* t, expansion_data* e, int son)
{
	// use son=-1 for the node itself
	board b;
	int place;
	move_hash_data* mh;
	node_element* node;

	if (e->node->score.boxes_in_level != global_boxes_in_level)
		return 0xffffffff;

	if (son == -1)
	{
		bytes_to_board(e->b, b);
		clear_bases_inplace(b);
		return get_board_hash(b);
	}

	mh = t->move_hashes + e->move_hash_place;

	place = find_node_by_hash(t, mh[son].hash);
	if (place == -1) exit_with_error("missing node");
	node = t->nodes + place;

	if (node->score.boxes_in_level != global_boxes_in_level)
		return 0xffffffff;

	bytes_to_board(e->b, b);
	apply_move(b, &(mh[son].move), NORMAL);

	clear_bases_inplace(b);
	return get_board_hash(b);
}

void update_perimeter(perimeter_entry* p, int depth, int pull_mode, int alg)
{
	// update perimeter hash with a better (usually shallower) entry

	// dragonfly nodes must not be explored twice. depth can be updated.
	if (p->alg == DRAGONFLY)
	{
		if (pull_mode && (depth < p->depth))
			p->depth = depth;
		return;
	}

	if (p->side == pull_mode) // same direction
	{
		if (depth < p->depth)
		{
			p->depth = depth;
			p->alg = alg;
		}
		return;
	}

	// so different directions. prioritize pull positions

	if (p->side == 1) return;

	p->side = pull_mode;
	p->depth = depth;
	p->alg = alg;
}


int perimeter_is_full()
{
	int limit = 1 << (log_perimeter_size - 1);
	if (total_perimeter_entries < limit) return 0;
	if (verbose >= 4) exit_with_error("perimeter is full\n");
	return 1;
}

int insert_node_into_perimeter(tree *t, expansion_data *e, int with_sons)
{
	UINT_64 hash;
	move_hash_data *mh;
	int j;
	int perimeter_entries = 0;
	perimeter_entry *p;


	if (perimeter_is_full()) return 0;

	if (t->search_mode == BASE_SEARCH)
		hash = get_hash_without_bases(t, e, -1);
	else
		hash = e->node->hash;

#ifdef THREADS
	if (cores_num > 1) pthread_mutex_lock(&perimeter_mutex);
#endif

	p = get_entry_for_hash(hash);

	if (p->hash != hash)
	{
		p->hash = hash;
		p->depth = e->depth;
		p->side = t->pull_mode;
		p->alg = t->search_mode;
		perimeter_entries++;
		total_perimeter_entries++;
	}
	else // already in perimeter - check if depth can be improved
	{
		update_perimeter(p, e->depth, t->pull_mode, t->search_mode);
		perimeter_entries++;
	}
#ifdef THREADS
	if (cores_num > 1) pthread_mutex_unlock(&perimeter_mutex);
#endif

	if (with_sons == 0) 
		return perimeter_entries;

	mh = t->move_hashes + e->move_hash_place;

#ifdef THREADS
	if (cores_num > 1) pthread_mutex_lock(&perimeter_mutex);
#endif

	for (j = 0; j < e->moves_num; j++)
	{
		if (t->search_mode == BASE_SEARCH)
			hash = get_hash_without_bases(t, e, j);
		else
			hash = mh[j].hash;

		p = get_entry_for_hash(hash);

		if (p->hash != hash)
		{
			p->hash  = hash;
			p->depth = e->depth + 1;
			p->side  = t->pull_mode;
			p->alg   = t->search_mode;
			perimeter_entries++;
			total_perimeter_entries++;
		}
		else // already in perimeter - check if depth can be improved
		{
			update_perimeter(p, e->depth + 1, t->pull_mode, t->search_mode);
			perimeter_entries++;
		}
	}

#ifdef THREADS
	if (cores_num > 1) pthread_mutex_unlock(&perimeter_mutex);
#endif

	return perimeter_entries;
}



int is_in_perimeter(UINT_64 hash, UINT_16 *depth, UINT_8 side)
{
	perimeter_entry *p;
	int res = 0;


#ifdef THREADS
	if (cores_num > 1) pthread_mutex_lock(&perimeter_mutex);
#endif

	p = get_entry_for_hash(hash);

	if (p->hash == hash)
	{
		if (p->side == side)
		{
			*depth = p->depth;
			res = 1;
		}
	}

#ifdef THREADS
	if (cores_num > 1) pthread_mutex_unlock(&perimeter_mutex);
#endif

	return res;
}

int cyclic_level_exception(tree *t)
{
	// A root in a cyclic level / pull mode can be both deadlocked or not.
	// suppose that zone 1 is deadlocked as a root, but when we start at zone 2
	// and reach it, this is solving the level.
	// this means that in cycle levels, pull mode might not find a solution (since it is
	// marked as deadlock). That's OK, we'll find it in the forward search.
	if ((t->pull_mode) && (is_cyclic_level()))
		return 1;
	else
		return 0;
}

int check_if_perimeter_reached(tree *t, helper *h)
{
	int i;
	int best_depth = 1000000;
	int best_son = -1;
	UINT_16 depth;
	int e_depth = 0;

	expansion_data *e;
	move_hash_data *mh;
	node_element *n;

	e = t->expansions + t->expansions_num - 1;
	mh = t->move_hashes + e->move_hash_place;

	if (e->weight < 0) // already a perimeter node
		e_depth = 2000 + e->weight;

	for (i = 0; i < e->moves_num; i++)
	{
		if (is_in_perimeter(mh[i].hash, &depth, 1 - t->pull_mode) == 0)
			continue;

		// do not go back in depth
		if ((e->weight < 0) && (depth >= e_depth)) continue;

		if (mh[i].deadlocked)
		{
			if (t->search_mode == GIRL_SEARCH) continue;
			// the move can be "deadlocked" because it is also in another branch
			
			if (cyclic_level_exception(t) == 0)
				exit_with_error("perimeter move is deadlocked1");
		}

		n = t->nodes + find_node_by_hash(t, mh[i].hash);
		if (n->deadlocked) 
			if (cyclic_level_exception(t) == 0)
				exit_with_error("perimeter move is deadlocked2");

		if (n->expansion != -1) continue; // move is expanded and can't be chosen
		// this is somewhat problematic in cyclic levels+pull mode, because the solution
		// position could be expanded. We'll just wait for the following forward mode.

		if (depth < best_depth)
		{
			best_depth = depth;
			best_son = i;
		}
	}

	if (h->perimeter_found)
		if (verbose >= 5)
			printf("best son is: %d\n", best_son);


	if (best_son != -1)
	{
		if ((verbose >= 4) && (h->perimeter_found == 0))
		{
			if (t->pull_mode == 0)
				print_in_color("< Perimeter hit!>\n", "red");
			else
				print_in_color("< Perimeter hit!!!>\n", "blue");
		}

		if ((h->perimeter_found == 0) && (t->search_mode == GIRL_SEARCH))
			insert_all_tree_nodes_to_queues(t); // switching to FESS

		e->weight = -2000 + (best_depth + 1); // +1 because of the played move
		mh[best_son].move.attr.weight = -1;
		
		set_best_move(t,e);
		replace_in_queue(t, e);

		h->perimeter_found = 1;

		return 1;
	}

	return 0;
}


void dragonfly_mark_visited_hashes(UINT_64 *hashes, int n, int *visited)
{
	int i;

	perimeter_entry* p;

	for (i = 0; i < n; i++)
	{
		p = get_entry_for_hash(hashes[i]);

		if ((p->hash == hashes[i]) && (p->alg == DRAGONFLY))
		{
			visited[i] = 1;
			continue;
		}

		
		if ((p->hash == hashes[i]) && (p->side == 0))
		{
			// a perimeter hit with a forward search
			visited[i] = -2000 + p->depth;
			continue;
		}
		

		visited[i] = 0;
	}
}

void dragonfly_update_perimeter(UINT_64* hashes, int* visited, int n, int depth)
{
	int i;
	perimeter_entry* p;

#ifdef THREADS
	if (cores_num > 1) pthread_mutex_lock(&perimeter_mutex);
#endif

	for (i = 0; i < n; i++)
	{
		if (visited[i] == 1) continue;

		p = get_entry_for_hash(hashes[i]);

		if ((p->hash == hashes[i]) && (p->side == 1)) // already in perimeter by another alg
		{
			if (depth < p->depth)
				p->depth = depth;

			p->alg = DRAGONFLY;
			continue;
		}

		p->hash = hashes[i];
		p->side = 1;
		p->depth = depth;
		p->alg = DRAGONFLY;

		total_perimeter_entries++;
	}

#ifdef THREADS
	if (cores_num > 1) pthread_mutex_unlock(&perimeter_mutex);
#endif
}