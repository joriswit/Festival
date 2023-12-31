// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include "global.h"
typedef struct hungarian_solution
{
	int V[MAX_BOXES];
	int U[MAX_BOXES]; 
	int match_v[MAX_BOXES];
	int match_u[MAX_BOXES];
	int weight;
} hungarian_solution;

typedef struct
{
	box_mat prev_c;
	hungarian_solution prev_sol;
	int prev_n;
} hungarian_cache;

void solve_hungarian(int n, box_mat cost, hungarian_solution *sol,
	hungarian_cache *hc);

hungarian_cache *allocate_hungarian_cache();


