// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include <stdio.h>
#include <string.h>

#include "rooms.h"
#include "bfs.h"
#include "util.h"

int_board rooms_board;
int rooms_num;

int base_graph[MAX_ROOMS][MAX_ROOMS];

void rooms_bfs(board b, int start_y, int start_x, int color, 
	int seen_during_bfs[MAX_ROOMS], board visited)
{
	// this BFS is allowed to spread either in "color" room or in corridors.
	// It stops when it reaches other rooms.

	int current_x[MAX_SIZE * 4];
	int current_y[MAX_SIZE * 4];
	int next_x[MAX_SIZE * 4];
	int next_y[MAX_SIZE * 4];
	int current_size, next_size;
	int i, j, y, x;
	int seen_color;
	board bfs_visited;

	zero_board(bfs_visited);

	for (i = 0; i < rooms_num; i++)
		seen_during_bfs[i] = 0;

	current_x[0] = start_x;
	current_y[0] = start_y;
	current_size = 1;
	visited[start_y][start_x] = 1;
	bfs_visited[start_y][start_x] = 1;

	do
	{
		next_size = 0;
		for (i = 0; i < current_size; i++)
		{
			for (j = 0; j < 4; j++)
			{
				x = current_x[i] + delta_x[j];
				y = current_y[i] + delta_y[j];

				seen_color = rooms_board[y][x];
				if ((seen_color != 1000000) && (seen_color != color))
				{
					seen_during_bfs[seen_color] = 1;
					continue;
				}

				if (bfs_visited[y][x]) continue; // handled during current BFS
				if (visited[y][x]) 	continue;  // handled during previous BFSs
				if (b[y][x] & OCCUPIED) continue;


				bfs_visited[y][x] = 1; // this is mostly for corridors

				if (seen_color == color)
					visited[y][x] = 1;  // this is for next BFS searches

				next_x[next_size] = x;
				next_y[next_size] = y;
				next_size++;

				if (next_size >= (MAX_SIZE * 4)) exit_with_error("next size too big");
			} 	 // j = 0..3
		} // i = current_size

		for (i = 0; i < next_size; i++)
		{
			current_x[i] = next_x[i];
			current_y[i] = next_y[i];
		}
		current_size = next_size;
	} 
	while (current_size > 0);
}





void get_current_graph(board b, int current_graph[MAX_ROOMS][MAX_ROOMS])
{
	int i, j, k, color;
	int seen_during_bfs[MAX_ROOMS];
	board visited;
	int color_processed[MAX_ROOMS];

	zero_board(visited);
	// visited are the color rooms seen in previous BFSs

	for (i = 0; i < rooms_num; i++)
		for (j = 0; j < rooms_num; j++)
			current_graph[i][j] = 1;

	for (i = 0; i < rooms_num; i++)
		current_graph[i][i] = 0;

	for (i = 0; i < rooms_num; i++)
		color_processed[i] = 0;


	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			if (inner[i][j] == 0) continue;
			if (b[i][j] & OCCUPIED) continue;
			if (visited[i][j]) continue;

			color = rooms_board[i][j];
			if (color == 1000000) continue;

			color_processed[color] = 1;

            rooms_bfs(b, i, j, color, seen_during_bfs, visited);

			for (k = 0; k < rooms_num; k++)
			if (seen_during_bfs[k] == 0)
			{
				current_graph[color][k] = 0;
				current_graph[k][color] = 0;
			}		
		}
	}

	// apply correction for rooms full of boxes
	for (i = 0 ; i < rooms_num ; i++)
		if (color_processed[i] == 0)
		{
			for (j = 0; j < rooms_num; j++)
			{
				current_graph[i][j] = 0;
				current_graph[j][i] = 0;
			}
		}
}


void print_current_graph(int current_graph[MAX_ROOMS][MAX_ROOMS])
{
	int i, j;

	for (i = 0; i < rooms_num; i++)
	{
		for (j = 0; j < rooms_num; j++)
			printf("%d", current_graph[i][j]);
		printf("\n");
	}
	printf("\n");
}


void new_mark_rooms(board b)
{
	board squares;
	int i, j, v;
	int_board tmp_rooms_board;

	zero_board(squares);

	for (i = 0; i < (height - 1); i++)
		for (j = 0; j < (width - 1); j++)
		{
			if (b[i][j] == 0)
				if (b[i][j + 1] == 0)
					if (b[i + 1][j] == 0)
						if (b[i + 1][j + 1] == 0)
							squares[i][j] = 1;
		}

	// remove stand-alone squares - too small to be considered rooms

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if (squares[i][j] == 1)
				if (squares[i - 1][j] == 0)
					if (squares[i][j + 1] == 0)
						if (squares[i + 1][j] == 0)
							if (squares[i][j - 1] == 0)
								squares[i][j] = 0;
		}

	// look for connected components
	negate_board(squares);

	rooms_num = mark_connectivities(squares, tmp_rooms_board);

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			rooms_board[i][j] = 1000000;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{ 
			v = tmp_rooms_board[i][j];
			if (v == 1000000) continue;

			rooms_board[i][j] = v;
			rooms_board[i][j+1] = v;
			rooms_board[i+1][j] = v;
			rooms_board[i+1][j+1] = v;
		}

	// remove cells shared between rooms
	
	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if (squares[i][j] == squares[i + 1][j + 1])
				if (squares[i + 1][j] == squares[i][j + 1])
					if (squares[i][j] != squares[i + 1][j])
					{
						rooms_board[i + 1][j + 1] = 1000000;
					}
		}
		
}

int analyse_rooms(board b_inp)
{
	board b,c;
	int i, j;
	int current_graph[MAX_ROOMS][MAX_ROOMS];

	clear_boxes(b_inp, b);
	clear_sokoban_inplace(b);
	clear_targets_inplace(b);

    new_mark_rooms(b);

	if (verbose >= 5)
		printf("%d rooms\n", rooms_num);

	if (rooms_num > MAX_ROOMS)
	{
		strcpy(global_fail_reason, "too many rooms");
		rooms_num = 0;
		return 1;
	}

	if (verbose >= 5)
		print_board_with_zones(b, rooms_board);

	clear_boxes(b, c);
	get_current_graph(c, current_graph);
//	print_current_graph(current_graph);

	for (i = 0; i < rooms_num; i++)
		for (j = 0; j < rooms_num; j++)
			base_graph[i][j] = current_graph[i][j];

	return 1;
}


int score_rooms(board b)
{
	int i, j;
	int sum = 0;
	int current_graph[MAX_ROOMS][MAX_ROOMS];

	if (rooms_num <= 1)	return 0;

	get_current_graph(b, current_graph);

	for (i = 0; i < rooms_num;i++)
		for (j = 0; j < rooms_num; j++)
			sum += base_graph[i][j] - current_graph[i][j];

	if (sum % 2) exit_with_error("odd");

	return (sum / 2);
}

