// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#ifndef __BOARD
#define __BOARD

#include "global.h"


void copy_board(board from, board to);

void init_inner(board b);

extern int index_num;
extern board inner;

void init_index_x_y();
int y_x_to_index(int y, int x);
void index_to_y_x(int index, int *y, int *x);
void clear_boxes(board b, board board_without_boxes);
void clear_boxes_inplace(board b);
int board_contains_boxes(board b);
void turn_boxes_into_walls(board b);

void expand_sokoban_cloud(board b);
void expand_sokoban_cloud_for_graph(board b);

void get_sokoban_position(board b, int *y, int *x);
void clear_sokoban_inplace(board b);
void clear_targets_inplace(board b);
int all_boxes_in_place(board b, int pull_mode);
int board_is_solved(board b, int pull_mode);



UINT_64 get_board_hash(board b);
void board_to_bytes(board b, UINT_8 *data);
void bytes_to_board(UINT_8 *data, board b);
void enter_reverse_mode(board b, int mark_bases);

void rev_board(board b);
void print_board(board b);
void save_initial_board(board b);

extern board initial_board;


int boxes_on_targets(board b);
int boxes_on_bases(board b);


int boxes_in_level(board b);

int is_same_board(board a, board b);

void zero_board(board b);
void turn_targets_into_walls(board b);
void show_on_initial_board(board data);
int board_popcnt(board x);
void mark_indices_with_boxes(board b, int *indices);

int count_walls_around(board b, int y, int x);
int count_occupied_around(board b, int y, int x);

void negate_board(board b);

void print_board_with_zones(board b, int_board zones);
void turn_targets_into_packed(board b);

void zero_int_board(int_board a);

void keep_boxes_in_inner(board b);
void clear_bases_inplace(board b);
void get_box_lacations_as_board(board b, board boxes);

void and_board(board a, board mask);

void copy_int_board(int_board from, int_board to);

int all_targets_filled(board b);
void go_to_final_position_with_given_walls(board b);

int get_indices_of_boxes(board b, int *indices);
int get_indices_of_targets(board b, int *indices);
int get_indices_of_initial_bases(board b, int *indices);
void put_sokoban_everywhere(board b);
void put_sokoban_in_zone(board b, int_board zones, int z);
int size_of_sokoban_cloud(board b);
void get_untouched_areas(board b, board untouched);
void print_with_bases(board b);
void print_int_board(int_board b, int with_walls);

#endif