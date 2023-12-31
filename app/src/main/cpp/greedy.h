#include "tree.h"
#include "helper.h"

void choose_pos_son_to_expand_Epsilon_greedy_tree(tree *t, int *pos, int *son, helper *h);
void back_propagate_values_tree(tree *t, expansion_data *e);
void update_subtree_size_tree(tree *t, expansion_data *e);
void set_inv_epsilon();
