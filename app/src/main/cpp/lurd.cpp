#include <stdio.h>

#include "board.h"
#include "moves.h"
#include "util.h"
#include "score.h"

#include "match_distance.h"


void lurd_to_moves(char* lurd, move* moves)
{
	int player_y, player_x;
	int dir, push;
	int next_y, next_x, next_y2, next_x2;
	board b;
	score_element base_score;
	score_element *scores = 0;

	copy_board(initial_board, b);

	get_sokoban_position(b, &player_y, &player_x);

	while (lurd)
	{
		if (*lurd == 'u') { dir = 0; push = 0; }
		if (*lurd == 'r') { dir = 1; push = 0; }
		if (*lurd == 'd') { dir = 2; push = 0; }
		if (*lurd == 'l') { dir = 3; push = 0; }
		if (*lurd == 'U') { dir = 0; push = 1; }
		if (*lurd == 'R') { dir = 1; push = 1; }
		if (*lurd == 'D') { dir = 2; push = 1; }
		if (*lurd == 'L') { dir = 3; push = 1; }

		lurd++;

		next_y = player_y + delta_y[dir];
		next_x = player_x + delta_x[dir];

		next_y2 = next_y + delta_y[dir];
		next_x2 = next_x + delta_x[dir];

		if (push)
		{
			if ((b[next_y][next_x] & BOX) == 0) exit_with_error("bad push1");
			if (b[next_y2][next_x2] & OCCUPIED) exit_with_error("bad push2");

			b[next_y][next_x] &= ~BOX;
			b[next_y2][next_x2] |= BOX;
		}
		else
		{
			if (b[next_y][next_x] & OCCUPIED) exit_with_error("bad move");
		}
		b[player_y][player_x] &= ~SOKOBAN;
		b[next_y][next_x] |= SOKOBAN;

		player_y = next_y;
		player_x = next_x;

		print_board(b);

		compute_match_distance(b, &base_score, 0, moves, scores);
		printf("dist= %d\n", base_score.dist_to_targets);

		my_getch();

	}
}



void lurd_tests()
{
	char lurd[5000];
	FILE* fp;
	move moves[500];


	fp = fopen("c:\\sokoban\\lurd.txt", "r");
	fgets(lurd, 5000, fp);
	fclose(fp);

	remove_newline(lurd);

	lurd_to_moves(lurd, moves);
}
