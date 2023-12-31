#include "board.h"
#include "score.h"
#include "helper.h"

extern int snail_level_detected;
extern int netlock_level_detected;

void fill_snail_scores(board b, score_element* s, int alg_type);
int is_better_snail_score(score_element* new_score, score_element* old_score, int pull_mode);
void print_score_in_snail_mode(score_element* s, int pull_mode);

void detect_snail_level(board b);
int set_snail_parameters(int search_type, int pull_mode, helper* h);

void fill_netlock_scores(board b, score_element* s, int alg_type);
int is_better_netlock_score(score_element* new_score, score_element* old_score);
void print_score_in_netlock_mode(score_element* s);

int set_netlock_parameters(int search_type, int pull_mode, helper* h);
