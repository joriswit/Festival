// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include <stdlib.h>
#include <stdio.h>

#include "deadlock_cache.h"
#include "util.h"
#include "global.h"

#ifdef THREADS
#include <pthread.h>
#endif

int log_deadlock_cache = 22;

unsigned int deadlock_cache_mask;
int total_deadlock_cache_entries;

typedef struct cache_entry
{
	UINT_64 hash;
	int queries;
	char result;
	char pull_mode;
	char alg;
} cache_entry;

cache_entry *deadlock_cache;




void set_log_deadlock_cache()
{
	if (cores_num == 1) log_deadlock_cache = 22 + extra_mem;
	if (cores_num == 2) log_deadlock_cache = 23 + extra_mem;
	if (cores_num == 4) log_deadlock_cache = 24 + extra_mem;
	if (cores_num == 8) log_deadlock_cache = 25 + extra_mem;
}


void allocate_deadlock_cache()
{
	set_log_deadlock_cache();
	size_t size;

	size = (1LL << log_deadlock_cache) * sizeof(cache_entry);
	deadlock_cache = (cache_entry*)malloc(size);

	if (verbose >= 4)
		printf("Allocating %12llu bytes for %d deadlock cache\n",
			size, 1 << log_deadlock_cache);

	if (deadlock_cache == 0)
		exit_with_error("can't allocate deadlock cache");

	deadlock_cache_mask = (1 << log_deadlock_cache) - 1;
	total_deadlock_cache_entries = 0;
}

void free_deadlock_cache()
{
	free(deadlock_cache);
}

void clear_deadlock_cache()
{
	int i;
	for (i = 0; i < (1 << log_deadlock_cache); i++)
	{
		deadlock_cache[i].hash = 0;
		deadlock_cache[i].result = -1;
		deadlock_cache[i].queries = 0;
		deadlock_cache[i].pull_mode = -1;
		deadlock_cache[i].alg = -1;
	}

	total_deadlock_cache_entries = 0;
}

#ifdef THREADS
pthread_mutex_t deadlock_cache_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif


cache_entry* get_deadlock_cache_entry(UINT_64 hash, int pull_mode, char alg, int* match)
{
	int index;
	cache_entry* e;

	*match = 0;
	index = hash & deadlock_cache_mask;

	while (deadlock_cache[index].hash != 0)
	{
		e = deadlock_cache + index;

		if ((e->hash == hash) && (e->pull_mode == pull_mode) && (e->alg == alg))
		{
			*match = 1;
			return e;
		}

		index = (index + 1) & deadlock_cache_mask;
	}

	return deadlock_cache + index;
}




int get_from_deadlock_cache(UINT_64 hash, int pull_mode, char alg)
{
	int res;
	int match;
	cache_entry* e;

#ifdef THREADS
	if (cores_num > 1) pthread_mutex_lock(&deadlock_cache_mutex);
#endif

	e = get_deadlock_cache_entry(hash, pull_mode, alg, &match);

	if (match)
		res = e->result;
	else
		res = -1;

#ifdef THREADS
	if (cores_num > 1) pthread_mutex_unlock(&deadlock_cache_mutex);
#endif

	return res;
}

int deadlock_cache_is_full()
{
	int limit = 1 << (log_deadlock_cache - 1);
	if (total_deadlock_cache_entries < limit) return 0;
	if (verbose >= 4) exit_with_error("deadlock cache is full !!!\n");
	return 1;
}

void insert_to_deadlock_cache(UINT_64 hash, int res, int pull_mode, char alg)
{
	int match;
	cache_entry* e;

	if (deadlock_cache_is_full()) return;

#ifdef THREADS
	if (cores_num > 1) pthread_mutex_lock(&deadlock_cache_mutex);
#endif

	e = get_deadlock_cache_entry(hash, pull_mode, alg, &match);

	if (match) // already in cache
	{
		if ((e->result ^ res) == 1)
			exit_with_error("different cache value!");

		if (e->result == 2)
			e->result = res;
	}
	else
	{
		e->hash = hash;
		e->pull_mode = pull_mode;
		e->alg = alg;
		e->result = res;
		e->queries = 0;

		total_deadlock_cache_entries++;
	}

#ifdef THREADS
	if (cores_num > 1) pthread_mutex_unlock(&deadlock_cache_mutex);
#endif
}

int get_queries_num(UINT_64 hash, int pull_mode, char alg)
{
	int match;
	cache_entry* e;

	e = get_deadlock_cache_entry(hash, pull_mode, alg, &match);

	if (match == 0) return 0;

	return e->queries;
}


void update_queries_counter(UINT_64 hash, int pull_mode, char alg)
{
	int match;
	cache_entry* e;

	e = get_deadlock_cache_entry(hash, pull_mode, alg, &match);

	if (match == 0) return;

	e->queries++;
}