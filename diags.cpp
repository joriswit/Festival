// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include "diags.h"

// a diagonal is a pettern of the form:
//
//  OO
//  O O
//   O O
//    O O
//     OO
//
// Where O is occupied, and the inner is clear. Note that any push results in a smaller diagonal.
// The next routine returns the length of the diagonal that starts in each place
// Diagnoal can go the the right (as in the example above) or to the left.
// The length is of the inner part (3 in the example).
//
// In addition, diags can start/end with two walls, for example:
//
//  #
// # O
//  O O
//   O O
//

void mark_diags(board b, board diag_right, board diag_left)
{
	board inner;
	int i, j, k;
	int diag_start,diag_end;

	zero_board(inner);
	zero_board(diag_right);
	zero_board(diag_left);

	// mark all empty squares surrounded by four elements
	for (i = 1; i < (height - 1); i++)
		for (j = 1; j < (width - 1); j++)
		{
			if (b[i][j] & OCCUPIED) continue;
			if (b[i][j] & SOKOBAN) continue; // this can happen in the initial position

			if ((b[i - 1][j] & OCCUPIED) == 0) continue;
			if ((b[i + 1][j] & OCCUPIED) == 0) continue;
			if ((b[i][j + 1] & OCCUPIED) == 0) continue;
			if ((b[i][j - 1] & OCCUPIED) == 0) continue;
			inner[i][j] = 1;
		}

	// look for pattern:

	// O
	//  D 
	//   D
	//    D
	//     O

	for (i = 1; i < (height - 1); i++)
		for (j = 1; j < (width - 1); j++)
		{
			if (inner[i][j] == 0) continue;

			diag_start = 0;
			if (b[i - 1][j - 1] & OCCUPIED) diag_start = 1;
			if ((b[i - 1][j] == WALL) && (b[i][j - 1] == WALL)) diag_start = 1;
			if (diag_start == 0) continue;

			k = 1;
			while (inner[i + k][j + k])
				k++;

			diag_end = 0;
			if (b[i + k][j + k] & OCCUPIED) diag_end = 1;
			if ((b[i + k - 1][j + k] == WALL) && (b[i + k][j + k - 1] == WALL)) diag_end = 1;

			if (diag_end == 0) continue;

			diag_right[i][j] = k;
		}

	// same for diag_left:
	//      O
	//     D 
	//    D
	//   D
	//  O

	for (i = 1; i < (height - 1); i++)
		for (j = 1; j < (width - 1); j++)
		{
			if (inner[i][j] == 0) continue;

			diag_start = 0;
			if (b[i - 1][j + 1] & OCCUPIED) diag_start = 1;
			if ((b[i - 1][j] == WALL) && (b[i][j + 1] == WALL)) diag_start = 1;
			if (diag_start == 0) continue;

			k = 1;
			while (inner[i + k][j - k])
				k++;

			diag_end = 0;
			if (b[i + k][j - k] & OCCUPIED) diag_end = 1;
			if ((b[i + k - 1][j - k] == WALL) && (b[i + k][j - k + 1] == WALL)) diag_end = 1;
			if (diag_end == 0) continue;

			diag_left[i][j] = k;
		}
}


void wallify_diag_void(board b, int i, int j, int k, int diag_right)
{
	int n;

	if (diag_right)
	{
		for (n = 0; n < k; n++)
		{
			b[i + n][j + n - 1] = WALL;
			b[i + n][j + n + 1] = WALL;
		}

		if (b[i - 1][j-1] & OCCUPIED)
			b[i - 1][j - 1] = WALL;
		b[i - 1][j] = WALL;

		if (b[i + k][j+k] & OCCUPIED)
			b[i + k][j + k] = WALL;
		b[i + k][j + k - 1] = WALL;
	}
	else
	{
		for (n = 0; n < k; n++)
		{
			b[i + n][j - n - 1] = WALL;
			b[i + n][j - n + 1] = WALL;
		}

		if (b[i - 1][j+1] & OCCUPIED)
			b[i - 1][j + 1] = WALL;
		b[i - 1][j] = WALL;

		if (b[i + k][j-k] & OCCUPIED)
			b[i + k][j - k] = WALL;
		b[i + k][j - k + 1] = WALL;
	}
}

int wallify_diag(board b, int i, int j, int k, int diag_right)
{
	board backup;
	copy_board(b, backup);

	wallify_diag_void(b, i, j, k, diag_right);

	return is_same_board(b, backup) ^ 1;
}

int diag_has_no_targets(board b, int i, int j, int n, int diag_right)
{
	int delta, k;
	delta = (diag_right ? 1 : -1);

	for (k = 0; k < n; k++)
	{
		if (b[i + k][j + delta*k] & TARGET)
			return 0;
	}
	return 1;
}



int diag_deadlock(board b, int start_y, int start_x, int n, int delta)
{
	int i;
	int current_y, current_x;
	unsigned char element1, element2;
	int good = 1;

// This routine works as follows. It scans the diagonal upside down, each time looking at two elements:
//
//  OO
//  O Y
//   YXZ
//    Z O
//     OO
//
// For example, when it gets to place X it looks at the two Y's. Pushing a Y into X opens the upper part
// but now the deadlock question should answered by the lower part.
// In addition, if up until now everything was good (all boxes on targets) and X has a target, 
// then pushing Z into X opens the lower part while maintaining the upper one.
//
// This routine works because of the following observation:
// If the situation is not good (either by Y or by previous boxes) then eventually the upper diagonal 
// has to be opened to fix this. 
// If opening the upper diagnal is done by pushing Y: that's what we're trying rigth now.
// If the upper diagonal is opened from below, then the box in X can be pushed back to Y, and
// there's no harm in doing this move now.
// So like in PI-Corral pruning, if pushing Y to X should be done, it can be done right away.

	if (b[start_y - 1][start_x - delta] == BOX) // not on target
		good = 0;

	for (i = 0; i < (n+1); i++)
	{
		current_y = start_y + i;
		current_x = start_x + i * delta;

		element1 = b[current_y - 1][current_x];
		element2 = b[current_y][current_x - delta];

		if ((element1 == BOX) || (element2 == BOX) || (good == 0)) // box not on target
		{
			if (i == n) return 1; // can't open the upper diagonal;

			good = (b[current_y][current_x] & TARGET ? 1 : 0);
			continue; // open the upper diagonal and continue the search on the lower diagonal
		}

		if (i == n)
			break;

		if (b[current_y][current_x] & TARGET)
			return 0;
		// push one of the next boxes, opening the lower diag (upper diag is already good)
	}

	if (b[current_y][current_x] == BOX) return 1;

	return 0;
}



int is_diag_deadlock(board b)
{
	board diag_right, diag_left;
	int i, j, n;

	mark_diags(b, diag_right, diag_left);

	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			n = diag_right[i][j];
			if (n != 0)
			{
				if (diag_deadlock(b, i, j, n, 1)) return 1;
			}

			n = diag_left[i][j];
			if (n != 0)
			{
				if (diag_deadlock(b, i, j, n, -1)) return 1;
			}
		}
	}
	return 0;
}

