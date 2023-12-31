// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include <stdio.h>
#include <stdlib.h>

#include "hungarian.h"
#include "global.h"
#include "util.h"

typedef struct
{
	box_mat c;
	int n;

	int V[MAX_BOXES];
	int U[MAX_BOXES];
	int matched_v[MAX_BOXES];
	int matched_u[MAX_BOXES];
	int match_size;

	// for BFS
	int src_of_v[MAX_BOXES];
	int src_of_u[MAX_BOXES];
	int visited_v[MAX_BOXES];
	int visited_u[MAX_BOXES];

	int v_list[MAX_BOXES];
	int v_list_num;
	int u_list[MAX_BOXES];
	int u_list_num;
} hungarian_data;

hungarian_cache *allocate_hungarian_cache()
{
	hungarian_cache *hc;
	hc = (hungarian_cache*)malloc(sizeof(hungarian_cache));
	if (hc == NULL) exit_with_error("can't alloc");
	hc->prev_n = -1;
	return hc;
}

void set_hungarian_problem(int size, box_mat mat, hungarian_data *hd)
{
	int i, j, n;

	n = hd->n = size;
	for (i = 0; i < n; i++)
		for (j = 0; j < n; j++)
			hd->c[i][j] = mat[i][j];
}

void print_cost_matrix(hungarian_data *hd)
{
	int i, j;
	int n = hd->n;

	for (i = 0; i < n; i++)
	{
		for (j = 0; j < n; j++)
			printf("%3d ", hd->c[i][j]);
		printf("\n");
	}
	printf("\n");
}


void init_U_V(hungarian_data *hd)
{
	int min = 0;
	int i, j;
	int n = hd->n;
	int *V = hd->V;
	int *U = hd->U;

	for (i = 0; i < n; i++)
	{
		min = 10000000;
		for (j = 0; j < n; j++)
		{
			if (hd->c[i][j] < min)
				min = hd->c[i][j];
		}
		V[i] = min;
	}

	for (j = 0; j < n; j++)
	{
		min = 10000000;
		for (i = 0; i < n; i++)
		{
			if ((hd->c[i][j] - V[i]) < min)
				min = (hd->c[i][j] - V[i]);
		}
		U[j] = min;
	}

}



void init_match(hungarian_data *hd)
{
	int i,j,n;
	int *matched_v = hd->matched_v;
	int *matched_u = hd->matched_u;
	int *V = hd->V;
	int *U = hd->U;

	n = hd->n;
	hd->match_size = 0;

	for (i = 0; i < n; i++)
	{
		matched_v[i] = -1;
		matched_u[i] = -1;
	}

	for (i = 0; i < n; i++)
	{
		for (j = 0; j < n; j++)
		{
			if (matched_u[j] != -1) continue;

			if (hd->c[i][j] == (V[i] + U[j]))
			{
				matched_v[i] = j;
				matched_u[j] = i;
				hd->match_size++;
//				printf("added %d %d\n", i, j);
				break; // j iteration
			}
		}
	}


}


void init_hungarian_bfs(hungarian_data *hd)
{
	int i;
	int n = hd->n;

	int *src_of_v = hd->src_of_v;
	int *src_of_u = hd->src_of_u;
	int *visited_v = hd->visited_v;
	int *visited_u = hd->visited_u;
	int *matched_v = hd->matched_v;
	int *matched_u = hd->matched_u;
	int *v_list = hd->v_list;
	int *u_list = hd->u_list;

	hd->v_list_num = 0;

	for (i = 0; i < n; i++)
	{
		src_of_v[i] = -1;
		src_of_u[i] = -1;
		visited_v[i] = 0;
		visited_u[i] = 0;
	}

	for (i = 0; i < n; i++)
	{
		if (matched_v[i] == -1)
		{
			v_list[(hd->v_list_num)++] = i;
			visited_v[i] = 1;
		}
	}
}


int bfs_v_side(hungarian_data *hd)
{
	int t, i,j;
	int changed = 0;

	int *src_of_u = hd->src_of_u;
	int *visited_u = hd->visited_u;
	int *u_list = hd->u_list;
	int *v_list = hd->v_list;
	int n = hd->n;
	int *V = hd->V;
	int *U = hd->U;

	hd->u_list_num = 0;

	for (t = 0; t < hd->v_list_num; t++)
	{
		i = v_list[t];

		for (j = 0; j < n; j++)
		{
			if (visited_u[j]) continue;

			if ((V[i] + U[j]) == hd->c[i][j])
			{
				visited_u[j] = 1;
				src_of_u[j] = i;
				u_list[hd->u_list_num++] = j;
				changed = 1;
			}
		}
	}
	return changed;
}

int bfs_u_side(hungarian_data *hd)
{
	int i, j, t;
	int changed = 0;

	int *src_of_v = hd->src_of_v;
	int *visited_v = hd->visited_v;
	int *u_list = hd->u_list;
	int *v_list = hd->v_list;
	int n = hd->n;
	int *V = hd->V;
	int *U = hd->U;
	int *matched_u = hd->matched_u;

	hd->v_list_num = 0;
	for (t = 0; t < hd->u_list_num; t++)
	{
		j = u_list[t];

		if (matched_u[j] == -1)
			continue;

		i = matched_u[j];

		if (visited_v[i] == 1)
			continue;

		v_list[(hd->v_list_num)++] = i;
		visited_v[i] = 1;
		src_of_v[i] = j;
		changed = 1;
	}
	return changed;
}

void hungarian_bfs(int side, hungarian_data *hd) // v = side 0
{
	int res;

	do
	{
		if (side == 0)
		{
			if (hd->v_list_num == 0) exit_with_error("bad hungarian v side\n");
			res = bfs_v_side(hd);
		}
		else
		{
			if (hd->u_list_num == 0) exit_with_error("bad hungarian u side\n");
			res = bfs_u_side(hd);
		}
		side = 1 - side;
	} 
	while (res);
}


int try_to_increase_match(hungarian_data *hd)
{
	int i, j;
	int n = hd->n;

	int *src_of_v = hd->src_of_v;
	int *src_of_u = hd->src_of_u;
	int *matched_v = hd->matched_v;
	int *matched_u = hd->matched_u;
	int *visited_u = hd->visited_u;

	for (j = 0; j < n; j++)
	{
		if (matched_u[j] != -1) continue;
		if (visited_u[j] == 0) continue;

//		printf("found a new vertex in u: %d\n", j);

		while (j != -1)
		{
			i = src_of_u[j];
			matched_u[j] = i;
			matched_v[i] = j;
			j = src_of_v[i];
		}
		return 1;
	}
	return 0;
}


void add_new_edges(hungarian_data *hd)
{
	int i, j;
	int min = 10000000;
	int delta;
	int n = hd->n;

	int *visited_v = hd->visited_v;
	int *visited_u = hd->visited_u;
	int *V = hd->V;
	int *U = hd->U;
	int *src_of_u = hd->src_of_u;
	int *u_list = hd->u_list;

	hd->u_list_num = 0;

	// determine delta
	for (i = 0; i < n; i++)
	{
		if (visited_v[i] == 0) continue;

		for (j = 0; j < n; j++)
		{
			if (visited_u[j] == 1) continue;

			delta = hd->c[i][j] - V[i] - U[j];
			if (delta < min)
				min = delta;
		}
	}

	delta = min;

	// update weights
	for (i = 0; i < n; i++)
	{
		if (visited_v[i])
			V[i] += delta;
		if (visited_u[i])
			U[i] -= delta;
	}

	// add vertices to U using new edges
	for (j = 0; j < n; j++)
	{
		if (visited_u[j]) continue;

		for (i = 0; i < n; i++)
		{
			if (visited_v[i] == 0) continue;

			if ((V[i] + U[j]) == hd->c[i][j])
			{
				visited_u[j] = 1;
				src_of_u[j] = i;
				break;
			}
		}  // i

		if (visited_u[j])
			u_list[(hd->u_list_num)++] = j;
	} // j
}


void verify_hungarian_invariant(hungarian_data *hd)
{
	int i, j;
	int n = hd->n;

	for (i = 0; i < n; i++)
	for (j = 0; j < n; j++)
		if ((hd->V[i] + hd->U[j]) > hd->c[i][j])
		{
			exit_with_error("invariant failed\n");
		}
}

void add_edge_to_match(hungarian_data *hd)
{
	int res;

	init_hungarian_bfs(hd);

	hungarian_bfs(0, hd); // side = v
	// do several v-u iterations, starting from v

	while (1)
	{
		res = try_to_increase_match(hd);
		if (res) return;

		add_new_edges(hd);
//		verify_hungarian_invariant();

		hungarian_bfs(1, hd); // side = u
	}
}


void verify_hungarian_solution(hungarian_data *hd)
{
	int i,j;

	verify_hungarian_invariant(hd);

	// confirm match

	for (i = 0; i < hd->n; i++)
	{
		j = hd->matched_v[i];
		if (j == -1) exit_with_error("not perm");
		if (hd->matched_u[j] != i) exit_with_error("not perm");

		if ((hd->V[i] + hd->U[j]) != hd->c[i][j])
			exit_with_error("match problem");
	}
}

int is_match_full(hungarian_data *hd)
{
	int i;
	int n = hd->n;

	for (i = 0; i < n; i++)
		if (hd->matched_v[i] == -1)
			return 0;
	return 1;
}

void do_hungarian_iterations(hungarian_data *hd)
{
	while (is_match_full(hd) == 0)
	{
		add_edge_to_match(hd);
		verify_hungarian_invariant(hd);
	}

	verify_hungarian_solution(hd);
}

void print_hungarian_value(hungarian_data *hd)
{
	int sum = 0;
	int i;
	int n = hd->n;

	for (i = 0; i < n; i++)
	{
		printf("match %d : %d\n", i, hd->matched_v[i]);
		sum += hd->c[i][hd->matched_v[i]];
	}

	printf("sum= %d\n", sum);
}

void set_hungarian_solution(hungarian_solution *sol, hungarian_data *hd)
{
	int i;
	sol->weight = 0;
	
	for (i = 0; i < hd->n; i++)
	{
		sol->match_v[i] = hd->matched_v[i];
		sol->match_u[i] = hd->matched_u[i];
		sol->U[i] = hd->U[i];
		sol->V[i] = hd->V[i];
		sol->weight += hd->c[i][hd->matched_v[i]];
	}
}

int did_one_row_change(int size, int *row, box_mat c,
	hungarian_cache *hc)
{
	int i, j;
	int row_changed[MAX_BOXES];
	int total = 0;

	if (size != hc->prev_n) return 0;

	for (i = 0; i < size; i++)
	{
		for (j = 0; j < size; j++)
		{
			if (c[i][j] != hc->prev_c[i][j])
				break;
		}
		row_changed[i] = (j == size ? 0 : 1);
	}

	for (i = 0; i < size; i++)
		total += row_changed[i];

	if (total == 0) // same problem !
	{
		*row = size;
		return 1;
	}

	if (total > 1) return 0;


	for (i = 0; i < size; i++)
		if (row_changed[i])
			*row = i;

	return 1;
}

void init_from_prev(int row, hungarian_data *hd, hungarian_cache *hc)
{
	int i;
	int min;
	int n = hc->prev_n;
	int *matched_v = hd->matched_v;
	int *matched_u = hd->matched_u;
	int *V = hd->V;
	int *U = hd->U;

	hd->n = n;

	for (i = 0; i < n; i++)
	{
		hd->V[i] = hc->prev_sol.V[i];
		hd->U[i] = hc->prev_sol.U[i];
		hd->matched_v[i] = hc->prev_sol.match_v[i];
		hd->matched_u[i] = hc->prev_sol.match_u[i];
	}
	matched_u[matched_v[row]] = -1;
	matched_v[row] = -1;

	min = 10000000;
	for (i = 0; i < n; i++)
		if ((hd->c[row][i] - U[i]) < min)
			min = hd->c[row][i] - U[i];
	V[row] = min;

	verify_hungarian_invariant(hd);
}

void save_prev_data(hungarian_solution *sol, hungarian_data *hd, hungarian_cache *hc)
{
	int i, j;
	int n = hd->n;

	hc->prev_n = n;
	for (i = 0; i < n; i++)
		for (j = 0; j < n; j++)
			hc->prev_c[i][j] = hd->c[i][j];
	hc->prev_sol = *sol;
}


void solve_hungarian(int n, box_mat cost, hungarian_solution *sol,	hungarian_cache *hc)
{
	int one_row_changed, row;

	hungarian_data *hd;

	hd = (hungarian_data*)malloc(sizeof(hungarian_data));
	if (hd == NULL) exit_with_error("can't alloc");

	set_hungarian_problem(n, cost, hd);

	one_row_changed = did_one_row_change(n, &row, cost, hc);

	if (one_row_changed == 0)
	{
		init_U_V(hd);
		init_match(hd);
	}
	else
	{
		if (row == n) // same problem
		{
			*sol = hc->prev_sol;
			free(hd);
			return;
		}
		init_from_prev(row, hd, hc);
	}

	do_hungarian_iterations(hd);

	set_hungarian_solution(sol, hd);

	if (one_row_changed == 0) // solved a new problem and not a variation
		save_prev_data(sol, hd, hc);

	free(hd);
}

