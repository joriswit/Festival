// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include <stdlib.h>
#include <malloc.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

#include "util.h"
#include "distance.h"
#include "sol.h"
#include "io.h"
#include "util.h"
#include "tree.h"

int level_sol_pushes = 0;
int level_sol_moves = 0;
int global_total_pushes = 0;
int global_total_moves = 0;
int total_sol_time = 0;
int total_levels_tried = 0;
int total_levels_solved = 0;


int find_sol_move(board b_in,
	int start_sok_index, int start_box_index, 
	int end_sok_index, int end_box_index,
	char *moves)
{
	int space_size;
	int i, j;

	int current_num = 0;
	int next_num = 0;
	int sok_index, box_index, sok_y, sok_x, box_y, box_x;
	int next_sok_y, next_sok_x, next_box_y, next_box_x;
	int current_index, next_index, target_index;
	char m;
	int moves_num = 0;
	int *current, *next, *src;
	board b;

	space_size = index_num * index_num;

	current = (int*)malloc(sizeof(int) * space_size);
	next    = (int*)malloc(sizeof(int) * space_size);
	src     = (int*)malloc(sizeof(int) * space_size);

	if ((current == 0) || (next == 0)  || (src == 0))
		exit_with_error("can't allocate memory for sol");

	copy_board(b_in, b);
	index_to_y_x(start_box_index, &box_y, &box_x);
	if ((b[box_y][box_x] & BOX) == 0) 
		exit_with_error("missing box1");
	b[box_y][box_x] &= ~BOX;

	for (i = 0; i < space_size; i++)
		src[i] = -1;

	target_index = start_sok_index * index_num + start_box_index;

	current[0] = end_sok_index * index_num + end_box_index; // we'll start from the end and pull back
	src[current[0]] = current[0];
	current_num = 1;

	while (current_num)
	{
		next_num = 0;

		for (i = 0; i < current_num; i++)
		{
			current_index = current[i];

			sok_index = current_index / index_num;
			box_index = current_index % index_num;

			index_to_y_x(sok_index, &sok_y, &sok_x);
			index_to_y_x(box_index, &box_y, &box_x);

			// try to move sokoban
			for (j = 0; j < 4; j++)
			{
				next_sok_y = sok_y + delta_y[j];
				next_sok_x = sok_x + delta_x[j];

				if ((next_sok_y == box_y) && (next_sok_x == box_x))
					continue; // the player can't move to the box position

				if (b[next_sok_y][next_sok_x] & OCCUPIED) continue;

				next_index = y_x_to_index(next_sok_y, next_sok_x) * index_num + box_index;
				if (src[next_index] != -1) continue;

				src[next_index] = current_index;
				next[next_num++] = next_index;
			}

			// try to pull box
			for (j = 0; j < 4; j++)
			{
				next_box_y = box_y + delta_y[j];
				next_box_x = box_x + delta_x[j];

				if (next_box_y != sok_y) continue;
				if (next_box_x != sok_x) continue;

				next_sok_y = sok_y + delta_y[j];
				next_sok_x = sok_x + delta_x[j];

				if (b[next_sok_y][next_sok_x] & OCCUPIED) continue;

				next_index = y_x_to_index(next_sok_y, next_sok_x) * index_num +
					y_x_to_index(next_box_y, next_box_x);

				if (src[next_index] != -1) continue;

				src[next_index] = current_index;
				next[next_num++] = next_index;
			}

			if (next_num >= space_size)
				exit_with_error("next_num overflow");
		} // loop on current

		current_num = 0;
		for (i = 0; i < next_num; i++)
			current[current_num++] = next[i];

		if (src[target_index] != -1)
			break; // early stop when a path is found

	} // while current_num

	// now display the path

	current_index = target_index;

	if (src[current_index] == -1)
		exit_with_error("could not find path");

	next_index = src[current_index];

	while (next_index != current_index)
	{
		index_to_y_x(current_index / index_num, &sok_y, &sok_x);

		index_to_y_x(next_index / index_num, &next_sok_y, &next_sok_x);
		index_to_y_x(next_index % index_num, &next_box_y, &next_box_x);

		m = ' ';
		if (next_sok_y == (sok_y + 1)) m = 'd';
		if (next_sok_y == (sok_y - 1)) m = 'u';
		if (next_sok_x == (sok_x + 1)) m = 'r';
		if (next_sok_x == (sok_x - 1)) m = 'l';

		if (m == ' ')
			exit_with_error("move error");

		if ((current_index % index_num) != (next_index % index_num))
			m = m + 'A' - 'a';

//		printf("%c", m);
		moves[moves_num++] = m;

		current_index = next_index;
		next_index = src[current_index];
	}

	free(current);
	free(next);
	free(src);

	return moves_num;
}


void write_solution_header()
{
	FILE *fp;
	char filename[2000];

#ifndef LINUX
	sprintf(filename, "%s\\solutions.sok", global_dir);
#else
	sprintf(filename, "%s/solutions.sok", global_dir);
#endif

	if (global_output_filename[0]) strcpy(filename, global_output_filename);

	fp = fopen(filename, "w");
	if (fp)
	{
		fprintf(fp, "%s solution file\n", solver_name);
		fprintf(fp, "===========================\n");
		fprintf(fp, "Collection: %s\n", global_level_set_name);
		fclose(fp);
	}
}

void save_level_to_solution_file(board b)
{
	FILE *fp;
	char filename[2000];

	if ((height == 0) || (width == 0)) return;

#ifndef LINUX
	sprintf(filename, "%s\\solutions.sok", global_dir);
#else
	sprintf(filename, "%s/solutions.sok", global_dir);
#endif

	if (global_output_filename[0]) strcpy(filename, global_output_filename);

	fp = fopen(filename, "a");
	if (fp == NULL) return;

	fprintf(fp, "\n");

	if (strcmp(level_title, "None") == 0)
	{
		fprintf(fp, "Level %d\n", level_id);
	}
	else
	{
		fprintf(fp, "%s\n", level_title);
	}
	fprintf(fp, "\n");

	save_board_to_file(b, fp);
	fprintf(fp, "\n");

	fclose(fp);
}


void store_solution_in_helper(tree *t, expansion_data *e, helper *h)
{
	int i, n;

	expansion_data* solution_path[MAX_SOL_LEN];
	move move_played[MAX_SOL_LEN];

	n = e->depth + 1; // number of nodes on the solution path

	if (n >= MAX_SOL_LEN) exit_with_error("solution is too long");

	while (e)
	{
		solution_path[e->depth] = e;
		e = e->father;
	}

	if (solution_path[0] != t->expansions) exit_with_error("does not start at root");

	for (i = 1; i < n; i++)
	{
		move_played[i] = get_move_to_node(t, solution_path[i]);
		h->sol_move[i - 1] = move_played[i];
	}
	h->sol_len = n - 1;
}


int count_pushes(char *sol, int len)
{
	int i;
	int sum = 0;

	for (i = 0; i < len; i++)
	{
		if (sol[i] == 'U') sum++;
		if (sol[i] == 'R') sum++;
		if (sol[i] == 'D') sum++;
		if (sol[i] == 'L') sum++;
	}
	return sum;
}


void save_sol_moves(helper *h)
{
	int sok_y, sok_x, next_sok_y, next_sok_x;
	int start_box_index, end_box_index;
	int end_box_y, end_box_x;
	move m;
	int i,n;
	board b;
	int moves_num;
	char filename[2000];
	FILE *fp;
	char *buffer;

	if (h->sol_len == 0) return;

#ifndef LINUX
	sprintf(filename, "%s\\solutions.sok", global_dir);
#else
	sprintf(filename, "%s/solutions.sok", global_dir);
#endif

	if (global_output_filename[0]) strcpy(filename, global_output_filename);

	fp = fopen(filename, "a");
	if (fp == 0) return;

	fprintf(fp, "Solution\n");

	sok_y = global_initial_sokoban_y;
	sok_x = global_initial_sokoban_x;

	copy_board(initial_board, b);

	level_sol_moves = 0;
	level_sol_pushes = 0;

	n = h->sol_len;

	buffer = (char *)malloc(100000);

	for (i = 0; i < n; i++)
	{
		m = h->sol_move[i];
		start_box_index = m.from;
		end_box_index = m.to;

		index_to_y_x(end_box_index, &end_box_y, &end_box_x);

		next_sok_y = end_box_y + delta_y[m.sokoban_position];
		next_sok_x = end_box_x + delta_x[m.sokoban_position];

		moves_num = find_sol_move(b, y_x_to_index(sok_y, sok_x), start_box_index,
			y_x_to_index(next_sok_y, next_sok_x), end_box_index, buffer);

		level_sol_moves += moves_num;
		level_sol_pushes += count_pushes(buffer, moves_num);

		fwrite(buffer, 1, moves_num, fp);

		sok_y = next_sok_y;
		sok_x = next_sok_x;

		apply_move(b, &m, NORMAL);
	}

	fprintf(fp, "\n");
	fclose(fp);

	free(buffer);
}





void write_log_header()
{
	FILE *fp;
	char s[2000];
	char date_string[200];
	char time_string[200];

	if (YASC_mode) return;

#ifndef LINUX
	sprintf(s, "%s\\times.txt", global_dir);
#else
	sprintf(s, "%s/times.txt", global_dir);
#endif

	get_date_as_string(time_string, date_string);

	fp = fopen(s, "w");
	if (fp == NULL)
		return;

	fprintf(fp, "--------------------------------------------------------------------------------------\n");
	fprintf(fp, "Sokoban Solver Statistics - Festival\n");
	fprintf(fp, "Date: %s  %s\n", date_string, time_string);
	fprintf(fp, "--------------------------------------------------------------------------------------\n");
	fprintf(fp, "   Task       Time Plugin               Result                     Moves  Pushes Level\n");
	fprintf(fp, "--------------------------------------------------------------------------------------\n");
	fclose(fp);

}

void save_level_log(int solved)
{
	FILE *fp;
	static int record_num = 1;
	int sol_time;
	char s[2000];

	if (YASC_mode) return;

	sol_time = end_time - start_time;
	total_sol_time += sol_time;

	total_levels_tried++;
	if (solved)
		total_levels_solved++;

#ifndef LINUX
	sprintf(s, "%s\\times.txt", global_dir);
#else
	sprintf(s, "%s/times.txt", global_dir);
#endif


	fp = fopen(s, "a");
	if (fp == NULL)
		return;

	fprintf(fp, "%7d  ", record_num++);
	get_sol_time_as_hms(sol_time, s);

	fprintf(fp, " %s ", s);

	fprintf(fp, "%-21s", solver_name);

	if (solved)
	{
		fprintf(fp, "OK                     ");
	}
	else
	{
		sprintf(s, "%-23s", global_fail_reason);
		fprintf(fp, "%s", s);
	}

	if (solved)
	{
		fprintf(fp, "%9d", level_sol_moves);
		fprintf(fp, "%8d ", level_sol_pushes);
	}
	else
		fprintf(fp, "                  ");

	fprintf(fp, "%s\n", level_title);

	fclose(fp);

	if (solved)
	{
		global_total_moves += level_sol_moves;
		global_total_pushes += level_sol_pushes;
	}
}

void write_log_footer()
{
	FILE *fp;
	int failed,i;
	char s[2000];

	if (YASC_mode) return;

	failed = total_levels_tried - total_levels_solved;
#ifndef LINUX
	sprintf(s, "%s\\times.txt", global_dir);
#else
	sprintf(s, "%s/times.txt", global_dir);
#endif

	fp = fopen(s, "a");
	if (fp == NULL)
		return;

	fprintf(fp, "--------------------------------------------------------------------------------------\n");
	fprintf(fp, ":%6d  ", total_levels_tried);

	get_sol_time_as_hms(total_sol_time, s);
	fprintf(fp, " %s ", s);

	sprintf(s, "Total                OK: %d Failed: %d      ", total_levels_solved, failed);
	fprintf(fp, "%s", s);

	for (i = (int)strlen(s); i < 44; i++)
		fprintf(fp, " ");


	fprintf(fp, "%9d", global_total_moves);
	fprintf(fp, "%8d", global_total_pushes);
	fprintf(fp, "     :\n");
	fprintf(fp,"--------------------------------------------------------------------------------------\n");
	fclose(fp);
}

void save_times_to_solutions_file(int sol_time)
{
	char filename[1000];
	FILE *fp;
	char hms[200];
	char date[200];
	char time[200];

#ifndef LINUX
	sprintf(filename, "%s\\solutions.sok", global_dir);
#else
	sprintf(filename, "%s/solutions.sok", global_dir);
#endif

	if (global_output_filename[0]) strcpy(filename, global_output_filename);

	fp = fopen(filename, "a");
	if (fp == 0) return;

	get_sol_time_as_hms(sol_time, hms);
	get_date_as_string(time, date);

	fprintf(fp, "\n");
	fprintf(fp, "Solver: %s", solver_name);
	fprintf(fp, "\n");

	fprintf(fp, "Solver time: %s\n", hms);
	fprintf(fp, "Solver date: %s  %s\n", date, time);
	fprintf(fp, "\n");
	fclose(fp);
}
