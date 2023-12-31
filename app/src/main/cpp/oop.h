#include "board.h"
#include "helper.h"
#include "score.h"

void prepare_oop_zones(helper *h);
int get_out_of_plan_score(board b, int parked_num, helper *h);

int new_oop_advisor(board b, score_element *base_score, int moves_num, 
	move *moves, score_element *scores, int pull_mode, helper *h);