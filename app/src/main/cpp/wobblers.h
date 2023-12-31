#include "board.h"
#include "score.h"
#include "helper.h"

int is_wobblers_deadlock(board b, int pull_mode);
void fill_wobblers_scores(board b, score_element *s, int pull_mode);
int reduce_moves_in_wobblers_search(move *moves, int moves_num);
int wobblers_search_actual(board b, int pull_mode, int max_positions);
void remove_boxes_with_no_player_access(board b, int pull_mode, int with_diag);
void remove_wobblers(board b, int pull_mode);
