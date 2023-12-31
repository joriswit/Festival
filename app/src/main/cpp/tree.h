#ifndef __TREE
#define __TREE

#include "board.h"
#include "moves.h"
#include "score.h"
#include "advisors.h"
#include "queue.h"
#include "helper.h"

#define MAX_TREE_LABELS 10000

typedef struct move_hash_data
{
	move move;
	UINT_64 hash;
	char deadlocked;
} move_hash_data;


typedef struct node_element
{
	UINT_64 hash;
	score_element score;
	int expansion;
	char deadlocked;
} node_element;

typedef struct expansion_data
{
	UINT_8 *b;
	node_element *node;
	expansion_data *father;
	int moves_num;
	int move_hash_place;
	int best_move;
	int best_weight;
	score_element best_score;
	score_element *best_past;
	int label;
	int has_corral;
	int weight;
	int depth;
	advisors_element advisors;

	int subtree_size;
	int next_son_to_check;

	int leaf_direction;
} expansion_data;



typedef struct tree
{
	// hash_array
	int *hash_array;
	int hash_size;
	int mask;

	// nodes
	node_element *nodes;
	int nodes_num;
	int max_nodes;

	// expansions
	expansion_data *expansions;
	int expansions_num;
	int max_expansions;

	// move-hash
	move_hash_data *move_hashes;
	int move_hashes_num;
	int max_move_hashes;

	// boards
	UINT_8 *boards;
	unsigned long long boards_size;

	// queues
	queue *queues;
	int labels_num;

	int pull_mode;
	int search_mode;
} tree;


void init_tree(tree *t, int log_max_nodes);
void free_tree(tree *t);
int tree_nearly_full(tree *t);
void set_root(tree *t, board b, helper *h);
void set_advisors_and_weights_to_last_node(tree *t, helper *h);
int best_move_in_tree(tree *t, int label, int *best_pos, int *best_son);
void expand_node(tree *t, expansion_data *e, int son, helper *h);
void set_node_as_deadlocked(tree *t, int pos, int son);
void set_best_move(tree *t, expansion_data *e);
void reset_tree(tree *t);
int find_node_by_hash(tree *t, UINT_64 hash);
move get_move_to_node(tree *t, expansion_data *e);
void remove_son_from_tree(tree *t, expansion_data *e, int son);
int node_is_solved(tree *t, expansion_data *e);
void get_score_of_hash(tree *t, UINT_64 hash, score_element *s);
void set_deadlock_status(tree *t, expansion_data *e);
expansion_data *last_expansion(tree *t);
move *get_node_move(tree *t, expansion_data *e, int son);
UINT_64 get_node_hash(tree *t, expansion_data *e, int son);
void add_label_to_last_node(tree *t, int label, helper *h);
void replace_in_queue(tree *t, expansion_data *e);
void insert_all_tree_nodes_to_queues(tree *t);
int board_is_in_tree(board b, tree *t);
void add_board_to_tree(tree *t, board b, helper *h);
int update_best_so_far(expansion_data **best_so_far, expansion_data *new_node, int pull_mode, int search_mode);
expansion_data *best_node_in_tree(tree *t);
int tree_has_unexpanded_nodes(tree *t);
void set_subtree_as_deadlocked(tree* t, expansion_data* e);

#endif