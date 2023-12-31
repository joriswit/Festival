// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#ifndef __GLOBAL
#define __GLOBAL

#define WALL 1
#define BOX 2
#define TARGET 4
#define SOKOBAN 8
#define BASE 16
#define DEADLOCK_ZONE 32

#define SPACE 0

#define MAX_SIZE 51
#define MAX_BOXES 200

#define MAX_SOL_LEN 1500

#define OCCUPIED (WALL | BOX)
#define PACKED_BOX (BOX | TARGET)
#define SOKOBAN_ON_TARGET (SOKOBAN | TARGET)

extern int height;
extern int width;

#define MAX_INNER (MAX_SIZE*MAX_SIZE)
#define MAX_ROOMS 70

#define UINT_64 unsigned long long
#define UINT_16 unsigned short
#define UINT_8 unsigned char


#define NORMAL                 0
#define DEADLOCK_SEARCH        1
#define BASE_SEARCH            2
#define K_DIST_SEARCH          3
#define MINI_SEARCH            4
#define FORWARD_WITH_BASES     5
#define GIRL_SEARCH            6
#define REV_SEARCH             7
#define DRAGONFLY              8
#define WOBBLERS_SEARCH        9
#define HF_SEARCH             10
#define ROOMS_DEADLOCK_SEARCH 11
#define NAIVE_SEARCH          12
#define XY_SEARCH             13
#define MINI_CORRAL           14
#define IGNORE_CORRAL         15
#define BICON_SEARCH          16
#define MAX_DIST_SEARCH       17
#define MAX_DIST_SEARCH2      18
#define SNAIL_SEARCH          19
#define NETLOCK_SEARCH        20

extern int delta_y[4];
extern int delta_x[4];
extern int delta_y_8[8];
extern int delta_x_8[8];

extern int verbose;
extern int level_id;
extern char level_title[1000];

extern int global_initial_sokoban_y, global_initial_sokoban_x;
extern int global_boxes_in_level;

extern char global_dir[1000];
extern char global_level_set_name[1000];

extern char global_output_filename[1000];
extern int YASC_mode;
extern int save_best_flag;
extern int extra_mem;


extern int just_one_level;
extern int global_from_level;
extern int global_to_level;

extern int start_time, end_time , time_limit;

extern char global_fail_reason[50];

extern char solver_name[100];

extern int cores_num;
extern int any_core_solved;

typedef unsigned char board[MAX_SIZE][MAX_SIZE];
typedef int int_board[MAX_SIZE][MAX_SIZE];
typedef int box_mat[MAX_BOXES][MAX_BOXES];

#ifdef _CRT_SECURE_NO_WARNINGS
#define VISUAL_STUDIO
#endif


#endif