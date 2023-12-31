// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include "board.h"

int work_left_in_deadlock_zone(board b);
int work_left_outside_deadlock_zone(board b);
void clear_boxes_out_of_deadlock_zone(board b);
void clear_frozen_boxes_away_from_deadlock_zone(board b);
void clear_deadlock_zone(board b_in, board b_out);
void get_deadlock_zone_indicator(board b, board i);
void init_deadlock_zone_from_group(board b, board group, int pull_mode);
void set_deadlock_zone_in_pull_mode(board b);
void update_deadlock_zone(board b, int pull_mode);
int size_of_deadlock_zone(board b);
void clear_deadlock_zone_inplace(board b);
void get_sokoban_cloud_as_group(board b, board g);
int exited_deadlock_zone(board b);
