// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include "global.h"


void exit_with_error(const char* message);

int is_same_seq(UINT_8 *a, UINT_8 *b, int len);
void print_2d_array(int_board a);
int in_group(int val, int *group, int group_size);
void my_getch();

void remove_newline(char *s);
void remove_preceeding_spaces(char *s);

void show_ints_on_initial_board(int_board dist);

void get_date_as_string(char *time, char *date);
void get_sol_time_as_hms(int sol_time, char *hms);
int is_cyclic_level();
int time_limit_exceeded(int time_limit, int local_start_time);

int get_number_of_cores();