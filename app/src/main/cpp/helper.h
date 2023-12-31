#ifndef _HELPER
#define _HELPER

#include "board.h"


typedef struct
{
	char advisor;
	char weight; // perimeter sets it to negative
	char reversible;
} move_attr;

typedef struct move
{
	short from;
	short to;
	char sokoban_position;
	char kill;
	char base;
	char pull;
	move_attr attr;
} move;


typedef struct park_order_data
{
	int from;
	int to;
	int until;
} park_order_data;

typedef struct
{
	board oop_zone_for_step[MAX_SOL_LEN];
	int_board oop_zone_distance_for_step[MAX_SOL_LEN];
} oop_data;


typedef struct
{
	board imagined_boards[MAX_SOL_LEN];
	int imagine_packed_num[MAX_SOL_LEN];
	board fixed_at_step[MAX_SOL_LEN];

	int imagined_boards_num;

} imagine_data;



typedef struct pack_request
{
	int boxes_in_level;
	int boxes_on_targets;
	int connectivity;
	int room_connectivity;
} pack_request;

typedef struct request
{
	int out_of_plan;
	int packed_boxes;
	int connectivity;
	int imagine;
} request;

#define MAX_REQUESTS 10000

typedef struct request_data
{
	pack_request pack_requests[MAX_REQUESTS];
	int pack_requests_num;
	int pack_requests_pos;

	request requests[MAX_REQUESTS];
	int requests_num;
	int requests_pos;
} request_data;









typedef struct
{
	// park order
	park_order_data parking_order[MAX_SOL_LEN];
	int parking_order_num;
	int boxes_were_removed;

	board used_in_plan;


	park_order_data full_parking_order[MAX_SOL_LEN];
	int full_parking_order_num;

	// scored distance
	int_board scored_distance;

	// oop
	oop_data *oop;

	// imagine
	imagine_data *imagine;
	board imagined_hf_board;
	board wallified;

	// request 
	request_data *request;

	// solution
	move sol_move[MAX_SOL_LEN];
	int sol_len;
	int level_solved;

	int perimeter_found;
	int enable_inefficient_moves;

	// room deadlock
	int tested_deadlock_room;

	// k-dist search
	int k_dist_value;

	int weighted_search;
	int alg_type;

	int my_core;
} helper;

void init_helper(helper *h);
void reset_helper(helper *h);
void init_helper_extra_fields(helper *h);
void free_helper(helper *h);

#endif