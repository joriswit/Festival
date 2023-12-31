// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include "global.h"

int get_from_deadlock_cache(UINT_64 hash, int pull_mode, char alg);
void insert_to_deadlock_cache(UINT_64 hash, int res, int pull_mode, char alg);
void clear_deadlock_cache();
int get_queries_num(UINT_64 hash, int pull_mode, char alg);
void update_queries_counter(UINT_64 hash, int pull_mode, char alg);


void allocate_deadlock_cache();
void free_deadlock_cache();