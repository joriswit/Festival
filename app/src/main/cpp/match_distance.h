#include "moves.h"
#include "score.h"
#include "helper.h"

void compute_match_distance(board b,
	score_element *base_score, int moves_num, move *moves, score_element *scores);

void compute_imagine_distance(board b,
	score_element *base_score, int moves_num, move *moves, score_element *scores, int search_type, helper *h);


void compute_base_distance(board b,
	score_element *base_score, int moves_num, move *moves, score_element *scores);

void compute_rev_distance(board b,
	score_element* base_score, int moves_num, move* moves, score_element* scores,
	board base_board, int from_to_mat[MAX_INNER][MAX_INNER]);
