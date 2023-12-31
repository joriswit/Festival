#include "board.h"
#include "helper.h"
#include "score.h"


int imagine_score(board b, int search_mode, helper *h);
void init_imagine(park_order_data *full, int n, helper *h);
void get_imagined_board(board b, board imagined, int *relevant_board, helper *h);
int choose_imagine_advisor(score_element *base_score, int moves_num, score_element *scores, int search_mode);

void print_board_with_imagined(board b_in, helper *h);
void set_imagined_hf_board(board b, helper *h);