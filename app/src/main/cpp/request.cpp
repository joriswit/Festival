// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include <stdio.h>

#include "request.h"
#include "util.h"



void init_pack_requets(helper *h)
{
	h->request->pack_requests_num = 0;
	h->request->pack_requests_pos = 0;
}

void init_requests(helper *h)
{
	h->request->requests_num = 0;
	h->request->requests_pos = 0;
}

int pack_request_matches_score(pack_request *req, score_element *s)
{
	if (req->boxes_in_level    == s->boxes_in_level)
	if (req->boxes_on_targets  == s->boxes_on_targets)
	if (req->connectivity      == s->connectivity)
	if (req->room_connectivity == s->rooms_score)
		return 1;
	return 0;
}

int request_matches_score(request *req, score_element *s)
{
	if (req->packed_boxes != s->packed_boxes) return 0;
	if (req->connectivity != s->connectivity) return 0;
	if (req->out_of_plan  != s->out_of_plan)  return 0;
	if (req->imagine      != s->imagine)      return 0;

	return 1;
}

int find_score_in_pack_requests(pack_request *req, int n, score_element *s)
{
	int i;
	for (i = 0; i < n; i++)
		if (pack_request_matches_score(req + i, s))
			return i;
	return -1;
}

int find_score_in_requests(request *req, int n, score_element *s)
{
	int i;
	for (i = 0; i < n; i++)
		if (request_matches_score(req + i, s))
			return i;
	return -1;
}



void set_pack_request_from_score(pack_request *p, score_element *s)
{
	p->boxes_in_level    = s->boxes_in_level;
	p->boxes_on_targets  = s->boxes_on_targets;
	p->connectivity      = s->connectivity;
	p->room_connectivity = s->rooms_score;
}

void set_request_from_score(request *p, score_element *s)
{
	p->packed_boxes = s->packed_boxes;
	p->connectivity = s->connectivity;
	p->out_of_plan  = s->out_of_plan;
	p->imagine      = s->imagine;
}

void add_pack_request(score_element *s, helper *h)
{
	int i;

	i = find_score_in_pack_requests(h->request->pack_requests, h->request->pack_requests_num, s);
	if (i != -1) return;

	if (verbose >= 5)
	printf("Adding pack cell: lvl: %2d tgts: %2d connectivity: %2d room: %2d\n",
		s->boxes_in_level, s->boxes_on_targets, s->connectivity, s->rooms_score);

	set_pack_request_from_score(h->request->pack_requests + h->request->pack_requests_num, s);

	h->request->pack_requests_pos = h->request->pack_requests_num;
	h->request->pack_requests_num++;

	if (h->request->pack_requests_num >= MAX_REQUESTS)
		exit_with_error("max requests");

}

void add_request(score_element *s, helper *h)
{
	int i;

	i = find_score_in_requests(h->request->requests, h->request->requests_num, s);
	if (i != -1) return;

	if (verbose >= 5)
	printf("Adding cell: imagine: %2d connect: %2d OOP: %2d packed: %2d\n",
		s->imagine,
		s->connectivity,
		s->out_of_plan,
		s->packed_boxes);

	set_request_from_score(h->request->requests + h->request->requests_num, s);

	h->request->requests_pos = h->request->requests_num;
	h->request->requests_num++;

	if (h->request->requests_num >= MAX_REQUESTS)
		exit_with_error("max requests");
}

void get_next_pack_request(pack_request *p, helper *h)
{
	*p = h->request->pack_requests[h->request->pack_requests_pos];

	h->request->pack_requests_pos++;
	if (h->request->pack_requests_pos == h->request->pack_requests_num)
		h->request->pack_requests_pos = 0;
}

void get_next_request(request *p, helper *h)
{
	*p = h->request->requests[h->request->requests_pos];

	h->request->requests_pos++;
	if (h->request->requests_pos == h->request->requests_num)
		h->request->requests_pos = 0;
}

void get_pack_request_of_score(pack_request *p, score_element *s, helper *h)
{
	int i;
	i = find_score_in_pack_requests(h->request->pack_requests, h->request->pack_requests_num, s);
	if (i == -1) exit_with_error("can't find pack request");
	*p = h->request->pack_requests[i];
}

void get_request_of_score(request *p, score_element *s, helper *h)
{
	int i;
	i = find_score_in_requests(h->request->requests, h->request->requests_num, s);
	if (i == -1) exit_with_error("can't find request");
	*p = h->request->requests[i];
}

void set_next_pack_request_to_score(score_element *s, helper *h)
{
	int i;

	i = find_score_in_pack_requests(h->request->pack_requests, h->request->pack_requests_num, s);
	if (i == -1) exit_with_error("can't find pack request");

	h->request->pack_requests_pos = i;	
}

void set_next_request_to_score(score_element *s, helper *h)
{
	int i;
	i = find_score_in_requests(h->request->requests, h->request->requests_num, s);
	if (i == -1) exit_with_error("can't find request");

	h->request->requests_pos = i;
}



void print_request(request *req)
{
	printf("OOP: %2d pack: %2d con: %2d imagine: %2d\n",
		req->out_of_plan,
		req->packed_boxes,
		req->connectivity,
		req->imagine);
}

int get_label_of_score(score_element *s, int pull_mode, helper *h)
{
	if (pull_mode == 0)
		return find_score_in_requests(h->request->requests, h->request->requests_num, s);

	return find_score_in_pack_requests(h->request->pack_requests, h->request->pack_requests_num, s);
}

int get_label_of_request(request *p, helper *h)
{
	score_element s;
	s.packed_boxes  = p->packed_boxes;
	s.connectivity  = p->connectivity;
	s.out_of_plan   = p->out_of_plan;
	s.imagine       = p->imagine;

	return get_label_of_score(&s, 0, h);
}

int get_label_of_pack_request(pack_request *p, helper *h)
{
	score_element s;
	s.boxes_in_level   = p->boxes_in_level;
	s.boxes_on_targets = p->boxes_on_targets;
	s.connectivity     = p->connectivity;
	s.rooms_score      = p->room_connectivity;

	return get_label_of_score(&s, 1, h);
}
