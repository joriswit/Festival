#include "board.h"
#include "moves.h"
#include "helper.h"

void init_rooms_deadlock();
int reduce_moves_in_rooms_deadlock_search(move *moves, int moves_num, helper *h);

int is_rooms_deadlock(board b_in, int pull_mode, move *m);
int is_in_room_with_corridor(int r, int y, int x);

