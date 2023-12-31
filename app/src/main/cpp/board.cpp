// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include <stdio.h>

#include "board.h"
#include "bfs.h"
#include "io.h"
#include "hungarian.h"
#include "distance.h"
#include "util.h"

board initial_board;
board inner;

int_board y_x_to_index_table;
int index_to_x[MAX_INNER];
int index_to_y[MAX_INNER];

int index_num;

void copy_board(board from, board to)
{
	int i, j;

	for (i = 0; i < height; i++)
	for (j = 0; j < width; j++)
		to[i][j] = from[i][j];
}

void save_initial_board(board b)
{
	copy_board(b, initial_board);

	get_sokoban_position(b, &global_initial_sokoban_y, &global_initial_sokoban_x);
	global_boxes_in_level = boxes_in_level(b);
}


void get_sokoban_position(board b, int *y, int *x)
{
	int i,j;

	for (i = 0; i < height ; i++)
	for (j = 0; j < width; j++)
	{
		if (b[i][j] & SOKOBAN)
		{
			*y = i;
			*x = j;
			return;
		}
	}
	print_board(b);
	exit_with_error("sokoban not found\n");
}

void init_inner(board b)
{
	board c;
	int i, j;

	clear_boxes(b, c);
	expand_sokoban_cloud(c);

	// set inner

	for (i = 0; i < height; i++)
	for (j = 0; j < width; j++)
	{
		inner[i][j] = 0;
		if (c[i][j] & SOKOBAN)
			inner[i][j] = 1;
	}
}




int y_x_to_index(int y, int x)
{
	int val = y_x_to_index_table[y][x];

	if (val < 0)
		exit_with_error("y_x_to_index");

	if (val >= index_num)
	{
		exit_with_error("val out of range");
	}

	return val;
}

void index_to_y_x(int index, int *y, int *x)
{
	if ((index < 0) || (index >= index_num))
	{
		printf("index: %d index_num: %d\n", index, index_num);
		exit_with_error("index_error");
	}

	*x = index_to_x[index];
	*y = index_to_y[index];
}

void init_index_x_y()
{
	int i, j;
	index_num = 0;

	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			y_x_to_index_table[i][j] = -1;

			if (inner[i][j] == 0)
				continue;

			y_x_to_index_table[i][j] = index_num;
			index_to_x[index_num] = j;
			index_to_y[index_num] = i;
			index_num++;

			if (index_num >= MAX_INNER)
				exit_with_error("inner too big");

		}
	}
}

void clear_boxes(board b, board board_without_boxes)
{
	int i, j;

	copy_board(b, board_without_boxes);

	for (i = 0; i < height; i++)
	for (j = 0; j < width; j++)
		board_without_boxes[i][j] &= ~BOX;
}

void clear_boxes_inplace(board b)
{
	int i, j;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			b[i][j] &= ~BOX;
}

void clear_sokoban_inplace(board b)
{
	int i, j;

	for (i = 0; i < height; i++)
	for (j = 0; j < width; j++)
		b[i][j] &= ~SOKOBAN;
}

int board_contains_boxes(board b)
{
	int i, j;

	for (i = 0; i < height; i++)
	for (j = 0; j < width; j++)
	if (b[i][j] & BOX)
		return 1;
	return 0;
}


void turn_boxes_into_walls(board b)
{
	int i, j;

	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			if (b[i][j] & BOX)
				b[i][j] = WALL;
		}
	}
}


void expand_sokoban_cloud(board b)
{
	int queue_y[MAX_SIZE*MAX_SIZE];
	int queue_x[MAX_SIZE*MAX_SIZE];

	int i, j, y, x, next_y, next_x;
	int queue_len = 0;
	int queue_pos = 0;

	// init queue from existing sokoban positions
	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (b[i][j] & SOKOBAN)
			{
				queue_y[queue_len] = i;
				queue_x[queue_len] = j;
				queue_len++;
			}

	while (queue_pos < queue_len)
	{
		y = queue_y[queue_pos];
		x = queue_x[queue_pos];

		for (i = 0; i < 4; i++)
		{
			next_y = y + delta_y[i];
			next_x = x + delta_x[i];

			if (b[next_y][next_x] & SOKOBAN) continue;
			if (b[next_y][next_x] & OCCUPIED) continue;

			b[next_y][next_x] |= SOKOBAN;
			queue_y[queue_len] = next_y;
			queue_x[queue_len] = next_x;
			queue_len++;
		}
		queue_pos++;
	}
}


void expand_sokoban_cloud_for_graph(board b)
{
	int queue_y[MAX_SIZE*MAX_SIZE];
	int queue_x[MAX_SIZE*MAX_SIZE];

	int i, j, y, x, next_y, next_x;
	int queue_len = 0;
	int queue_pos = 0;

	// init queue from existing sokoban positions
	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (b[i][j] & SOKOBAN)
			{
				queue_y[queue_len] = i;
				queue_x[queue_len] = j;
				queue_len++;
			}

	while (queue_pos < queue_len)
	{
		y = queue_y[queue_pos];
		x = queue_x[queue_pos];

		for (i = 0; i < 4; i++)
		{
			next_y = y + delta_y[i];
			next_x = x + delta_x[i];

			if (b[next_y][next_x] & SOKOBAN) continue;
			if (b[next_y][next_x] & OCCUPIED) continue;

			b[next_y][next_x] |= SOKOBAN;
			queue_y[queue_len] = next_y;
			queue_x[queue_len] = next_x;
			queue_len++;
		}
		queue_pos++;
	}
}



UINT_64 get_board_hash(board b)
{
	UINT_64 hash = 1;
	int i, j;

	for (i = 0; i < height; i++)
	for (j = 0; j < width; j++)
		hash = hash * 12345 + b[i][j] + (hash >> 32);

	return hash;
}

void board_to_bytes(board b, UINT_8 *data)
{
	int i, j;
	int pos = 0;

	for (i = 0; i < height; i++)
	for (j = 0; j < width; j++)
		data[pos++] = b[i][j];
}


void bytes_to_board(UINT_8 *data, board b)
{
	int i, j;
	int pos = 0;

	for (i = 0; i < height; i++)
	for (j = 0; j < width; j++)
		b[i][j] = data[pos++];
}

int all_boxes_in_place(board b, int pull_mode)
{
	int i, j;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if ((b[i][j] & BOX) == 0) continue;

			if (pull_mode == 0)
			{
				if ((b[i][j] & TARGET) == 0) return 0; 
			}
			else
			{
				if ((initial_board[i][j] & BOX) == 0) return 0;
				// don't use BASES because they don't exist in paper mode
			}
		}
	return 1;
}

int board_is_solved(board b, int pull_mode)
{
	if (pull_mode)
	{
		if ((b[global_initial_sokoban_y][global_initial_sokoban_x] & SOKOBAN) == 0)
			return 0;
	}

	return all_boxes_in_place(b, pull_mode);
}

void enter_reverse_mode(board b, int mark_bases)
{
	int i, j;

	clear_sokoban_inplace(b);

	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			if (b[i][j] == BOX)
				b[i][j] = 0;

			if (b[i][j] == TARGET)
				b[i][j] = BOX | TARGET;

			if (mark_bases)
				if (initial_board[i][j] & BOX)
					b[i][j] |= BASE;
		}
	}
}

void rev_board(board b)
{
	int i, j;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			b[i][j] ^= 1;
}

void print_board(board b)
{
	show_board(b);
}


int boxes_on_targets(board b)
{
	int i, j;
	int sum = 0;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if ((b[i][j] & ~BASE) == (BOX | TARGET))
				sum++;

	return sum;
}

int boxes_on_bases(board b)
{
	int i, j;
	int sum = 0;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if ((b[i][j] & BOX) == 0) continue;

			if (initial_board[i][j] & BOX)
				sum++;
		}

	return sum;
}


int boxes_in_level(board b)
{
	int i, j;
	int sum = 0;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (b[i][j] & BOX)
				sum++;
	return sum;
}

int is_same_board(board a, board b)
{
	int i, j;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (a[i][j] != b[i][j])
				return 0;
	return 1;
}


void zero_board(board b)
{
	int i, j;
	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			b[i][j] = 0;
}

void turn_targets_into_walls(board b)
{
	int i, j;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (b[i][j] & TARGET)
				b[i][j] = WALL;
}


void show_on_initial_board(board data)
{
	board b;
	int i, j;

	clear_boxes(initial_board, b);
	clear_sokoban_inplace(b);
	
	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (data[i][j])
			{
				if ((b[i][j] != 0) && (b[i][j] != TARGET))
				{
					printf("data %d %d is %d\n", i, j, data[i][j]);
					exit_with_error("bad value\n");
				}
				b[i][j] |= SOKOBAN;
			}

	print_board(b);
}

int board_popcnt(board x)
{
	int i, j;
	int sum = 0;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			sum += x[i][j];
	return sum;
}

void mark_indices_with_boxes(board b, int *indices)
{
	int i,y,x;
	for (i = 0; i < index_num; i++)
	{
		index_to_y_x(i, &y, &x);
		indices[i] = (b[y][x] & BOX ? 1 : 0);
	}
}

int count_walls_around(board b, int y, int x)
{
	int i;
	int sum = 0;
	for (i = 0; i < 4; i++)
		if (b[y + delta_y[i]][x + delta_x[i]] == WALL)
			sum++;
	return sum;
}

void negate_board(board b)
{
	int i, j;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if (b[i][j] > 1)
				exit_with_error("bad neg value");
			b[i][j] = 1 - b[i][j];
		}
}

void clear_targets_inplace(board b)
{
	int i, j;
	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			b[i][j] &= ~TARGET;
}


void print_board_with_zones(board b, int zones[MAX_SIZE][MAX_SIZE])
{
	int i, j;

	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			if (b[i][j] & WALL)
			{
				printf("#");
			}
			else
			{
				if (inner[i][j] == 0)
					printf(" ");
				else
				{
					if (zones[i][j] == 1000000)
						printf(" ");
					else
						printf("%d", zones[i][j]);
				}
			}
		}
		printf("\n");
	}
}


void turn_targets_into_packed(board b)
{
	int i, j;
	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (b[i][j] & TARGET)
				b[i][j] = PACKED_BOX;
}


void keep_boxes_in_inner(board b)
{
	int i, j;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if (inner[i][j] == 0)
				if (b[i][j] & BOX)
					b[i][j] = WALL;
		}
}



void zero_int_board(int_board a)
{
	int i, j;
	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			a[i][j] = 0;
}

void clear_bases_inplace(board b)
{
	int i, j;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			b[i][j] &= ~BASE;
}

void get_box_lacations_as_board(board b, board boxes)
{
	int i, j;
	zero_board(boxes);
	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (b[i][j] & BOX)
				boxes[i][j] = 1;

}

void and_board(board a, board mask)
{
	int i, j;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			a[i][j] &= mask[i][j];
}


void copy_int_board(int_board from, int_board to)
{
	int i, j;
	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			to[i][j] = from[i][j];
}


int all_targets_filled(board b)
{
	int i, j;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if ((b[i][j] & ~SOKOBAN) == TARGET)
				return 0;
	return 1;
}

void go_to_final_position_with_given_walls(board b)
{
	int i, j;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if (b[i][j] == WALL) continue;

			if (b[i][j] & TARGET)
				b[i][j] |= BOX;
			else
				b[i][j] &= ~BOX;
		}
}

int get_indices_of_boxes(board b, int *indices)
{
	int i, y, x;
	int n = 0;

	for (i = 0; i < index_num; i++)
	{
		index_to_y_x(i, &y, &x);
		if (b[y][x] & BOX)
			indices[n++] = i;
	}
	return n;
}

int get_indices_of_targets(board b, int *indices)
{
	int i, y, x;
	int n = 0;

	for (i = 0; i < index_num; i++)
	{
		index_to_y_x(i, &y, &x);
		if (b[y][x] & TARGET)
			indices[n++] = i;
	}
	return n;
}

int get_indices_of_initial_bases(board b, int *indices)
{
	int i, y, x;
	int n = 0;

	for (i = 0; i < index_num; i++)
	{
		index_to_y_x(i, &y, &x);
		if (initial_board[y][x] & BOX)
			indices[n++] = i;
	}
	return n;
}

void put_sokoban_everywhere(board b)
{
	int i,y,x;

	for (i = 0; i < index_num; i++)
	{
		index_to_y_x(i, &y, &x);
		if ((b[y][x] & OCCUPIED) == 0)
			b[y][x] |= SOKOBAN;
	}
}

void put_sokoban_in_zone(board b, int_board zones, int z)
{
	int i, j;
	clear_sokoban_inplace(b);
	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (zones[i][j] == z)
				if ((b[i][j] & OCCUPIED) == 0)
					b[i][j] |= SOKOBAN;
	expand_sokoban_cloud(b);
}

int size_of_sokoban_cloud(board b)
{
	int i, j;
	int sum = 0;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (b[i][j] & SOKOBAN)
				sum++;
	return sum;
}

void get_untouched_areas(board b, board untouched)
{
	int i, j;

	zero_board(untouched);

	for (i = 0 ; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if (inner[i][j] == 0) continue;
			if (b[i][j] & OCCUPIED) continue;

			if ((b[i][j] & SOKOBAN) == 0)
				untouched[i][j] = 1;
		}
}


void print_with_bases(board b_in)
{
	int i, j;
	board b;

	copy_board(b_in, b);

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			b[i][j] &= ~TARGET;
			if (initial_board[i][j] & BOX)
				b[i][j] |= BASE;
		}
	print_board(b);
}

void print_int_board(int_board b, int with_walls)
{
	int i, j;

	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			if ((with_walls) && (initial_board[i][j] == WALL))
			{
				printf(" # ");
				continue;
			}

			if (b[i][j] == 1000000)
			{
				printf(" - ");
				continue;
			}

			printf("%2d ", b[i][j]);
		}
		printf("\n");
	}
	printf("\n");
}