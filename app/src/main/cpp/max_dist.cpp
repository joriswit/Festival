#include "max_dist.h"
#include "match_distance.h"
#include "util.h"

void set_max_dist_data_in_node(tree* t, expansion_data* e, helper* h)
{
	move moves[MAX_MOVES];
	score_element scores[MAX_MOVES];
	move_hash_data* mh;
	int i;
	int place;
	board b;

	if (t->pull_mode == 0) return; // for this the imagined distance is relevant

	mh = t->move_hashes + e->move_hash_place;
	for (i = 0; i < e->moves_num; i++)
	{
		moves[i] = mh[i].move;
		get_score_of_hash(t, mh[i].hash, scores + i);
	}
	bytes_to_board(e->b, b);

	compute_match_distance(b, &(e->node->score), e->moves_num, moves, scores);

	for (i = 0; i < e->moves_num; i++)
	{
		place = find_node_by_hash(t, mh[i].hash);
		if (place == -1) exit_with_error("missing node");
		t->nodes[place].score.dist_to_targets = scores[i].dist_to_targets;
	}
}