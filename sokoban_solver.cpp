// Festival Sokoban Solver
// Copyright 2018-2022 Yaron Shoham

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#ifdef THREADS
#include <pthread.h>
#endif

#include "global.h"
#include "board.h"
#include "distance.h"
#include "deadlock.h"
#include "engine.h"
#include "rooms.h"
#include "io.h"
#include "advanced_deadlock.h"
#include "debug.h"
#include "hotspot.h"
#include "sol.h"
#include "util.h"
#include "deadlock_cache.h"
#include "packing_search.h"
#include "park_order.h"
#include "perimeter.h"
#include "mpdb2.h"
#include "envelope.h"
#include "k_dist_deadlock.h"
#include "oop.h"
#include "imagine.h"
#include "preprocess.h"
#include "holes.h"
#include "rooms_deadlock.h"
#include "girl.h"
#include "helper.h"
#include "stuck.h"
#include "dragonfly.h"
#include "snail.h"

tree search_trees[8];
helper helpers[8];

int forced_alg = -1;
//int forced_alg = 0;

#define FROM_LEVEL 1

void allocate_search_trees()
{
	int log_size = 23; // about 1.5GB per core
	int i;

	if ((cores_num != 1) && (cores_num != 2) && (cores_num != 4) && (cores_num != 8))
		exit_with_error("Number of cores should be 1/2/4/8");

#ifdef VISUAL_STUDIO
	log_size = 22; // should fit in a the 2GB memory limit...
#endif

	log_size += extra_mem;

	for (i = 0; i < cores_num; i++)
	{
		if (i == 7) log_size -= 3; // dragonfly does not need the tree

		if (verbose >= 4) printf("Allocating search tree for core %d\n", i);
		init_tree(&search_trees[i], log_size);
	}
}

void free_search_trees()
{
	int i;
	for (i = 0; i < cores_num; i++)
		free_tree(search_trees + i);
}


void allocate_helpers()
{
	int i;

	for (i = 0; i < cores_num; i++)
	{
		init_helper(helpers + i);
		init_helper_extra_fields(helpers + i);
		helpers[i].my_core = i;
	}
}

void free_helpers()
{
	int i;

	for (i = 0; i < cores_num; i++)
		free_helper(helpers + i);
}

int preprocess_level(board b)
{
	int res;

	strcpy(global_fail_reason, "Unknown reason");

	if ((height == 0) || (width == 0)) return 0;

	if (sanity_checks(b) == 0)
		return 0;

	clear_deadlock_cache();
	clear_k_dist_hash();
	clear_perimeter();

	turn_decorative_boxes_to_walls(b);
	close_holes_in_board(b);

	init_inner(b);
	init_index_x_y();

	keep_boxes_in_inner(b);
	save_initial_board(b);
	expand_sokoban_cloud(b);

	set_forbidden_tunnel();
	mark_target_holes(b);

	if (verbose >= 3)
	{
		printf("\nLevel %d:\n", level_id);
		print_board(b);
	}

	set_distances(b);

	res = analyse_rooms(b);
	if (res == 0) return 0;

	init_rooms_deadlock();

	init_hotspots(b);

	build_mpdb2();
	build_pull_mpdb2();

	init_envelope_patterns();
	
	init_girl_variables(b);
	init_stuck_patterns();

	detect_snail_level(b);

	return 1;
}

int setup_plan_features(int search_type, helper *h)
{
	if (search_type == HF_SEARCH) return 1; 
	if (search_type == BICON_SEARCH) return 1;
	if (search_type == MAX_DIST_SEARCH) return 1;
	if (search_type == MAX_DIST_SEARCH2) return 1;
	if (search_type == REV_SEARCH) return 1;
	if (search_type == NAIVE_SEARCH) return 1;
	if (search_type == DRAGONFLY) return 1;

	if (h->parking_order_num == 0)
	{
		if (verbose >= 4)
			printf("No packing order\n");
		strcpy(global_fail_reason, "Could not find packing order");
		return 0;
	}

	verify_parking_order(h);
	reduce_parking_order(h);
	show_parking_order(h);

	if (search_type == GIRL_SEARCH) return 1;

	prepare_oop_zones(h);

	return 1;
}



void packing_search_control(board b, int time_allocation, int search_type, tree* t, helper* h)
{
	int end_time = (int)time(0) + time_allocation;

	if (time_allocation <= 0) return;

	h->weighted_search = 1;

	search_type = set_snail_parameters(search_type, 1, h); 
	search_type = set_netlock_parameters(search_type, 1, h);
	
	if (search_type == SNAIL_SEARCH)   time_allocation *= 2;
	if (search_type == NETLOCK_SEARCH) time_allocation *= 2;
	if (search_type == DRAGONFLY)      time_allocation *= 3;

	if (search_type == REV_SEARCH)
	{
		FESS(b, time_allocation, search_type, t, h);
		return;
	}

	if (search_type == DRAGONFLY)
	{
		dragonfly_search(b, time_allocation, h);
		return;
	}

	packing_search(b, time_allocation, search_type, t, h);
}

void forward_search_control(board b, int time_allocation, int search_type, tree* t, helper* h)
{
	int end_time = (int)time(0) + time_allocation;

	if (time_allocation <= 0) return;

	h->weighted_search = 1;

	search_type = set_snail_parameters(search_type, 0, h);
	search_type = set_netlock_parameters(search_type, 0, h);

	if (search_type == HF_SEARCH)
	{
		h->weighted_search = 0;
		FESS(b, time_allocation * 3 / 4, search_type, t, h);
		if (h->level_solved) return;

		h->weighted_search = 1;
		time_allocation = end_time - (int)time(0);
		FESS(b, time_allocation, search_type, t, h);

		return;
	}


	if (search_type == REV_SEARCH)
	{
		packing_search(b, time_allocation, search_type, t, h);
		if (h->perimeter_found == 0) return;
		time_allocation = end_time - (int)time(0);
	}

	FESS(b, time_allocation, search_type, t, h);
	return;
}


void solve_with_alg(board b, int time_allocation, int strategy_index, helper *h)
{
	// Strategy A: eliminate boxes via sink squares. Stop packing search when boxes are removed from targets.
	// Strategy B: do not eliminate boxes. Use 1/3 of the time for preparing the perimeter.
	// Strategy C: girl mode 
	// strategy D: HF search

	int local_start_time, remaining_time;
	int search_type;
	tree *t;

	int backward_search_type[8] = 
	{
		BASE_SEARCH,
		MAX_DIST_SEARCH2, //NORMAL, 
		GIRL_SEARCH, 
		HF_SEARCH , 
		BICON_SEARCH, 
		MAX_DIST_SEARCH, 
		REV_SEARCH,
		DRAGONFLY
	};
	
	int forward_search_type[8] = 
	{
		FORWARD_WITH_BASES, 
		HF_SEARCH, //NORMAL, 
		GIRL_SEARCH, 
		HF_SEARCH , 
		HF_SEARCH,    
		HF_SEARCH, 
		REV_SEARCH,
		NAIVE_SEARCH,
	};

	if (time_allocation <= 0) return;

	reset_helper(h);
	t = search_trees + h->my_core;
	
	// backward search

	search_type = backward_search_type[strategy_index];

	if (verbose >= 4)
	{
		printf("Starting strategy %c. ", 'A' + strategy_index);
		printf(" Time limit: %d seconds\n", time_allocation);
	}

	local_start_time = (int)time(0);
	
	packing_search_control(b, time_allocation / 3, search_type, t, h);

	if (setup_plan_features(search_type, h) == 0)
		return;

	// forward search

	remaining_time = local_start_time + time_allocation - (int)time(0);

	search_type = forward_search_type[strategy_index];;

	forward_search_control(b, remaining_time, search_type, t, h);
}

typedef struct
{
	board b;
	int time_allocation;
	int alg;
	helper *h;
} work_element;

void *solve_work_element(void *we_in)
{
	work_element *we = (work_element*)we_in;

	if ((cores_num > 1) && (verbose >= 4))
		printf("core %d starting\n", we->h->my_core);

	solve_with_alg(we->b, we->time_allocation, we->alg, we->h);

	if ((cores_num > 1) && (verbose >= 4))
		printf("core %d ending\n", we->h->my_core);
	
	return NULL;
}



void prepare_work_element(board b, int time_allocation, int alg, helper *h, work_element *we)
{
	copy_board(b, we->b);
	we->time_allocation = time_allocation;
	we->alg = alg;
	we->h = h;
}

int get_search_time(double ratio)
{
	int remaining_time;

	remaining_time = start_time + time_limit - (int)time(0);
	return (int)(remaining_time * ratio);
}

void solve_with_time_control_single_core(board b)
{
	work_element we;
	int search_time;
	int i;

	if (forced_alg != -1)
	{
		search_time = get_search_time(1.0);
		prepare_work_element(b, search_time, forced_alg, helpers + 0, &we);
		solve_work_element((void*)(&we));
		return;
	}

	for (i = 0; i < 8; i++)
	{
		search_time = get_search_time(1 / (double)(8 - i));

		prepare_work_element(b, search_time, i, helpers + 0, &we);
		solve_work_element((void*)(&we));
		if (helpers[0].level_solved) return;
	}
}


void solve_with_time_control_two_cores(board b)
{
#ifndef THREADS
	return;
#else
	work_element we_0, we_1;
	int search_time;
	int rc0, rc1;
	int i;
	pthread_t threads[2];

	for (i = 0; i < 4; i++)
	{
		search_time = get_search_time(1 / (double)(4 - i));

		prepare_work_element(b, search_time, i*2 + 0, helpers + 0, &we_0);
		prepare_work_element(b, search_time, i*2 + 1, helpers + 1, &we_1);

		rc0 = pthread_create(&threads[0], NULL, solve_work_element, &we_0);
		rc1 = pthread_create(&threads[1], NULL, solve_work_element, &we_1);

		pthread_join(threads[0], NULL);
		pthread_join(threads[1], NULL);

		if (any_core_solved) return;
	}
#endif
}


void solve_with_time_control_four_cores(board b)
{
#ifndef THREADS
	return;
#else
	work_element we[4];
	int search_time;
	int rc[4];
	int i,j;
	pthread_t threads[4];

	for (i = 0; i < 2; i++)
	{
		search_time = get_search_time(1 / (double)(2 - i));

		for (j = 0 ; j < 4 ; j++)
			prepare_work_element(b, search_time, i * 4 + j, helpers + j, we + j);

		for (j = 0 ; j < 4 ; j++)
			rc[j] = pthread_create(&threads[j], NULL, solve_work_element, we + j);

		for (j = 0 ; j < 4 ; j++)
			pthread_join(threads[j], NULL);

		if (any_core_solved) return;
	}
#endif
}



void solve_with_time_control_eight_cores(board b)
{
#ifndef THREADS
	return;
#else
	work_element we[8];
	int rc[8];
	pthread_t threads[8];
	int i,search_time;

	search_time = get_search_time(1.0);

	for (i = 0 ; i < 8 ; i++)
	{
		prepare_work_element(b, search_time, i, helpers + i, we + i);
		rc[i] = pthread_create(&threads[i], NULL, solve_work_element, &we[i]);
	}

	for (i = 0 ; i < 8 ; i++)
		pthread_join(threads[i], NULL);
#endif
}


void solve_with_time_control(board b)
{
	int i;

	start_time = (int)time(0);
	any_core_solved = 0;

	for (i = 0; i < cores_num; i++)
		reset_helper(helpers + i); // remove leftovers solutions from previous levels

	if (preprocess_level(b) == 1)
	{
		if (cores_num == 1) solve_with_time_control_single_core(b);
		if (cores_num == 2) solve_with_time_control_two_cores(b);
		if (cores_num == 4) solve_with_time_control_four_cores(b);
		if (cores_num == 8) solve_with_time_control_eight_cores(b);
	}
	else
	{
		if (verbose >= 4)
			printf("preprocess failed\n");
	}

	end_time = (int)time(0);

	if (end_time < start_time) end_time = start_time;

}

int save_solution_if_found()
{
	int i;
	int solved = 0;

	if (save_best_flag)
	{
		for (i = 0; i < cores_num; i++)
		{
			save_sol_moves(helpers + i);
			solved |= helpers[i].level_solved;
		}
		return solved;
	}

	for (i = 0; i < cores_num; i++)
	{
		if (helpers[i].level_solved)
		{
			save_sol_moves(helpers + i);
			return 1;
		}
	}
	return 0;
}


void solve_level_set()
{	
	board b;
	int i,solved;
	int levels_num;
	int from, to;

	levels_num = load_level_from_file(b, 10000); // count levels in file

	write_log_header();
	write_solution_header();

	from = FROM_LEVEL;
	to = levels_num;

	if (global_from_level != -1) from = global_from_level;
	if (global_to_level   != -1) to   = global_to_level;
	if (just_one_level != -1)
		from = to = just_one_level;

	for (i = from; i <= to; i++) // using one-based here
	{
		level_id = i;
		printf("\n=================\nLevel %d\n=================\n", level_id);

		load_level_from_file(b, level_id);
		save_level_to_solution_file(b);

		solve_with_time_control(b);

		solved = save_solution_if_found();
		if (solved) printf("SOLVED!\n");

		save_times_to_solutions_file(end_time - start_time);

		if ((end_time - start_time) >= time_limit)
		{
			if (strcmp(global_fail_reason, "Too many moves") != 0)
				strcpy(global_fail_reason, "Time limit exceeded");
		}

		save_level_log(solved);
	}

	write_log_footer();
}


void process_args(int argc, char **argv)
{
	int i;

	if (global_dir[0] == '.') // not experimental
	{
		if (argc < 2)
		{
			printf("Usage: festival.exe levels.sok [options]");
			exit(0);
		}

		strcpy(global_level_set_name, argv[1]);
	}
	

	for (i = 0; i < argc; i++)
	{
		if (strcmp(argv[i], "-level") == 0)
			sscanf(argv[i + 1], "%d", &just_one_level);

		if (strcmp(argv[i], "-out_dir") == 0)
			strcpy(global_dir, argv[i + 1]);

		if (strcmp(argv[i], "-time") == 0)
			sscanf(argv[i + 1], "%d", &time_limit);

		if (strcmp(argv[i], "-from") == 0)
			sscanf(argv[i + 1], "%d", &global_from_level);

		if (strcmp(argv[i], "-to") == 0)
			sscanf(argv[i + 1], "%d", &global_to_level);

		if (strcmp(argv[i], "-out_file") == 0)
		{
			strcpy(global_output_filename, argv[i + 1]);
			YASC_mode = 1;
		}

		if (strcmp(argv[i], "-save_best") == 0)
			save_best_flag = 1;

		if (strcmp(argv[i], "-cores") == 0)
			sscanf(argv[i + 1], "%d", &cores_num);

		if (strcmp(argv[i], "-alg") == 0)
			sscanf(argv[i + 1], "%d", &forced_alg);

		if (strcmp(argv[i], "-extra_mem") == 0)
			sscanf(argv[i + 1], "%d", &extra_mem);
	}

	if (cores_num == -1) 
		cores_num = get_number_of_cores();
}


int main(int argc, char **argv)
{
	printf("===================================\n");
	printf("%s\n", solver_name);
	printf("Copyright 2019-2022 by Yaron Shoham\n");
	printf("===================================\n");

	process_args(argc, argv);

	allocate_perimeter();
	allocate_deadlock_cache();
	allocate_search_trees();
	allocate_helpers();
	init_dragonfly();


	read_deadlock_patterns(0); // normal mode
	read_deadlock_patterns(1); // pull mode
 
	solve_level_set();  // SOLVE LEVEL SETS

	free_perimeter();
	free_deadlock_cache();
	free_search_trees();
	free_helpers();


	return 0;
}

