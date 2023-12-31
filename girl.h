#include "tree.h"
#include "helper.h"

void print_score_in_girl_mode(score_element *s, int pull_mode);
expansion_data* find_best_RL_parking_node(tree *t);
void fill_girl_scores(board b, score_element *s, int pull_mode, helper *h);
void handle_new_pos_in_girl_mode_tree(tree *t, expansion_data *e);
void init_girl_variables(board b);
double score_to_value(score_element *s, int pull_mode);
