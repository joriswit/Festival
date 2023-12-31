#include "board.h"
#include "moves.h"

int corral_deadlock(board b_in, int pull_mode);
//int learn_pull_deadlock(board b_in);

int check_for_deadlock_zone_moves(board b, move* moves, int moves_num);
