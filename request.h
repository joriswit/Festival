// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#ifndef __REQUEST
#define __REQUEST

#include "score.h"
#include "helper.h"

void init_pack_requets(helper *h);
void add_pack_request(score_element *s, helper *h);
void get_next_pack_request(pack_request *p, helper *h);

void init_requests(helper *h);
void add_request(score_element *s, helper *h);
void get_next_request(request *p, helper *h);

void set_next_request_to_score(score_element *s, helper *h);
void set_next_pack_request_to_score(score_element *s, helper *h);

int get_label_of_score(score_element *s, int pull_mode, helper *h);

int get_label_of_request(request *p, helper *h);
int get_label_of_pack_request(pack_request *p, helper *h);

void get_request_of_score(request *p, score_element *s, helper *h);
void get_pack_request_of_score(pack_request *p, score_element *s, helper *h);


#endif