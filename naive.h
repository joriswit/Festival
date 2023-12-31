#include "tree.h"
#include "helper.h"

void set_naive_data_in_node(tree *t, expansion_data *e, helper *h);
void fill_naive_scores(board b, score_element *s, int pull_mode);
int is_better_naive_score(score_element *new_score, score_element *old_score, int pull_mode);
