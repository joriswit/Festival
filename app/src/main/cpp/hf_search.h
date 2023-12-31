#include "score.h"
#include "tree.h"

int is_better_hf_score(score_element* new_score, score_element* old_score, int pull_mode);

int is_better_rev_score(score_element* new_score, score_element* old_score, int pull_mode);

void set_rev_advisor(board b, score_element* base_score, int moves_num, move* moves,
	score_element* scores, int pull_mode,
	int search_mode, int* already_expanded, advisors_element* l);

void setup_rev_board(expansion_data* e, helper* h);

void set_rev_distance(tree* t, expansion_data* e, helper* h,
	move *moves, score_element *scores);


void set_score_in_rev_mode(board b, score_element* s, int pull_mode, helper *h);

int compare_to_imagined_hf(board b, helper* h);

int get_distance_in_hf_search(board b, score_element* s, helper* h);

int hf_hotspots(board b, int pull_mode, helper* h);

int can_be_accessed_from_some_direction(board b, int y, int x);

void fill_hf_scores(board b, score_element* s, int pull_mode, int search_mode, helper* h);
