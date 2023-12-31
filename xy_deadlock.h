#include "moves.h"
#include "score.h"

int reduce_moves_in_xy_search(board b, move *moves, int moves_num);
int is_xy_deadlock(board b_in, int pull_mode);
int xy_deadlock_search(board b, int pull_mode);
void mark_hv_indices(board b, move *moves, int moves_num, int *indices);
void clear_xy_boxes(board b, int pull_mode);
