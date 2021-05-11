#ifndef _TIMER_H
#define _TIMER_H

#include "utils.h"
#include "http.h"

#define MAX_TIMERS 4096

// TODO - timer
typedef struct timer_node {
    struct timeval t;
    void* req;
	int deleted;
}timer_node;

typedef struct timer_queue{
    timer_node* p[MAX_TIMERS];
    int n;
}timer_queue;

void timer_queue_init(timer_queue* tq);

int timer_queue_add(timer_queue* tq, timer_node* tn);

int timer_queue_del_node(timer_node* tn);

int timer_queue_clean(timer_queue* tq);

timer_node* timer_queue_get_min(timer_queue* tq);

int timer_queue_del_min(timer_queue* tq);

int timer_queue_empty(timer_queue* tq);

int timeval_cmp_le(struct timeval* a, struct timeval* b);

double timeval_to_ms(struct timeval* tv);

#endif
