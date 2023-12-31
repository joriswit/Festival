// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include <stdlib.h>

#include "graph.h"
#include "bfs.h"
#include "moves.h"
#include "distance.h"
#include "util.h"
#include "deadlock.h"

graph_data *allocate_graph()
{
	graph_data *gd;

	gd = (graph_data*)malloc(sizeof(graph_data));
	if (gd == 0)
		exit_with_error("can't allocate graph");

	gd->vertices_num = index_num * 4;
	return gd;
}

void init_active(board b, graph_data *gd)
{
	int i, j;
	int y, x, base_y, base_x;

	vertex *vertices = gd->vertices;

	// "active" tests if sokoban can be positioned in this side of the box

	for (i = 0; i < index_num; i++)
		for (j = 0; j < 4; j++)
			vertices[i * 4 + j].active = 0;

	for (i = 0; i < index_num; i++)
	{
		index_to_y_x(i, &base_y, &base_x);

		if (b[base_y][base_x] & WALL)
		{
			// this can happen when boxes are converted to walls
			// in order to find possible moves of a single box
			for (j = 0; j < 4; j++)
				vertices[i * 4 + j].active = 0;
			continue;
		}


		for (j = 0; j < 4; j++)
		{
			x = base_x + delta_x[j];
			y = base_y + delta_y[j];

			if (b[y][x] & WALL)
				vertices[i * 4 + j].active = 0;
			else
			{
				vertices[i * 4 + j].active = 1;
			}
		}
	}
}

void make_holes_inactive(board b, graph_data *gd)
{
	// this routine identifies cases such as
	//
	//     @        $
	//    #@#      # #    (maybe target in the hole)
	//     #        #
	// 
	// and makes it impossible to position the player in the hole, closed by a box

	int i, y, x, sum;
	int box_y, box_x, index;

	vertex *vertices = gd->vertices;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			if (inner[y][x] == 0) continue;
			if (b[y][x] & OCCUPIED) continue;

			sum = 0;
			for (i = 0; i < 4; i++)
				if (b[y + delta_y[i]][x + delta_x[i]] == WALL)
					sum++;

			if (sum != 3) continue;

			for (i = 0; i < 4; i++)
			{
				box_y = y + delta_y[i];
				box_x = x + delta_x[i];
				if (b[box_y][box_x] == WALL) continue;

				if (inner[box_y][box_x] == 0) continue;

				index = y_x_to_index(box_y, box_x) * 4;

				if (b[y][x] & SOKOBAN)
					if (b[box_y][box_x] & SOKOBAN)
						vertices[index + ((i+2) & 3)].active = 0;

				if ((b[y][x] & SOKOBAN) == 0) 
					vertices[index + ((i + 2) & 3)].active = 0;
			}
		}
	}
}


void init_shift(board b, graph_data *gd)
{
	int i, j;
	int y, x;
	int_board zones;
	int from, to;

	vertex *vertices = gd->vertices;
	int vertices_num = gd->vertices_num;

	for (i = 0; i < vertices_num; i++)
		for (j = 0; j < 4; j++)
			vertices[i].shift[j] = 0;

	for (i = 0; i < index_num; i++)
	{
		index_to_y_x(i, &y, &x);

		if (b[y][x] & OCCUPIED)
			continue;

		b[y][x] |= BOX;
		mark_connectivities_around_place(b, zones, y, x);

		b[y][x] &= ~BOX;

		for (from = 0; from < 4; from++)
		{
			if (vertices[i * 4 + from].active == 0) continue;
			if (zones[y + delta_y[from]][x + delta_x[from]] == 1000000) continue;

			for (to = 0; to < 4; to++)
			{
				if (to == from) continue;
				if (vertices[i * 4 + to].active == 0) continue;
				if (zones[y + delta_y[to]][x + delta_x[to]] == 1000000) continue;

				if (zones[y + delta_y[from]][x + delta_x[from]] ==
					zones[y + delta_y[to]][x + delta_x[to]])
					vertices[i * 4 + from].shift[to] = i * 4 + to;
			}
		}
	} // index of box
}



void init_push(board b, graph_data *gd)
{
	int i;
	int base_x, base_y;
	int push_x, push_y;
	int new_index;

	vertex *vertices = gd->vertices;
	int vertices_num = gd->vertices_num;

	for (i = 0; i < vertices_num; i++)
	{
		if (vertices[i].active == 0)
			continue;

		index_to_y_x(i / 4, &base_y, &base_x);

		if (b[base_y][base_x] & OCCUPIED)
		{
			exit_with_error("something is bad");
		}

		push_x = base_x - delta_x[i % 4];
		push_y = base_y - delta_y[i % 4];

		if (b[push_y][push_x] & OCCUPIED)
			vertices[i].push = 0;
		else
		{
			new_index = y_x_to_index(push_y, push_x);

			vertices[i].push = new_index * 4 + (i % 4);
		}
	}
}



void init_pull(board b, graph_data *gd)
{
	int i;
	int base_x, base_y;
	int pull_x, pull_y;
	int sok_y, sok_x;
	int new_index;

	vertex *vertices = gd->vertices;
	int vertices_num = gd->vertices_num;

	for (i = 0; i < vertices_num; i++)
	{
		if (vertices[i].active == 0)
			continue;

		index_to_y_x(i / 4, &base_y, &base_x);

		if (b[base_y][base_x] & OCCUPIED)
			exit_with_error("something is bad");

		pull_x = base_x + delta_x[i % 4];
		pull_y = base_y + delta_y[i % 4];

		sok_x = base_x + delta_x[i % 4]*2;
		sok_y = base_y + delta_y[i % 4]*2;

		vertices[i].pull = 0;

		if (b[sok_y][sok_x] & OCCUPIED)
			continue;

		new_index = y_x_to_index(pull_y, pull_x);

		vertices[i].pull = new_index * 4 + (i % 4);
		
	}
}


void set_graph_weights_to_infinity(graph_data *gd)
{
	int i;
	vertex *vertices = gd->vertices;
	int vertices_num = gd->vertices_num;

	for (i = 0; i < vertices_num; i++)
	{
		vertices[i].weight = 1000000;
		vertices[i].src = -1;
		vertices[i].reversible = 0;
	}
}



void build_graph(board b, int pull_mode, graph_data *gd)
{
	// set up the active vertices and edges, don't compute anything yet
	vertex *vertices = gd->vertices;
	int vertices_num = gd->vertices_num;

	vertices_num = index_num * 4;

	if (board_contains_boxes(b)) exit_with_error("boxes in board");
	
	init_active(b, gd);
	init_shift(b, gd);

	if (pull_mode)
		init_pull(b, gd);
	else
		init_push(b, gd);

	set_graph_weights_to_infinity(gd);
}

void clear_weight_around_cell(int index, graph_data *gd)
{
	int j;
	vertex *vertices = gd->vertices;

	for (j = 0; j < 4; j++)
	{
		if (vertices[index * 4 + j].active)
			vertices[index * 4 + j].weight = 0;
	}
}


#define MAX_BFS_NODES (MAX_INNER * 4)

void do_graph_iterations(int pull_mode, graph_data *gd)
{
	int current[MAX_BFS_NODES];
	int next[MAX_BFS_NODES];
	int visited[MAX_BFS_NODES];
	int i, j, next_vertex;
	int current_num = 0;
	int next_num = 0;
	int d = 0;
	int v;

	vertex *vertices = gd->vertices;
	int vertices_num = gd->vertices_num;

	for (i = 0; i < vertices_num; i++)
		visited[i] = 0;

	// init from start vertices
	for (i = 0; i < vertices_num; i++)
	{
		if (vertices[i].weight == 0)
		{
			vertices[i].src = -1;
			next[next_num++] = i;
			visited[i] = 1;
		}
	}

	while (next_num)
	{
		d++;

		for (i = 0; i < next_num; i++)
			current[i] = next[i];
		current_num = next_num;
		next_num = 0;

		for (i = 0; i < current_num; i++)
		{
			v = current[i];

			for (j = 0; j < 4; j++)
			{
				next_vertex = vertices[v].shift[j];
				if (next_vertex == 0) continue;

				if (vertices[next_vertex].active == 0)
					exit_with_error("internal error1");

				if (visited[next_vertex]) continue;

				visited[next_vertex] = 1;
				vertices[next_vertex].weight = vertices[v].weight;
				vertices[next_vertex].src = v;
				next[next_num++] = next_vertex;
			}

			if (pull_mode)
				next_vertex = vertices[v].pull;
			else
				next_vertex = vertices[v].push;

			if (next_vertex == 0) continue;

			if (vertices[next_vertex].active == 0)
				exit_with_error("internal error2");


			if (visited[next_vertex]) continue;
			visited[next_vertex] = 1;
			vertices[next_vertex].weight = vertices[v].weight + 1;
			vertices[next_vertex].src = v;
			next[next_num++] = next_vertex;

			if (next_num >= MAX_BFS_NODES)
				exit_with_error("bfs overflow");
		} // loop on current vertices with weight d
	} // while next d

}



void mark_reversible_nodes(int pull_mode, graph_data *gd)
{
	// this routine marks all vertices from which the initial set can be reached.
	// note that the other direction may not be possible.

	int i;
	vertex *vertices = gd->vertices;
	int vertices_num = gd->vertices_num;

	do_graph_iterations(pull_mode ^ 1, gd);
	for (i = 0; i < vertices_num; i++)
	{
		if (vertices[i].weight < 1000000)
			vertices[i].reversible = 1;

		// clear the weights left by backward graph iterations
		if (vertices[i].weight != 0)
			vertices[i].weight = 1000000;
	}
}


int get_weight_around_cell(int index, graph_data *gd)
{
	int j;
	int min = 1000000;
	vertex *vertices = gd->vertices;

	for (j = 0; j < 4; j++)
	{
		if (vertices[index * 4 + j].active == 0) continue;

		if (vertices[index * 4 + j].weight < min)
			min = vertices[index * 4 + j].weight;
	}
	return min;
}


int get_box_moves_from_graph(int start_index, move *moves, graph_data *gd)
{
	int i, j, y, x, src;
	int moves_num = 0;

	vertex *vertices = gd->vertices;

	for (i = 0; i < index_num; i++)
	{
		index_to_y_x(i, &y, &x);

		if ((gd->current_impossible_board)[y][x]) continue; // can't push box here - probably simple deadlock

		for (j = 0; j < 4; j++)
		{
			if (vertices[i * 4 + j].weight == 1000000)
				continue;

			if (vertices[i * 4 + j].weight == 0)
				continue; // this is one of the start positions - not a push

			src = vertices[i * 4 + j].src;
			if ((src/4) == i)
				continue; // last move was not a push, but a sokoban move

			moves[moves_num].from = start_index;
			moves[moves_num].to = i;
			moves[moves_num].sokoban_position = j;
			moves[moves_num].kill = 0;
			moves[moves_num].base = 0;
			moves[moves_num].attr.weight = 0;
			moves[moves_num].attr.advisor = 0;
			moves[moves_num].attr.reversible = vertices[i * 4 + j].reversible;

			moves_num++;

		}
	}
	return moves_num;
}


int find_sources_of_a_group(board b, int *group, int group_size, board res)
{
	// compute which indices can get to any of the places in "group"
	int i;
	int n = 0;
	int y, x;
	int srcs[MAX_INNER];
	int cond1;
	int cond2;

	graph_data *gd = allocate_graph();

	build_graph(b, 1, gd); // 1 - pull mode

	for (i = 0; i < group_size; i++)
		clear_weight_around_cell(group[i], gd);

	do_graph_iterations(1, gd);

	for (i = 0; i < index_num; i++)
	{
		cond1 = get_weight_around_cell(i, gd) != 1000000;
		cond2 = in_group(i, group, group_size);

		if (cond1 || cond2)
			srcs[n++] = i;
	}

	zero_board(res);

	for (i = 0; i < n; i++)
	{
		index_to_y_x(srcs[i], &y, &x);
		res[y][x] = 1;
	}

	free(gd);

	return n;
}

int find_targets_of_a_group(board b, int *group, int group_size, board targets)
{
	int i,y,x;
	int n = 0;
	int tmp_targets[MAX_INNER];

	graph_data *gd = allocate_graph();

	build_graph(b, 0, gd); // 0 - push mode

	for (i = 0; i < group_size; i++)
		clear_weight_around_cell(group[i], gd);

	do_graph_iterations(0, gd); // 0 - push mode

	for (i = 0; i < index_num; i++)
	{
		if ((get_weight_around_cell(i, gd) != 1000000) ||
			in_group(i, group, group_size))
			tmp_targets[n++] = i;
	}

	zero_board(targets);
	for (i = 0; i < n; i++)
	{
		index_to_y_x(tmp_targets[i], &y, &x);
		targets[y][x] = 1;
	}

	free(gd);
	return n;
}

void find_targets_of_a_board_input(board b, board in, board targets)
{
	int group[MAX_INNER];
	int group_size = 0;
	int i, j;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if (inner[i][j] == 0) continue;

			if (in[i][j])
				group[group_size++] = y_x_to_index(i, j);
		}

	find_targets_of_a_group(b, group, group_size, targets);
}


void find_sources_of_a_board_input(board b, board in, board sources)
{
	int group[MAX_INNER];
	int group_size = 0;
	int i, j;

	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			if (in[i][j])
				group[group_size++] = y_x_to_index(i, j);
		}
	}

	find_sources_of_a_group(b, group, group_size, sources);
}

void find_targets_of_place_without_init_graph(int start_y, int start_x, board targets, graph_data *gd)
{
	int i, y, x;
	vertex *vertices = gd->vertices;
	int vertices_num = gd->vertices_num;

	zero_board(targets);
	set_graph_weights_to_infinity(gd);
	clear_weight_around_cell(y_x_to_index(start_y, start_x), gd);

	do_graph_iterations(0, gd);

	for (i = 0; i < vertices_num; i++)
		if (vertices[i].weight != 1000000)
		{
			index_to_y_x(i / 4, &y, &x);
			targets[y][x] = 1;
		}

	targets[start_y][start_x] = 1;
}

void find_distance_from_a_group(board b, board group, int_board dist, int pull_mode)
{
	int i,j, y,x;
	graph_data *gd = allocate_graph();

	build_graph(b, pull_mode, gd); 

	for (i = 0; i < index_num; i++)
	{
		index_to_y_x(i, &y, &x);
		if (group[y][x])
			clear_weight_around_cell(i, gd);
	}

	do_graph_iterations(pull_mode, gd); 

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			dist[i][j] = 1000000;

	for (i = 0; i < index_num; i++)
	{
		index_to_y_x(i, &y, &x);
		dist[y][x] = get_weight_around_cell(i, gd);
	}

	free(gd);
}


void clear_shift(graph_data *gd)
{
	int i, j;
	vertex *vertices = gd->vertices;
	int vertices_num = gd->vertices_num;

	for (i = 0; i < (index_num * 4); i++)
		for (j = 0; j < 4; j++)
			vertices[i].shift[j] = 0;
}



void update_shift(board b, int box_y, int box_x, graph_data *gd)
{
	// using the sokoban cloud, mark which shifts are possible
	int i, from, to, index;
	int has_sokoban[4] = { 0, 0, 0, 0 };

	vertex *vertices = gd->vertices;
	int vertices_num = gd->vertices_num;

	index = y_x_to_index(box_y, box_x) * 4;

	for (i = 0; i < 4; i++)
		if (b[box_y + delta_y[i]][box_x + delta_x[i]] & SOKOBAN)
			has_sokoban[i] = 1;

	for (from = 0; from < 4; from++)
	{
		if (has_sokoban[from] == 0) continue;

		for (to = 0; to < 4; to++)
			if (has_sokoban[to])
				vertices[index + from].shift[to] = index + to;

	}
}

void mark_box_moves_in_graph(board b_in, int box_index, int pull_mode, graph_data *gd)
{
	board b;
	int i, j;
	int box_y, box_x;
	int next_box_y, next_box_x;

	int sokoban_y, sokoban_x;
	int next_sok_y, next_sok_x;

	vertex *vertices = gd->vertices;
	int vertices_num = gd->vertices_num;

	int direction;

	int d = 0;
	int index, next_index;
	int current[MAX_SIZE * MAX_SIZE];
	int next[MAX_SIZE * MAX_SIZE];
	int current_size = 0;
	int next_size = 0;
	int index_after_push;

	copy_board(b_in, b);

	clear_shift(gd);

	set_graph_weights_to_infinity(gd);

	index_to_y_x(box_index, &box_y, &box_x);
	if ((b[box_y][box_x] & BOX) == 0)
	{
		print_board(b);
		exit_with_error("missing box3");
	}

	for (i = 0; i < 4; i++)
	{
		sokoban_y = box_y + delta_y[i];
		sokoban_x = box_x + delta_x[i];
		if (b[sokoban_y][sokoban_x] & SOKOBAN)
		{
			vertices[box_index * 4 + i].weight = d;
			current[current_size++] = box_index * 4 + i;
		}
	}

	b[box_y][box_x] &= ~BOX;

	// invariant: queue contains box locations and all nearby player positions.
	// all that is left is to push/pull

	while (current_size)
	{
		d++;
		for (i = 0; i < current_size; i++)
		{
			index = current[i];
			direction = index & 3;
			index_to_y_x(index / 4, &box_y, &box_x);

			sokoban_y = box_y + delta_y[direction];
			sokoban_x = box_x + delta_x[direction];

			if (pull_mode == 0)
			{
				next_box_y = box_y - delta_y[direction];
				next_box_x = box_x - delta_x[direction];

				next_sok_y = sokoban_y - delta_y[direction];
				next_sok_x = sokoban_x - delta_x[direction];

				if (b[next_box_y][next_box_x] & OCCUPIED) continue;
			}
			else
			{
				next_box_y = box_y + delta_y[direction];
				next_box_x = box_x + delta_x[direction];

				next_sok_y = sokoban_y + delta_y[direction];
				next_sok_x = sokoban_x + delta_x[direction];

				if (b[next_sok_y][next_sok_x] & OCCUPIED) continue;
			}

			if ((gd->current_impossible_board)[next_box_y][next_box_x]) continue;

			next_index = y_x_to_index(next_box_y, next_box_x) * 4 + direction;

			if (vertices[next_index].weight != 1000000) continue;

			// so the box can be pushed there, and this is a new move. now compute surroundings

			vertices[next_index].weight = d;
			vertices[next_index].src = index;
			next[next_size++] = next_index;
			index_after_push = next_index;

			clear_sokoban_inplace(b);
			b[next_box_y][next_box_x] |= BOX;
			b[next_sok_y][next_sok_x] |= SOKOBAN;
			expand_sokoban_cloud_for_graph(b);

			update_shift(b, next_box_y, next_box_x, gd);

			for (j = 0; j < 4; j++)
			{
				next_sok_y = next_box_y + delta_y[j];
				next_sok_x = next_box_x + delta_x[j];

				if ((b[next_sok_y][next_sok_x] & SOKOBAN) == 0) continue;

				next_index = y_x_to_index(next_box_y, next_box_x) * 4 + j;

				if (vertices[next_index].weight != 1000000) continue;

				vertices[next_index].weight = d;
				vertices[next_index].src = index_after_push;
				next[next_size++] = next_index;
			}

			b[next_box_y][next_box_x] &= ~BOX;
		} // current_size

		for (i = 0; i < next_size; i++)
			current[i] = next[i];
		current_size = next_size;
		next_size = 0;
	} // BFS queue
}

// some routines to detect reversible moves

int can_move_from_index_to_index(int from, int to, graph_data *gd)
{
	vertex *vertices = gd->vertices;

	if ((from ^ to) >> 2)
		exit_with_error("bad call\n");

	if (vertices[from].shift[to & 3] == to) return 1;

	return 0;
}

void find_reversible_moves(board b, int box_index, int pull_mode, graph_data *gd)
{
	int current[MAX_SIZE * MAX_SIZE];
	int next[MAX_SIZE * MAX_SIZE];
	int box_y, box_x;
	int current_size = 0;
	int next_size;
	int i,j,index,next_index;
	int visited[MAX_INNER * 4];
	int sokoban_y, sokoban_x;
	int direction;

	vertex *vertices = gd->vertices;
	int vertices_num = gd->vertices_num;

	for (i = 0; i < vertices_num; i++)
		visited[i] = 0;

	index_to_y_x(box_index, &box_y, &box_x);

	if ((b[box_y][box_x] & BOX) == 0)
		exit_with_error("missing box4");

	b[box_y][box_x] &= ~BOX;

	// maintain a list of indices such that:
	// all of them are reachable from the start (tested using "src" value)
	// all of them can lead to the start (verified by pulling the boxes)

	for (i = 0; i < vertices_num; i++)
		vertices[i].reversible = 0;

	for (i = 0; i < 4; i++)
	{
		if ((b[box_y + delta_y[i]][box_x + delta_x[i]] & SOKOBAN) == 0) continue;
		index = box_index * 4 + i;

		vertices[index].reversible = 1;
		current[current_size++] = index;
		visited[index] = 1;
	}

	while (current_size)
	{
		next_size = 0;
		for (i = 0; i < current_size; i++)
		{
			index = current[i];

			// try shifts
			for (j = 0; j < 4; j++)
			{
				next_index = (index & ~3) + j;

				if (visited[next_index]) continue;

				if (can_move_from_index_to_index(index, next_index, gd))
					if (vertices[next_index].src != -1)
					{
						next[next_size++] = next_index;
						vertices[next_index].reversible = 1;
						visited[next_index] = 1;
					}
			}

			// try pull

			index_to_y_x(index >> 2, &box_y, &box_x);
			direction = index & 3;

			sokoban_y = box_y + delta_y[direction];
			sokoban_x = box_x + delta_x[direction];

			if (b[sokoban_y][sokoban_x] & OCCUPIED)
				exit_with_error("back problem");

			if (pull_mode == 0)
			{
				sokoban_y += delta_y[direction];
				sokoban_x += delta_x[direction];

				box_y += delta_y[direction];
				box_x += delta_x[direction];

				if (b[sokoban_y][sokoban_x] & OCCUPIED)
					continue;
			}
			else
			{
				box_y -= delta_y[direction];
				box_x -= delta_x[direction];

				if (b[box_y][box_x] & OCCUPIED)
					continue;
			}

			next_index = y_x_to_index(box_y, box_x) * 4 + direction;

			if ((gd->current_impossible_board)[box_y][box_x]) continue;
			if (visited[next_index]) continue;

			if (vertices[next_index].src != -1)
			{
				next[next_size++] = next_index;
				vertices[next_index].reversible = 1;
				visited[next_index] = 1;
			}
		} // i

		for (i = 0; i < next_size; i++)
			current[i] = next[i];
		current_size = next_size;

	} // while current


	index_to_y_x(box_index, &box_y, &box_x);
	b[box_y][box_x] |= BOX;

}

void set_current_impossible(board b_in, int box_y, int box_x, int pull_mode, int search_mode,
	board possible_board, graph_data *gd)
{
	int i, j, y, x;
	board blocked;
	board b;

	copy_board(b_in, b);

	zero_board(gd->current_impossible_board);

	for (i = 0; i < index_num; i++)
	{
		index_to_y_x(i, &y, &x);

		if (impossible_place[i]) // general impossible places of the level. board edges etc.
			(gd->current_impossible_board)[y][x] = 1;

		// add boxes that were wallified
		if (b[y][x] == WALL)
			(gd->current_impossible_board)[y][x] = 1;
	}

	if (pull_mode == 0) // pull mode can't pull to 2x2 patterns
	{
		b[box_y][box_x] &= ~BOX;

		mark_deadlock_2x2(b, blocked);

		for (i = 0; i < height; i++)
			for (j = 0; j < width; j++)
			{
				if (inner[i][j] == 0) continue;

				if (blocked[i][j])
					(gd->current_impossible_board)[i][j] = 1;
			}
	}


	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if (inner[i][j] == 0) continue;

			if (possible_board[i][j] == 0)
				(gd->current_impossible_board)[i][j] = 1;
		}
}


