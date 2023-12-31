#include <stdlib.h>
#include <stdio.h>

#include "biconnect.h"


void biconnect_dfs(board b, int y, int x, int father, int d,
	board visited, int_board low, board on_cycle)
{
	int i, next_y, next_x;
//	int occupied = 0;

	visited[y][x] = 1;
	low[y][x] = d;

	for (i = 0; i < 4; i++)
	{
		next_y = y + delta_y[i]; next_x = x + delta_x[i];

		if (b[next_y][next_x] & OCCUPIED) continue;
		if (visited[next_y][next_x]) continue;

		biconnect_dfs(b, next_y, next_x, (i + 2) & 3, d + 1, visited, low, on_cycle);
	}

	for (i = 0; i < 4; i++)
	{
		if (i == father) continue;
		next_y = y + delta_y[i]; 
		next_x = x + delta_x[i];
		if (low[next_y][next_x] < low[y][x]) low[y][x] = low[next_y][next_x];
		if (low[next_y][next_x] < d) on_cycle[next_y][next_x] = 1;

//		if (b[next_y][next_x] & OCCUPIED) occupied++;
	}

	if (low[y][x] < d)
		on_cycle[y][x] = 1;
	
	/*
	if ((occupied == 3) && (father != -1))
	{
		i = (father + 2) & 3;
		next_y = y + delta_y[i]; 
		next_x = x + delta_x[i];

		if ((b[next_y][next_x] == WALL) || (b[next_y][next_x] & TARGET))
		{
			on_cycle[y][x] = 1;
			b[y][x] = WALL;
		}
	}
	*/
	
}

void show_cycles(board b_in, board on_cycle)
{
	board b;
	copy_board(b_in, b);
	int i, j;

	clear_sokoban_inplace(b);

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (on_cycle[i][j])
				b[i][j] |= SOKOBAN;

	printf("\ncycles:\n");
	print_board(b);
}


int biconnect_score(board b_in, int report)
{
	int i, j;
	board b, visited, on_cycle;
	int_board low;

	copy_board(b_in, b);

	for (i = 0 ; i < height ; i++)
		for (j = 0; j < width; j++)
		{
			visited[i][j] = 0;
			on_cycle[i][j] = 0;
			low[i][j] = 1000000;
		}

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if (inner[i][j] == 0) continue;
			if (visited[i][j]) continue;
			if (b[i][j] & OCCUPIED) continue;

			biconnect_dfs(b, i, j, -1, 0, visited, low, on_cycle);
		}

	// allow pulling boxes from target areas even if it creates tunnels
	// (which increase biconnectivity)
	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if ((b[i][j] & TARGET) && (b[i][j] & BOX) == 0)
				on_cycle[i][j] = 1; 


	if (report)
		show_cycles(b_in, on_cycle);

	return index_num - global_boxes_in_level - board_popcnt(on_cycle);
}

