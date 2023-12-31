#include <stdlib.h>

#include "helper.h"
#include "util.h"

void reset_helper(helper *h)
{
	h->parking_order_num = 0;
	h->full_parking_order_num = 0;
	h->sol_len = 0;
	h->perimeter_found = 0;
	h->enable_inefficient_moves = 0;
	h->level_solved = 0;
	h->weighted_search = 1;
	zero_board(h->used_in_plan);
}

void init_helper(helper *h)
{
	// the large data structures
	h->oop = 0;
	h->imagine = 0;
	h->request = 0;

	// smaller data structures
	reset_helper(h);
}


void init_helper_extra_fields(helper *h)
{
	h->oop = (oop_data*)malloc(sizeof(oop_data));
	if (h->oop == 0) exit_with_error("can't alloc oop");
	h->imagine = (imagine_data*)malloc(sizeof(imagine_data));
	if (h->imagine == 0) exit_with_error("can't alloc imagine");
	h->request = (request_data*)malloc(sizeof(request_data));
	if (h->request == 0) exit_with_error("can't alloc request");

}

void free_helper(helper *h)
{
	free(h->oop);
	free(h->imagine);
	free(h->request);
}
