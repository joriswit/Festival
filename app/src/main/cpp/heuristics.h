#include "board.h"
#include "score.h"

void compute_possible_from_bases(board b, board possible_from_bases);

int mark_sink_squares_moves(board b, move *moves, int moves_num);

int packing_search_advisor(board b, score_element *base_score,
	int moves_num, move *moves, score_element *scores, int search_mode);