// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#ifndef _GRAPH
#define _GRAPH

#include "board.h"
#include "moves.h"

typedef struct vertex
{
	int shift[4];
	int push;
	int pull;
	int weight;
	int active;
	int src;
	int reversible;
} vertex;

typedef struct graph_data
{
	int vertices_num;
	vertex vertices[MAX_INNER * 4];

	board current_impossible_board;
} graph_data;

// vertex index is: (position of box) | (sokoban position)
// sokoban position is 2 bits.
// shift,push,pull is in the index to which we move by applying this operation.

graph_data *allocate_graph();
void build_graph(board b, int pull_mode, graph_data *gd);
void set_graph_weights_to_infinity(graph_data *gd);
void clear_weight_around_cell(int index, graph_data *gd);
void do_graph_iterations(int pull_mode, graph_data *gd);
int get_weight_around_cell(int index, graph_data *gd);
int get_box_moves_from_graph(int start_index, move *moves, graph_data *gd);
int find_sources_of_a_group(board b, int *group, int group_size, board res);
int find_targets_of_a_group(board b, int *group, int group_size, board targets);
void find_targets_of_a_board_input(board b, board in, board targets);
void find_sources_of_a_board_input(board b, board in, board sources);
void find_targets_of_place_without_init_graph(int start_y, int start_x, board targets, graph_data *gd);
void find_distance_from_a_group(board b, board group, int_board dist, int pull_mode);
void mark_box_moves_in_graph(board b, int box_index, int pull_mode, graph_data *gd);
void make_holes_inactive(board b, graph_data *gd);
void find_reversible_moves(board b, int box_index, int pull_mode, graph_data *gd);


void set_current_impossible(board b_in, int box_y, int box_x, int pull_mode, int search_mode,
	board possible_board, graph_data *gd);


#endif