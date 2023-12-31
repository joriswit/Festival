#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "util.h"
#include "bfs.h"
#include "snail.h"

int snail_level_detected;
int snail_target_y, snail_target_x;

int netlock_level_detected;
board netlock_grid;

int_board snail_dist; // from snail target
board snail_chain;
int snail_chain_len;

void set_netlock_grid()
{
	int i, j;
	int sum = 0;

	zero_board(netlock_grid);
	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (inner[i][j])
				if (((i ^ snail_target_y) & 1) == 0)
					if (((j ^ snail_target_x) & 1) == 0)
						netlock_grid[i][j] = 1;

	if (verbose >= 5)
		show_on_initial_board(netlock_grid);
}

int simple_box_can_be_pulled(board b, int y, int x)
{
	// ignores player's position
	int i;

	for (i = 0; i < 4; i++)
		if ((b[y + delta_y[i]][x + delta_x[i]] & OCCUPIED) == 0)
			if ((b[y + 2 * delta_y[i]][x + 2 * delta_x[i]] & OCCUPIED) == 0)
				return 1;
	return 0;
}


int pullable_boxes(board b)
{
	int i, j, res = 0;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (b[i][j] & BOX)
				res += simple_box_can_be_pulled(b, i, j);
	return res;
}

void compute_pull_order(board b_in, int_board order)
{
	board b, visited;

	int current[MAX_INNER];
	int next[MAX_INNER];
	int current_num = 0;
	int next_num;

	int i, j, y, x, box_y, box_x, sok_y, sok_x, depth = 0;

	copy_board(b_in, b);
	zero_board(visited);

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			order[i][j] = -1;

	// initialize the first layer of pullable boxes
	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (b[i][j] & BOX)
				if (simple_box_can_be_pulled(b, i, j))
				{
					visited[i][j] = 1;
					current[current_num++] = y_x_to_index(i, j);
				}

	while (current_num)
	{
		// remove current layer
		for (i = 0; i < current_num; i++)
		{
			index_to_y_x(current[i], &y, &x);
			order[y][x] = depth;
			b[y][x] &= ~BOX;
		}

		next_num = 0;
		depth++;

		for (i = 0; i < current_num; i++)
		{
			index_to_y_x(current[i], &y, &x);

			for (j = 0; j < 4; j++)
			{
				box_y = y + delta_y[j];
				box_x = x + delta_x[j];
				sok_y = y - delta_y[j];
				sok_x = x - delta_x[j];

				if ((b[box_y][box_x] & BOX) == 0) continue;
				if (visited[box_y][box_x]) continue;
				if (b[sok_y][sok_x] & OCCUPIED) continue;

				visited[box_y][box_x] = 1;
				next[next_num++] = y_x_to_index(box_y, box_x);
			}

			for (j = 0; j < 4; j++)
			{
				if (b[y + delta_y[j]][x + delta_x[j]] & OCCUPIED) continue;

				box_y = y + delta_y[j] * 2;
				box_x = x + delta_x[j] * 2;

				if ((b[box_y][box_x] & BOX) == 0) continue;
				if (visited[box_y][box_x]) continue;

				visited[box_y][box_x] = 1;
				next[next_num++] = y_x_to_index(box_y, box_x);
			}
		}

		for (i = 0; i < next_num; i++)
			current[i] = next[i];
		current_num = next_num;
	}


	if (boxes_in_level(b) > 0)
	{
		// remaining boxes are crystalline-stuck
		return; // order will be -1
	}
}

void analyze_chain(board b)
{
	int i, j;
	int max_val = -1;
	int peel_from_y, peel_from_x;

	board not_targets;
	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			not_targets[i][j] = (b[i][j] & TARGET ? 0 : 1);


	// find the connected chain
	init_dist(snail_dist);
	bfs_from_place(not_targets, snail_dist, snail_target_y, snail_target_x, 1);

	// determine where the chain starts (which box must be pulled first)

	snail_chain_len = -1;
	zero_board(snail_chain);

	for (i = 0 ; i < height ; i++)
		for (j = 0; j < width; j++)
		{
			if (snail_dist[i][j] == 1000000) continue;

			snail_chain[i][j] = 1;
			if (snail_dist[i][j] > snail_chain_len)
			{
				snail_chain_len = snail_dist[i][j];
				peel_from_y = i;
				peel_from_x = j;
			}
		}

//	printf("%d %d %d\n", peel_from_y, peel_from_x, snail_chain_len);

	init_dist(snail_dist);
	bfs_from_place(not_targets, snail_dist, peel_from_y, peel_from_x, 1);

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (snail_dist[i][j] != 1000000)
				snail_dist[i][j] = snail_chain_len - snail_dist[i][j];


//	print_int_board(snail_dist, 1);
//	my_getch();


	snail_chain_len++;

	if (verbose >= 4)
	{
		print_int_board(snail_dist, 1);
		printf("snail chain len: %d\n", snail_chain_len);
	}
}


void detect_snail_level(board b_in)
{
	int i, j;
	board b;
	int sum = 0;
	int_board order;
	int max_difficulty = -1;

	snail_level_detected = 0;
	netlock_level_detected = 0;

	copy_board(b_in, b);
	enter_reverse_mode(b, 0);

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if ((initial_board[i][j] & BOX) == 0)
				b[i][j] &= ~BOX;


	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if ((b[i][j] & TARGET) == 0) continue;
			if ((initial_board[i][j] & BOX)) continue;


			b[i][j] |= BOX;
			compute_pull_order(b, order);
			b[i][j] &= ~BOX;

			if (order[i][j] > max_difficulty)
			{
				max_difficulty = order[i][j];
				snail_target_y = i;
				snail_target_x = j;
			}
		}

	set_netlock_grid();

	if (max_difficulty >= 20)
	{ 
		snail_level_detected = 1;

		if (verbose >= 4)
			printf("snail target= %d %d. len= %d\n", snail_target_y, snail_target_x, max_difficulty);
		analyze_chain(b);
		return;
	}

	if (max_difficulty >= 10)
	{
		if (verbose >= 4)
			printf("netlock target= %d %d. len= %d\n", snail_target_y, snail_target_x, max_difficulty);

		netlock_level_detected = 1;
	}
}




int touched_boxes(board b)
{
	int sum = 0;

	int i, j, k;

	for (i= 0; i < height ; i++)
		for (j = 0; j < width; j++)
		{
			if ((b[i][j] & BOX) == 0) continue;

			for (k = 0; k < 4; k++)
				if (b[i + delta_y[k]][j+delta_x[k]] & SOKOBAN) break;
			if (k < 4)
				sum++;
		}
	return sum;
}

int count_101(board b)
{
	int i, j;
	int sum = 0;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (b[i][j] & BOX)
			{
				if ((b[i][j + 1] & OCCUPIED) == 0)
					if (b[i][j + 2] & BOX)
						sum++;

				if ((b[i + 1][j] & OCCUPIED) == 0)
					if (b[i + 2][j] & BOX)
						sum++;
			}

	return sum;
}



int unlikely_pattern(board b)
{
	int i, j, k;
	int res = 0, sum = 0;

	for (i = 0 ; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if ((b[i][j] & BOX) == 0) continue;

			sum = 0;
			for (k = 0; k < 8; k++)
				if (b[i + delta_y_8[k]][j + delta_x_8[k]] & OCCUPIED)
					sum++;
			if (sum >= 6) res++;
		}
	return res;
}


int snail_packed(board b)
{
	int i, j;
	int seq[1000];

	for (i = 0; i < snail_chain_len; i++)
		seq[i] = 1;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (snail_chain[i][j])
				if ((b[i][j] & BOX) == 0)
					seq[snail_dist[i][j]] = 0;


	for (i = 1; i < snail_chain_len; i++)
		if (seq[i] == 0)
			return i;

	return i;
}

int netlock_score(board b);
int boxes_on_grid(board b);



void fill_snail_scores(board b, score_element* s, int alg_type)
{
	if (b[snail_target_y][snail_target_x] & BOX)
		s->out_of_plan = 1; // not solved yet
	else
		s->out_of_plan = 0;

	if (alg_type == 0) 
		s->dist_to_imagined = unlikely_pattern(b);

	s->biconnect = global_boxes_in_level - touched_boxes(b);

	if (s->out_of_plan == 1) // did not solve yet
	{
		if (alg_type == 0)
			s->hotspots = count_101(b);
	}
	else
	{
		s->boxes_on_targets = snail_packed(b);
	}
}



int is_better_snail_score(score_element* new_score, score_element* old_score, int pull_mode)
{
	if (new_score->out_of_plan < old_score->out_of_plan) return 1;
	if (new_score->out_of_plan > old_score->out_of_plan) return 0;

	if (new_score->biconnect < old_score->biconnect) return 1; // touched boxes
	if (new_score->biconnect > old_score->biconnect) return 0;

	if (new_score->dist_to_imagined < old_score->dist_to_imagined) return 1; // unlikely pattern
	if (new_score->dist_to_imagined > old_score->dist_to_imagined) return 0;

	if (new_score->out_of_plan == 0) // solved the chain
	{
		if (new_score->boxes_on_targets > old_score->boxes_on_targets) return 1; // packed boxes
		if (new_score->boxes_on_targets < old_score->boxes_on_targets) return 0;
	}
	else
	{
		if (new_score->hotspots > old_score->hotspots) return 1; // 101 pattern
		if (new_score->hotspots < old_score->hotspots) return 0;
	}

	if (new_score->dist_to_targets > old_score->dist_to_targets) return 1;
	if (new_score->dist_to_targets < old_score->dist_to_targets) return 0;

	return 0;
}

void print_score_in_snail_mode(score_element* s, int pull_mode)
{
	printf("cnct= %d pat= %d dist= %d solved= %d tgts= %d img=%d netlock=%d\n",
		s->biconnect, 
		s->hotspots, s->dist_to_targets, 1 - s->out_of_plan, s->boxes_on_targets,
		s->dist_to_imagined, s->rooms_score);
}


int set_snail_parameters(int search_type, int pull_mode, helper* h)
{
	if (snail_level_detected == 0) return search_type;

	if (pull_mode == 0)
	{
		if ((search_type == FORWARD_WITH_BASES) || 
			(search_type == HF_SEARCH) ||
			(search_type == GIRL_SEARCH))
			return HF_SEARCH;
		return search_type;
	}

	if (search_type == BASE_SEARCH)
	{
		if (verbose >= 4) printf("snail type A\n");
		h->alg_type = 0;
		h->weighted_search = 0;
		return SNAIL_SEARCH;
	}
	if (search_type == MAX_DIST_SEARCH2)
	{
		if (verbose >= 4) printf("snail type B\n");
		h->alg_type = 1;
		h->weighted_search = 0;
		return SNAIL_SEARCH;
	}
	if (search_type == GIRL_SEARCH)
	{
		if (verbose >= 4) printf("snail type C\n");
		h->alg_type = 3;
		h->weighted_search = 1;
		return NETLOCK_SEARCH;
	}

	return search_type;
}


// netlock code starts here





int netlock_score(board b)
{
	board c;
	int res;
	int_board order;

	if ((b[snail_target_y][snail_target_x] & BOX) == 0)
		return 0;
			
	copy_board(b, c);

	compute_pull_order(c, order);

	res = order[snail_target_y][snail_target_x];

	if (b[snail_target_y][snail_target_x] & BOX)
		if (res == -1)
			return 1000;

	return res;
}

int boxes_on_grid(board b)
{
	int i, j;
	int sum = 0;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (netlock_grid[i][j])
				if (b[i][j] & BOX)
					sum++;

	return sum;
}


void fill_netlock_scores(board b, score_element* s, int alg_type)
{
	s->connectivity = get_connectivity(b);
	s->boxes_on_targets = netlock_score(b);
	s->rooms_score = boxes_on_grid(b);

	if (s->boxes_on_targets == 0) // solved level
	{
		s->imagine = pullable_boxes(b);

		// an alternative for type 0
		//s->rooms_score = boxes_on_targets(b);
		//s->imagine = count_101(b);
	}
	else
	{
		if (alg_type == 0)
		{
			s->rooms_score = count_101(b);
			s->imagine = boxes_on_grid(b);
		}

		if (alg_type == 2)
			s->imagine = pullable_boxes(b);
	}

	if (alg_type == 3)
	{
		s->rooms_score = boxes_on_targets(b);
		s->imagine = count_101(b);
	}
}

int is_better_netlock_score(score_element* new_score, score_element* old_score)
{

	if (new_score->connectivity < old_score->connectivity) return 1;
	if (new_score->connectivity > old_score->connectivity) return 0;

	if (new_score->boxes_on_targets < old_score->boxes_on_targets) return 1;
	if (new_score->boxes_on_targets > old_score->boxes_on_targets) return 0;

	if (new_score->rooms_score > old_score->rooms_score) return 1; // pattern
	if (new_score->rooms_score < old_score->rooms_score) return 0;

	if (new_score->imagine > old_score->imagine) return 1; // space
	if (new_score->imagine < old_score->imagine) return 0;

	return 0;
}

void print_score_in_netlock_mode(score_element* s)
{
	printf("con= %d netlock= %d pullable= %d pat= %d\n", 
		s->connectivity, s->boxes_on_targets,
		s->imagine, s->rooms_score);
}

int set_netlock_parameters(int search_type, int pull_mode, helper* h)
{
	if (netlock_level_detected == 0) return search_type;

	if (pull_mode == 0)
	{
		if ((search_type == FORWARD_WITH_BASES) || 
			(search_type == HF_SEARCH) || 
			(search_type == GIRL_SEARCH)) 
			return HF_SEARCH;
		return search_type;
	}

	if (search_type == BASE_SEARCH)
	{
		if (verbose >= 4) printf("netlock type A\n");
		h->alg_type = 0;
		return NETLOCK_SEARCH;

	}
	if (search_type == MAX_DIST_SEARCH2)
	{
		if (verbose >= 4) printf("netlock type B\n");
		h->alg_type = 1;
		return NETLOCK_SEARCH;
	}
	if (search_type == GIRL_SEARCH)
	{
		if (verbose >= 4) printf("netlock type C\n");
		h->alg_type = 2;
		return NETLOCK_SEARCH;
	}

	return search_type;
}

