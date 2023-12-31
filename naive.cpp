#include "naive.h"
#include "util.h"
#include "match_distance.h"
#include "bfs.h"

void set_naive_data_in_node(tree *t, expansion_data *e, helper *h)
{
	move moves[MAX_MOVES];
	score_element scores[MAX_MOVES];
	move_hash_data *mh;
	int i;
	int place;
	board b;

	mh = t->move_hashes + e->move_hash_place;
	for (i = 0; i < e->moves_num; i++)
	{
		moves[i] = mh[i].move;
		get_score_of_hash(t, mh[i].hash, scores + i);
	}
	bytes_to_board(e->b, b);

	if (t->pull_mode) 
		compute_base_distance(b, &(e->node->score), e->moves_num, moves, scores);
	else
		compute_match_distance(b, &(e->node->score), e->moves_num, moves, scores);

	for (i = 0; i < e->moves_num; i++)
	{
		place = find_node_by_hash(t, mh[i].hash);
		if (place == -1) exit_with_error("missing node");
		t->nodes[place].score.dist_to_targets = scores[i].dist_to_targets;
	}
}

void fill_naive_scores(board b, score_element *s, int pull_mode)
{
	//s->biconnect = (size_of_sokoban_cloud(b) <= 8 ? 1 : 0);
	// statistically this is 50% deadlock, so we add a small penalty

	s->packed_boxes = boxes_on_bases(b);

	return;
}

int is_better_naive_score(score_element *new_score, score_element *old_score, int pull_mode)
{
	int new_val, old_val;
	// distance to bases, + a small penalty if stuck in a small corral

//	new_val = new_score->dist_to_targets + new_score->biconnect;
//	old_val = old_score->dist_to_targets + old_score->biconnect;

	new_val = new_score->dist_to_targets;
	old_val = old_score->dist_to_targets;


	if (new_val < old_val) return 1;
	if (new_val > old_val) return 0;

	if (new_score->packed_boxes > old_score->packed_boxes) return 1;
	if (new_score->packed_boxes < old_score->packed_boxes) return 0;


//	if (new_score->biconnect < old_score->biconnect) return 1;
//	if (new_score->biconnect > old_score->biconnect) return 0;


	return 0;
}