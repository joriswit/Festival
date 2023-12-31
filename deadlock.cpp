// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include <string.h>
#include <stdio.h>

#include "moves.h"
#include "deadlock.h"
#include "distance.h"
#include "bfs.h"
#include "deadlock_utils.h"
#include "fixed_boxes.h"
#include "util.h"
#include "packing_search.h"
#include "park_order.h"
#include "advanced_deadlock.h"
#include "mpdb2.h"
#include "perimeter.h"
#include "k_dist_deadlock.h"
#include "overlap.h"
#include "imagine.h"
#include "textfile.h"

int debug_deadlock = 0;

board forbidden_push_tunnel;
board forbidden_pull_tunnel;

#define MAX_PATTERNS 400

typedef struct deadlock_pattern
{
	UINT_8 pat[10][10];
	int y;
	int x;
	int box_shift_x;
	int box_shift_y;
} deadlock_pattern;


deadlock_pattern patterns[MAX_PATTERNS];
int patterns_num = 0;

deadlock_pattern pull_patterns[MAX_PATTERNS];
int pull_patterns_num = 0;


void set_forbidden_tunnel()
{
	int i, j, k, n;
	board b;
	int base, target;

	zero_board(forbidden_push_tunnel);
	zero_board(forbidden_pull_tunnel);

	clear_boxes(initial_board, b);
	clear_sokoban_inplace(b);

	// look for
	// ######
	//
	// ######
	for (i = 1; i < (height - 1); i++)
	{
		for (j = 1; j < (width - 1); j++)
		{

			if (b[i - 1][j - 1] == WALL)
			if (b[  i  ][j - 1] != WALL)
			if (b[i + 1][j - 1] == WALL) continue; // middle of tunnel


			for (k = 0; k < width; k++)
			{
				if (b[i - 1][j + k] != WALL) break;
				if (b[  i  ][j + k] == WALL) break;
				if (b[i + 1][j + k] != WALL) break;
			}
			if (k >= 2)
			{
				n = k;

				base = target = 0;
				for (k = 0; k < n; k++)
				{
					if (initial_board[i][j + k] & BOX) base = 1;
					if (b[i][j + k] & TARGET) target = 1;
				}

				for (k = 0; k < n; k++)
				{
					forbidden_push_tunnel[i][j + k] = 1;
					forbidden_pull_tunnel[i][j + k] = 1;
				}

				if (base == 0)
					forbidden_pull_tunnel[i][j] = 0; 
				
				if (target == 0)
					forbidden_push_tunnel[i][j] = 0;			
			}
		}
	}

	// look for
	// # #
	// # #
	// # #
	for (i = 1; i < (height - 1); i++)
	{
		for (j = 1; j < (width - 1); j++)
		{
			if (b[i - 1][j - 1] == WALL)
			if (b[i - 1][  j  ] != WALL)
			if (b[i - 1][j + 1] == WALL) continue; // middle of tunnel


			for (k = 0; k < height; k++)
			{
				if (b[i + k][j - 1] != WALL) break;
				if (b[i + k][  j  ] == WALL) break;
				if (b[i + k][j + 1] != WALL) break;
			}
			if (k >= 2)
			{
				n = k;

				base = target = 0;
				for (k = 0; k < n; k++)
				{
					if (initial_board[i + k][j] & BOX) base = 1;
					if (b[i + k][j] & TARGET) target = 1;
				}


				for (k = 0; k < n; k++)
				{
					forbidden_push_tunnel[i + k][j] = 1;
					forbidden_pull_tunnel[i + k][j] = 1;
				}

				if (base == 0)
					forbidden_pull_tunnel[i][j] = 0;

				if (target == 0)
					forbidden_push_tunnel[i][j] = 0;
					
				
			}
		}
	}

	if (verbose >= 5)
	{
		printf("forbidden push tunnel:\n");
		show_on_initial_board(forbidden_push_tunnel);
		printf("forbidden pull tunnel:\n");
		show_on_initial_board(forbidden_pull_tunnel);
	}
}




int is_2x2_deadlock_at_place(board b, int i, int j)
{
	int y, x;

	for (y = 0; y < 2; y++)
		for (x = 0; x < 2; x++)
			if ((b[i + y][j + x] & OCCUPIED) == 0)
				return 0;

	for (y = 0; y < 2; y++)
		for (x = 0; x < 2; x++)
			if (b[i + y][j + x] & BOX)
				if ((b[i + y][j + x] & TARGET) == 0)
					return 1;
	return 0;
}


void mark_deadlock_2x2(board c_inp, board d)
{
	int i, j,y,x;
	int sum, stuck;
	board c;
	unsigned char val;

	copy_board(c_inp, c);
	clear_sokoban_inplace(c);

	zero_board(d);

	for (i = 0; i < (height - 1); i++)
	for (j = 0; j < (width - 1); j++)
	{

		// look for patterns composed of 3 boxes/walls such as
		// $?
		// #$

		sum = 0;
		stuck = 0; // boxes that will deadlock if they become fixed
		for (y = 0; y < 2; y++)
			for (x = 0; x < 2; x++)
			{
				val = c[i + y][j + x];
				if (val & OCCUPIED) sum++;
				if ((val != WALL) && ((val & TARGET) == 0)) stuck++;
				// it could be a box or an empty square, but immobilizing a box there would be a mistake
			}

		if (sum == 3) 
			if (stuck)
		{
			for (y = 0; y < 2; y++)
			for (x = 0; x < 2; x++)
//			if ((c[i + y][j + x] & TARGET) == 0)
				d[i + y][j + x] = 1;
		}
		
		// look for pattern
		// #? 
		// ?#

		if ((c[i][j] == WALL) && (c[i + 1][j + 1] == WALL))
		{
			if (c[i][j + 1] == 0)
				d[i][j + 1] = 1;
			if (c[i + 1][j] == 0)
				d[i + 1][j] = 1;
		}

		// look for pattern
		// ?# 
		// #?

		if ((c[i][j+1] == WALL) && (c[i + 1][j] == WALL))
		{
			if (c[i][j] == 0)
				d[i][j] = 1;
			if (c[i + 1][j + 1] == 0)
				d[i + 1][j + 1] = 1;
		}
		
	}

//	show_on_initial_board(d);
//	my_getch();
}



// constants for the next routine
int square_place_y[4][2] = {{ -2, -2 }, { -1, 0 }, { 1, 1 }, { -1, 0 }};
int square_place_x[4][2] = {{ -1, 0 }, { 1, 1 }, { -1, 0 }, { -2, -2 }};

int deadlock_in_direction(board b, int y, int x, int d)
{
	// check if a freeze deadlock will occur after pushing the box in direction d.

	//
	//   00
	//  3  1
	//  3 $1
	//   22

	int dy, dx, new_y, new_x;
	int dy2, dx2; // push sideway
	int res_0, res_1;
	int wall1 = 0, wall2 = 0, wall3 = 0, wall4 = 0;

	dy = delta_y[d];
	dx = delta_x[d];
	dy2 = delta_y[(d + 1) & 3]; // sideway
	dx2 = delta_x[(d + 1) & 3];
	new_y = y + dy;
	new_x = x + dx;

	if (inner[new_y][new_x] == 0) return 1; // safeguard before calling y_x_to_index
	if (impossible_place[y_x_to_index(new_y, new_x)]) 
		return 1;

	if ((b[y][x] & BOX) == 0) exit_with_error("missing box");
	if (b[new_y][new_x] & OCCUPIED) exit_with_error("no space");

	// first, look for two walls:
	//   #
	//  #$
	//   ^ = (y,x)


	if ( 
		((b[new_y][new_x] & TARGET) == 0) && 
		(b[new_y + dy][new_x + dx] == WALL)
		)
	{
		if (b[new_y + dy2][new_x + dx2] == WALL) return 1;
		if (b[new_y - dy2][new_x - dx2] == WALL) return 1;
	}

	// check patterns of type
	//   1  #$   2
	//   3   $#	 4
	//       ^

	if (b[new_y + dy][new_x + dx] & BOX)
	{
		if (b[new_y + dy2][new_x + dx2] == WALL) wall4 = 1;
		if (b[new_y - dy2][new_x - dx2] == WALL) wall3 = 1;
		if (b[new_y + dy + dy2][new_x + dx + dx2] == WALL) wall2 = 1;
		if (b[new_y + dy - dy2][new_x + dx - dx2] == WALL) wall1 = 1;

		if (((wall1 + wall4) == 2) || ((wall2 + wall3) == 2))
		{
			if ((b[new_y][new_x]           & TARGET) == 0) return 1;
			if ((b[new_y + dy][new_x + dx] & TARGET) == 0) return 1;
		}
	}

	// temporarily push the box and check 2x2 deadlocks
	b[y][x] &= ~BOX;
	b[new_y][new_x] |= BOX;

	res_0 = is_2x2_deadlock_at_place(b, y + square_place_y[d][0], x + square_place_x[d][0]);
	res_1 = is_2x2_deadlock_at_place(b, y + square_place_y[d][1], x + square_place_x[d][1]);

	// restore box
	b[new_y][new_x] &= ~BOX;
	b[y][x] |= BOX;

	if (res_0 || res_1) return 1;

	return 0;
}


int box_touches_unreachable_square(board b, int box_y, int box_x)
{
	int i, y, x;

	for (i = 0; i < 8; i++)
	{
		y = box_y + delta_y_8[i];
		x = box_x + delta_x_8[i];

		if (b[y][x] & OCCUPIED) continue;
		if (b[y][x] & SOKOBAN) continue;
		return 1;
	}
	return 0;
}


int box_is_pushable(board b, int box_y, int box_x)
{
	// a naive check if the box can be pushed vertically or horizontally

	if ((b[box_y - 1][box_x] & OCCUPIED) == 0)
	if ((b[box_y + 1][box_x] & OCCUPIED) == 0)
		return 1;

	if ((b[box_y][box_x - 1] & OCCUPIED) == 0)
	if ((b[box_y][box_x + 1] & OCCUPIED) == 0)
		return 1;

	return 0;
}

int is_freeze_deadlock(board b, int y, int x)
{
	// check patterns of type
	//    #O     #$    
	//     $#     *#

	// also:
	// #O
	//  BO
	//   #

	int i,j;
	board c;
	int changed = 1;

	if ((b[y][x] & BOX) == 0) exit_with_error("check caller");

	if (box_is_pushable(b, y, x))
		return 0;

	// eliminate all pushable boxes

	copy_board(b, c);

	while (changed)
	{
		changed = 0;

		for (i = 0; i < height; i++)
			for (j = 0; j < width; j++)
				if (c[i][j] & BOX)
					if (box_is_pushable(c, i, j))
					{
						c[i][j] &= ~BOX;
						changed = 1;
					}
	}

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (c[i][j] & BOX)
				if ((c[i][j] & TARGET) == 0)
					return 1;

	return 0;
}


int is_3x3_deadlock(board b, int box_y, int box_x)
{
	// look for patterns of type 
	// 
	// OO
	// O O
	//  OO
	//
	// or
	//
	// OO
	// O #
	// #O

	int y, x, i, val1, val2, val3;

	if (box_touches_unreachable_square(b, box_y, box_x) == 0) return 0;

	for (y = box_y - 1; y <= (box_y + 1); y++)
	{
		for (x = box_x - 1; x <= (box_x + 1); x++)
		{
			if (inner[y][x] == 0) continue;
			if (b[y][x] & OCCUPIED) continue;

			if (b[y][x] & TARGET) continue;

			// surrounded by four:

			for (i = 0; i < 4; i++)
				if ((b[y + delta_y[i]][x + delta_x[i]] & OCCUPIED) == 0)
					break;

			if (i < 4) continue;

			// at least one box is not in place

			for (i = 0; i < 4; i++)
				if (b[y + delta_y[i]][x + delta_x[i]] & BOX)
					if ((b[y + delta_y[i]][x + delta_x[i]] & TARGET) == 0)
						break;

			if (i == 4) continue;

			// OO
			// O O
			//  OO

			if (b[y-1][x-1] & OCCUPIED)
				if (b[y+1][x+1] & OCCUPIED)
					return 1;

			if (b[y - 1][x + 1] & OCCUPIED)
				if (b[y + 1][x - 1] & OCCUPIED)
					return 1;

			// OO
			// O #
			// #O

			for (i = 0; i < 8; i+=2)
			{
				val1 = b[y + delta_y_8[i]][x + delta_x_8[i]];
				if (val1 != WALL) continue;

				val2 = b[y + delta_y_8[(i+3)&7]][x + delta_x_8[(i+3)&7]];
				val3 = b[y + delta_y_8[(i+5)&7]][x + delta_x_8[(i+5)&7]];

				if ((val2 & OCCUPIED) == 0) continue;
				if ((val3 & OCCUPIED) == 0) continue;

				if ((val2 == WALL) || (val3 == WALL))
					return 1;
			}
		}
	}

	return 0;
}



int is_same_pattern(deadlock_pattern *a, deadlock_pattern *b)
{
	int i, j;

	if (a->y != b->y) return 0;
	if (a->x != b->x) return 0;

	for (i = 0; i < a->y; i++)
	for (j = 0; j < a->x; j++)
	if (a->pat[i][j] != b->pat[i][j])
		return 0;

	return 1;
}

void set_pattern_box_shift(deadlock_pattern *p)
{
	int i, j;
	for (i = p->y - 1; i >= 0; i--)
	{
		for (j = p->x - 1; j >= 0 ; j--)
		{
			if (p->pat[i][j] == 0xff)
				continue;

			if (p->pat[i][j] & BOX)
			{
				p->box_shift_y = i;
				p->box_shift_x = j;
				return;
			}
		}
	}
	exit_with_error("could not find box shift");
}

void add_pattern(deadlock_pattern *p, int pull_mode)
{
	int i;

	set_pattern_box_shift(p);

	if (pull_mode == 0)
	{
		for (i = 0; i < patterns_num; i++)
			if (is_same_pattern(p, patterns + i))
				return;

		patterns[patterns_num++] = *p;

		if (patterns_num >= MAX_PATTERNS)
			exit_with_error("too many patterns");
	}
	else
	{
		for (i = 0; i < pull_patterns_num; i++)
			if (is_same_pattern(p, pull_patterns + i))
				return;

		pull_patterns[pull_patterns_num++] = *p;

		if (pull_patterns_num >= MAX_PATTERNS)
			exit_with_error("too many patterns");
	}

}

void rotate_clockwise(deadlock_pattern *p)
{
	int i, j;

	deadlock_pattern q;

	q.x = p->y;
	q.y = p->x;

	for (i = 0; i < p->y; i++)
	for (j = 0; j < p->x; j++)
		q.pat[j][p->y - 1 - i] = p->pat[i][j];

	*p = q;
}

void flip_pattern(deadlock_pattern *p)
{
	int i, j;
	deadlock_pattern q;

	q = *p;

	for (i = 0; i < p->y; i++)
	for (j = 0; j < p->x; j++)
		q.pat[i][p->x - 1 - j] = p->pat[i][j];

	*p = q;
}


void add_pattern_rotations(deadlock_pattern *p, int pull_mode)
{
	int i;

	for (i = 0; i < 4; i++)
	{
		add_pattern(p, pull_mode);
		rotate_clockwise(p);
	}
	flip_pattern(p);
	for (i = 0; i < 4; i++)
	{
		add_pattern(p, pull_mode);
		rotate_clockwise(p);
	}
}


void print_pattern(deadlock_pattern *p)
{
	int i, j;

	for (i = 0; i < p->y; i++)
	{
		for (j = 0; j < p->x; j++)
		{
			switch (p->pat[i][j])
			{
			case WALL:
				printf("#");
				break;
			case BOX:
				printf("$");
				break;
			case SOKOBAN:
				printf("@");
				break;
			case SPACE:
				printf(" ");
				break;
			case 0xff:
				printf("?");
				break;
			case PACKED_BOX:
				printf("*");
				break;
			case SOKOBAN_ON_TARGET:
				printf("+");
				break;
			default:
				exit_with_error("unknown pat\n");
			}
		}
		printf("\n");
	}
	printf("\n");
}


void print_deadlock_patterns(int pull_mode)
{
	int i;

	if (pull_mode == 0)
	{
		for (i = 0; i < patterns_num; i++)
		{
			printf("deadlock pattern %d\n", i);
			print_pattern(patterns + i);
		}
	}
	else
	{
		for (i = 0; i < pull_patterns_num; i++)
		{
			printf("deadlock pattern %d\n", i);
			print_pattern(pull_patterns + i);
		}

	}
}


void read_deadlock_patterns(int pull_mode)
{
	char s[2000];
	int y, x, i, j;
	deadlock_pattern p;

	open_text_file(pull_mode);

	while (textfile_fgets(s, 1000))
	{
		if (s[0] == '/')
			continue;

		if (strstr(s, "PATTERN") == 0)
			continue;

		textfile_fgets(s, 1000);

		sscanf(s, "%d %d", &y, &x);

		if ((y < 1) || (y > 5)) exit_with_error("bad pattern\n");
		if ((x < 1) || (x > 5)) exit_with_error("bad pattern\n");


		for (i = 0; i < y; i++)
		{
			textfile_fgets(s, 1000);

			for (j = 0; j < x; j++)
			{
				switch (s[j])
				{
				case '#':
					p.pat[i][j] = WALL;
					break;
				case '$':
					p.pat[i][j] = BOX;
					break;
				case '?':
					p.pat[i][j] = 0xff;
					break;
				case '@':
					p.pat[i][j] = SOKOBAN;
					break;
				case ' ':
					p.pat[i][j] = SPACE;
					break;
				case '*':
					p.pat[i][j] = PACKED_BOX;
					break;
				case '+':
					p.pat[i][j] = SOKOBAN_ON_TARGET;
					break;
				default:
					exit_with_error("unknown pattern");
				}
			}
		}
		p.x = x;
		p.y = y;

		add_pattern_rotations(&p, pull_mode);
	}

	if (pull_mode == 0)
	{
//		print_deadlock_patterns(pull_mode);
//		print_pattern(patterns + 3);
//		my_getch();
	}
}




int pattern_at(deadlock_pattern *p, board b, int y, int x)
{
	int i, j;
	UINT_8 c;

	for (i = 0; i < p->y; i++)
	for (j = 0; j < p->x; j++)
	{
		c = p->pat[i][j];

		if (c == 0xff) continue;

		if (c != b[i + y][j + x])
			return 0;
	}
	return 1;
}

int check_deadlock_pattern(deadlock_pattern *p, board b, int box_y, int box_x)
{
	int start_x, start_y;
	int end_x, end_y;
	int x, y;
	int shift_y, shift_x;


	start_x = box_x - p->x + 1;
	start_y = box_y - p->y + 1;

	end_x = box_x + 1;
	end_y = box_y + 1;

	if (start_x < 0) start_x = 0;
	if (start_y < 0) start_y = 0;

	shift_y = p->box_shift_y;
	shift_x = p->box_shift_x;

	for (y = start_y; y < end_y; y++)
	{
		for (x = start_x; x < end_x; x++)
		{
			if ((b[y + shift_y][x + shift_x] & BOX) == 0) continue;

			if (pattern_at(p, b, y, x))
				return 1;
		}
	}

	return 0;
}



int check_all_patterns(board b_in, int box_y, int box_x, int pull_mode)
{
	int i;
	board b;

	if (box_touches_unreachable_square(b_in, box_y, box_x) == 0)
		return -1;

	if ((pull_mode) && (size_of_sokoban_cloud(b_in) > 4))
		return -1; // all pull patterns are <= 4
	
	clear_deadlock_zone(b_in, b);

	if (pull_mode == 0)
	{
		for (i = 0; i < patterns_num; i++)
			if (check_deadlock_pattern(patterns + i, b, box_y, box_x))
				return i;
	}
	else
	{
		if (b[global_initial_sokoban_y][global_initial_sokoban_x] & SOKOBAN)
			return -1;
		// in pull mode, it is ok to be confined in a corral containing the start position


		for (i = 0; i < pull_patterns_num; i++)
			if (check_deadlock_pattern(pull_patterns + i, b, box_y, box_x))
				return i;
	}

	return -1;
}

int is_inefficient_position(board b, int box_index, int pull_mode, int search_mode, helper *h)
{
	int box_y, box_x;
	int val[8];
	int i, j;

	// check positions like
	//  #
	// $ 
	// ##

	if (h->perimeter_found) return 0; 
	// allow inefficient moves once we reach the perimeter

	if (h->enable_inefficient_moves) return 0;

	if (search_mode == K_DIST_SEARCH) return 0;
	// allow all pulls in k_dist search, k_dist boards must be accurate

	index_to_y_x(box_index, &box_y, &box_x);

	// do not reject positions expected in the plan
	if (pull_mode == 0)
	{
		if (h->used_in_plan[box_y][box_x]) return 0;
		if ((search_mode == HF_SEARCH) && (h->imagined_hf_board[box_y][box_x]))
			return 0;
	}
	else
	{
		if ((search_mode == REV_SEARCH) && (h->imagined_hf_board[box_y][box_x]))
			return 0;
	}


	// never prune moves that push a box to a target or to a base
	if (pull_mode == 0)
	{
		if (b[box_y][box_x] & TARGET)
			return 0;
	}
	else
	{
		if (initial_board[box_y][box_x] & BOX)
			return 0;
	}


	if ((pull_mode == 0) && (forbidden_push_tunnel[box_y][box_x])) return 1;
	if ((pull_mode     ) && (forbidden_pull_tunnel[box_y][box_x])) return 1;


	for (i = 0; i < 8; i++)
		val[i] = b[box_y + delta_y_8[i]][box_x + delta_x_8[i]];

	if (pull_mode)
	{
		// check position:
		// #
		//  #
		// $
		// ##

		for (j = 0; j < 8; j += 2)
		{
			if ((val[(j + 0) & 7] & OCCUPIED) == 0)
			if (val[(j + 1) & 7] == WALL)
			if (val[(j + 3) & 7] == WALL)
			if (val[(j + 4) & 7] == WALL)
			{
				if (b[box_y + delta_y_8[j] * 2][box_x + delta_x_8[j] * 2] == WALL)
					return 1;
			}
		}

		
		// and symmetrical
		for (j = 0; j < 8; j += 2)
		{
			if ((val[(j + 0) & 7] & OCCUPIED) == 0)
			if (val[(j + 4) & 7] == WALL)
			if (val[(j + 5) & 7] == WALL)
			if (val[(j + 7) & 7] == WALL)
			{
				if (b[box_y + delta_y_8[j] * 2][box_x + delta_x_8[j] * 2] == WALL)
					return 1;
			}
		}
		
		return 0;
	}


	for (j = 0; j < 8; j+=2)
	{
		// check positions like
		//     
		//  -$# 
		//  # #
		//
		// last move must have been up/down, both are silly

		if ( val[(j + 2) & 7] == WALL)
		if ( val[(j + 3) & 7] == WALL)
		if ( val[(j + 5) & 7] == WALL)
		if ( val[(j + 6) & 7] != WALL)
		if ((val[(j + 6) & 7] & SOKOBAN) == 0)
		{
//			printf("type 1 %d %d\n", box_y, box_x);
			return 1;
		}

		if ( val[(j + 3) & 7] == WALL)
		if ( val[(j + 5) & 7] == WALL)
		if ( val[(j + 6) & 7] == WALL)
		if ( val[(j + 2) & 7] != WALL)
		if ((val[(j + 2) & 7] & SOKOBAN) == 0)
		{
//			printf("type 2\n");
			return 1;
		}

		// check positions like
		//   @ 
		//  -$# 
		//  # #
		//
		// could just as well park it
		
		if (val[(j + 2) & 7] == WALL)
		if (val[(j + 3) & 7] == WALL)
		if (val[(j + 5) & 7] == WALL)
		if (val[(j + 6) & 7] != WALL)
		if (val[(j + 0) & 7] & SOKOBAN)
		{
//			printf("type 3 j=%d\n" ,j);
			return 1;
		}

		if (val[(j + 3) & 7] == WALL)
		if (val[(j + 5) & 7] == WALL)
		if (val[(j + 6) & 7] == WALL)
		if (val[(j + 2) & 7] != WALL)
		if (val[(j + 0) & 7] & SOKOBAN)
		{
//			printf("type 4\n");
			return 1;
		}
		
		// check
		// ???
		// -$$
		// #?#
		if (val[(j + 3) & 7] == WALL)
		if (val[(j + 5) & 7] == WALL)
		if (val[(j + 2) & 7] & BOX)
			if ((val[(j + 6) & 7] & SOKOBAN) == 0)
			{
//				printf("type 5\n");
				return 1;
			}

		if (val[(j + 3) & 7] == WALL)
		if (val[(j + 5) & 7] == WALL)
		if (val[(j + 6) & 7] & BOX)
			if ((val[(j + 2) & 7] & SOKOBAN) == 0)
			{
//				printf("type 6\n");
				return 1;
			}
	}

	return 0;

}


int has_pull_continuation(board b, int search_mode)
{
	int i, j,k;
	int sok_y, sok_x;
	int next_sok_y, next_sok_x;
	int all_boxes_solved = 1;

	// check if all boxes are on initial squares
	all_boxes_solved = all_boxes_in_place(b, 1);

	// it is ok to be stuck on initial position.
	if (b[global_initial_sokoban_y][global_initial_sokoban_x] & SOKOBAN)
		if (all_boxes_solved)
			return 1;

	if (search_mode == K_DIST_SEARCH)
	{
		if (size_of_sokoban_cloud(b) > 1)
			return 1;
		if (sokoban_touches_a_filled_hole(b))
			return 1; // the cloud size could be 1 because a nearby hole (probably garage) was wallified
		return 0;
	}

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if ((b[i][j] & BOX) == 0) continue;

			for (k = 0; k < 4; k++)
			{
				sok_y = i + delta_y[k];
				sok_x = j + delta_x[k];

				if ((b[sok_y][sok_x] & SOKOBAN) == 0)
					continue;

				next_sok_y = sok_y + delta_y[k];
				next_sok_x = sok_x + delta_x[k];

				if ((b[next_sok_y][next_sok_x] & OCCUPIED) == 0)
				{
					return 1;
				}
			}
		}

//	print_board(b);
//	my_getch();

	return 0;
}

int has_pull_moves(board b)
{
	int moves_num;
	int corral;
	move moves[MAX_MOVES];
	helper h;

	init_helper(&h);

	moves_num = find_possible_moves(b, moves, 1, &corral, NORMAL, &h);

	return (moves_num > 0);
}


void check_deadlock_in_moves(board b_in, move *moves, int moves_num, int *valid,
	int pull_mode, int search_mode, helper *h)
{
	int i,j, dest, dest_x,dest_y,res;
	board b,c;
	int elim[MAX_INNER];

	copy_board(b_in, b);

	if (search_mode == DEADLOCK_SEARCH)
		search_mode = NORMAL;
	// The problem is that the caller (find_possible_moves) removed all deadlock zone markings.
	// But... apply_move, which is used here, removes boxes out of the zone.


	if (pull_mode)
		clear_bases_inplace(b);

	for (i = 0; i < index_num; i++)
		elim[i] = 0;

	if ((pull_mode) && (search_mode == BASE_SEARCH))
	{
		// in reverse mode, mark all eliminating places.
		// if a box gets there, do not prune the move
		for (i = 0; i < height; i++)
			for (j = 0; j < width; j++)
			{
				if (b_in[i][j] & BASE)
					elim[y_x_to_index(i, j)] = 1;
			}
	}

	for (i = 0; i < moves_num; i++)
	{
		if (debug_deadlock)
		{
			printf("checking move: ");
			print_move(moves + i);
		}

		valid[i] = 1;


		if (moves[i].kill == 1)
		{
			if (debug_deadlock)
				printf("ok\n");
			continue;
		}

		dest = moves[i].to;
		index_to_y_x(dest, &dest_y, &dest_x);

		if ((pull_mode) && (elim[dest]))
			continue; // do not prune move into the elimination zone


		if (impossible_place[dest])
		{
			valid[i] = 0;
			if (debug_deadlock)
				printf("rejected: impossible place\n");
			continue;
		}

		copy_board(b, c);		
		apply_move(c, moves + i, search_mode);

		if (is_mpdb_deadlock(c, pull_mode))
		{
			valid[i] = 0;
			if (debug_deadlock)
				printf("rejected on mpdb\n");

			continue;
		}


		if (is_inefficient_position(c, moves[i].to, pull_mode, search_mode, h))
		{
			valid[i] = 0;
			if (debug_deadlock)
				printf("rejected on inefficient move\n");

			continue;
		}

		if (pull_mode) 
		{
			if (has_pull_continuation(c, search_mode) == 0)
			{
				valid[i] = 0;
				if (debug_deadlock)
					printf("no continuation\n");
				continue;
			}


			if (elim[moves[i].to])
			{
				exit_with_error("should not be here");
				continue; // do not prune eliminating moves
				// base squares can be on inefficient squares.
			}

			res = check_all_patterns(c, dest_y, dest_x, 1); // 1 - pull mode
			if (res != -1)
			{
				valid[i] = 0;
				if (debug_deadlock)
				{
					printf("rejected on pattern %d\n", res);
//					print_pattern(pull_patterns + res);
				}

				continue;
			}

			if (search_mode == BASE_SEARCH) 
			{// check that every base has at least one box
				res = verify_simple_base_matching(c, b_in); // b_in still has the bases
				if (res == 0)
				{
					if (debug_deadlock)
						printf("base matching\n");
					valid[i] = 0;
					continue;
				}
			}

			if (debug_deadlock) printf("\n");

			continue;
		} // if pull mode

		// so now we are in push mode

		if (is_freeze_deadlock(c, dest_y, dest_x))
		{
			valid[i] = 0;
			if (debug_deadlock)
				printf("freeze deadlock\n");
			continue;
		}

		res = is_3x3_deadlock(c, dest_y, dest_x);
		if (res)
		{
			valid[i] = 0;
			if (debug_deadlock)
				printf("rejected on 3X3\n");

			continue;
		}

		res = check_all_patterns(c, dest_y, dest_x , 0); // 0 - push mode
		if (res != -1)
		{
			valid[i] = 0;

			if (debug_deadlock)
			{
				printf("rejected on pattern %d\n", res);
//				print_pattern(patterns + res);
			}
			continue;
		}
	

		if (debug_deadlock)
			printf("ok\n");
	}


	if (h->perimeter_found) 
	{
		UINT_64 hash;
		UINT_16 depth;

		for (i = 0; i < moves_num; i++)
		{
			if (valid[i] == 1) continue;
			copy_board(b_in, c);
			apply_move(c, moves + i, NORMAL);

			hash = get_board_hash(c);
			if (is_in_perimeter(hash, &depth, 1 - pull_mode))
			{
				printf("MOVE REJECTED! despite in perimeter\n");
				print_board(b_in);
				print_board(c);
				my_getch();
			}
		}
	}

	if (debug_deadlock)
		printf("done debug deadlock\n");
}
