#ifndef __DRAGONFLY
#define __DRAGONFLY

#include "helper.h"

typedef struct dragonfly_node
{
	unsigned char board;
	unsigned short depth;
	int father;
	unsigned short move_from;
	unsigned short move_to;
	unsigned char player_position;
	int dist;
	unsigned short packed;
} dragonfly_node;

void init_dragonfly();
void dragonfly_search(board b_in, int time_allocation, helper* h);
void dragonfly_print_node(dragonfly_node* e);

#endif