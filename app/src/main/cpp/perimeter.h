// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include "tree.h"
#include "helper.h"

void allocate_perimeter();
void free_perimeter();

int insert_node_into_perimeter(tree *t, expansion_data *e, int with_sons);

int is_in_perimeter(UINT_64 hash, UINT_16 *depth, UINT_8 side);

int check_if_perimeter_reached(tree *t, helper *h);

void clear_perimeter();

int perimeter_is_full();


void dragonfly_mark_visited_hashes(UINT_64* hashes, int n, int* visited);
void dragonfly_update_perimeter(UINT_64* hashes, int* visited, int n, int depth);