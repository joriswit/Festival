#include "board.h"
#include "tree.h"

int is_stuck_deadlock(board b, int pull_mode);
void init_stuck_patterns();
void learn_from_subtrees(tree* t, expansion_data* e);
void register_mini_corral(board b);

int test_best_for_patterns(tree* t, expansion_data* e);
void recheck_tree_with_patterns(tree* t);


