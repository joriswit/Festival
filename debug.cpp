// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include <stdio.h>
#include <string.h>

#ifdef THREADS
#include <pthread.h>
#endif

#include "debug.h"
#include "io.h"
#include "util.h"
#include "tree.h"
#include "imagine.h"


void save_status(board b_in, int pull_mode)
{
	FILE *fp;
	char s[1000];
	board b;
	char message[1000];
	int y,x;

	if (YASC_mode) return;
	if (cores_num > 1) return;

	copy_board(b_in, b);

	// prepare board for saving
	clear_bases_inplace(b);
	get_sokoban_position(b, &y, &x);
	clear_sokoban_inplace(b);
	b[y][x] |= SOKOBAN;


	sprintf(s, "%s\\status.sok", global_dir);
	fp = fopen(s, "w");
	if (fp == 0) return;

	save_board_to_file(b, fp);

	if (pull_mode == 0)
		strcpy(message, "forward search");
	else // pull mode
		strcpy(message, "backward search");

	fprintf(fp, "\n");
	fprintf(fp, "Status: %s\n", message);
	fclose(fp);
}


void print_node_advisors(tree* t, expansion_data* e)
{
	move moves[MAX_MOVES];
	score_element scores[MAX_MOVES];
	move_hash_data* mh;
	int i;

	mh = t->move_hashes + e->move_hash_place;
	for (i = 0; i < e->moves_num; i++)
	{
		moves[i] = mh[i].move;
		get_score_of_hash(t, mh[i].hash, scores + i);
	}

	print_advisors(&(e->advisors), moves, scores, t->pull_mode, t->search_mode);
}


#ifdef THREADS
pthread_mutex_t print_node_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

void print_node(tree* t, expansion_data* e, int print_moves, const char* message, helper* h)
{
	int moves_num;
	int i;
	move_hash_data* mh;
	node_element* node;
	score_element s;
	int node_place;
	board b;

	if (e == 0)
	{
		if (verbose >= 4) exit_with_error("null node pointer");
		return;
	}

#ifdef THREADS
	pthread_mutex_lock(&print_node_mutex);
#endif

	printf("%s", message);

	bytes_to_board(e->b, b);
	print_board(b);

	if (verbose < 4)
	{
		printf("\n");
#ifdef THREADS
		pthread_mutex_unlock(&print_node_mutex);
#endif
		return;
	}

	// print imagined board
	if (t->pull_mode == 0)
		if (t->search_mode == HF_SEARCH)
			print_board_with_imagined(b, h);

	if ((t->pull_mode) && (t->search_mode == REV_SEARCH))
		print_board_with_imagined(b, h);

	if ((t->pull_mode) && (t->search_mode == NAIVE_SEARCH))
	{
		copy_board(initial_board, h->imagined_hf_board);
		print_board_with_imagined(b, h);
	}


	//	printf("node index: %d\n", (int)(e - t->expansions));
	//	printf("hash= %016llx\n", e->node->hash);

	printf("weight: %d\n", e->weight);
	printf("depth: %d\n", e->depth);

	if (e->has_corral)
		printf("node has corral\n");

	print_score(&(e->node->score), t->pull_mode, t->search_mode);

	if (t->search_mode != GIRL_SEARCH)
		print_node_advisors(t, e);

	if (e->node->deadlocked)
		if (board_contains_boxes(b))
			printf("#position is deadlocked.#\n");

	if (print_moves)
	{
		mh = t->move_hashes + e->move_hash_place;

		moves_num = e->moves_num;

		printf("Node has %d moves\n", moves_num);

		for (i = 0; i < moves_num; i++)
		{
			printf("%3d: ", i);
			print_move(&(mh[i].move));

			node_place = find_node_by_hash(t, mh[i].hash);
			if (node_place == -1) exit_with_error("missing node");
			node = t->nodes + node_place;

			if (node->expansion != -1)
				printf("+");
			else
				printf("-");

			if ((mh[i].deadlocked) || (node->deadlocked))
			{
				printf("#deadlock#\n");
				continue;
			}

			get_score_of_hash(t, mh[i].hash, &s);
			print_score(&s, t->pull_mode, t->search_mode);
		}
	}

	printf("\n");

#ifdef THREADS
	pthread_mutex_unlock(&print_node_mutex);
#endif

}


void print_dragonfly_node(dragonfly_node *e, const char* message, helper* h)
{
#ifdef THREADS
	pthread_mutex_lock(&print_node_mutex);
#endif
	printf("%s", message);
	dragonfly_print_node(e);
#ifdef THREADS
	pthread_mutex_unlock(&print_node_mutex);
#endif
}




void show_progress_tree(tree *t, expansion_data *last_node, expansion_data *best_so_far, int iter_num,
	expansion_data **last_best, helper *h)
{
	int show_current = 0;
	board b;
	char message[1000];

	int my_core = h->my_core;

	if (best_so_far == 0) return;
	if ((*last_best) == best_so_far) return;

	if (verbose <= 3)
	{   // do not show positions with removed boxes
		if (best_so_far->node->score.boxes_in_level != global_boxes_in_level)
			return;
	}

	if ((verbose >= 5) || ((verbose >= 3) && (iter_num % 500) == 0))
	{
		sprintf(message, "Level: %d\n", level_id);

		if (show_current)
		{
			printf("position after expansion:\n");
			print_node(t, last_node, 0, (char*)"", h);
			printf("\n");
		}

		if (t->pull_mode == 0)
			strcat(message, "best so far:\n");
		else
			strcat(message, "best so far: (pull mode)\n");

		if (cores_num > 1) 
			sprintf(message + strlen(message), "core %d\n", my_core);

		print_node(t, best_so_far, 0, message, h);

		*last_best = best_so_far;

		bytes_to_board(best_so_far->b, b);
		save_status(b, t->pull_mode);
	}
}



void show_progress_dragonfly(dragonfly_node* best_so_far, int iter_num, helper* h)
{
	char message[1000];
	int my_core = h->my_core;

	static dragonfly_node* last_best; // static - assumes only one dragonfly instance

	if (best_so_far == 0) return;
	if (last_best == best_so_far) return;

	if ((verbose >= 5) || ((verbose >= 3) && (iter_num % 500) == 0))
	{
		sprintf(message, "Level: %d\n", level_id);
		strcat(message, "best so far: (pull mode)\n");

		if (cores_num > 1)
			sprintf(message + strlen(message), "core %d\n", my_core);

		print_dragonfly_node(best_so_far, message, h);

		last_best = best_so_far;
	}
}




void debug_tree(tree *t, expansion_data *e, helper *h)
{
	int m;
	move_hash_data *mh;
	UINT_64 hash;
	int place;

	while (1)
	{
		print_node(t, e, 1, (char*)"", h);
		scanf("%d", &m);

		mh = t->move_hashes + e->move_hash_place;

		hash = mh[m].hash;

		printf("Expanding hash: %016llx\n", hash);
		place = find_node_by_hash(t, hash);

		e = t->expansions + t->nodes[place].expansion;
	}
}

/*
void do_log(board b, int res)
{
	int i, j, k;
	int pullable = 0;

	static int first_time = 1;
	static FILE* fp;

	if (first_time)
	{
		first_time = 0;
		fp = fopen("c:\\data\\corral_log.txt", "w");
	}

	for (i = 0 ; i < height ; i++)
		for (j = 0; j < width; j++)
		{
			if ((b[i][j] & BOX) == 0) continue;

			for (k = 0 ; k < 4 ; k++)
				if (b[i + delta_y[k]][j + delta_x[k]] & SOKOBAN)
					if (b[i + 2 * delta_y[k]][j + 2 * delta_x[k]] & SOKOBAN)
					{
						pullable++;
						break;
					}

		}

	int cloud = size_of_sokoban_cloud(b);

	fprintf(fp, "%d %d %d\n", cloud, pullable, res);
}*/