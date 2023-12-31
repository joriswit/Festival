#include "board.h"
#include "moves.h"
#include "positions.h"

int check_for_k_dist_moves(board b, move *moves, int moves_num, helper *h);

int is_k_dist_deadlock(board b); 

void clear_k_dist_hash();

int sokoban_touches_a_filled_hole(board b);

void clear_k_dist_boxes(board b, int pull_mode, int k_dist_val);
int actual_k_dist_search(board b, int pull_mode, int k_dist_val, int search_size);

int is_better_k_dist_score(score_element* new_score, score_element* old_score);
