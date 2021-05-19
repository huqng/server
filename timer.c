#include "timer.h"

static pthread_mutex_t tq_mutex;

void timer_queue_init(timer_queue* tq) {    
	/* init mutex*/
    if(pthread_mutex_init(&tq_mutex, NULL) != 0){
        perror("pthread mutex init");
        exit(-1);
    }
    tq->n = 0;
}

int timer_queue_add(timer_queue* tq, timer_node* tn) {
	int ret = 0;
	pthread_mutex_lock(&tq_mutex);
	if(tq->n >= MAX_TIMERS)
		ret = -1;
	else {	
		tq->p[tq->n] = tn;
		int i = tq->n++;
		while(i >= 0) {
			int j = (i - 1) / 2;
			if(j < i && timeval_cmp_le(&tq->p[i]->t, &tq->p[j]->t)) {
				tn = tq->p[i];
				tq->p[i] = tq->p[j];
				tq->p[j] = tn;
				i = j;
			}
			else
				break;
		}
	}
	/* TODO - fix heap */
	pthread_mutex_unlock(&tq_mutex);
	LOG_DEBUG("A timer added");
	return ret;
}

int timer_queue_del_node(timer_node* tn) {
	pthread_mutex_lock(&tq_mutex);
	tn->deleted = 1;
	pthread_mutex_unlock(&tq_mutex);
	LOG_DEBUG("A timer tagged deleted");
	return 0;
}

int timer_queue_clean(timer_queue* tq) {
	pthread_mutex_lock(&tq_mutex);

	/* while tq is not empty:
	 *     if tq_min is timeout:
	 *         delete tq_min
	 *     else:
	 *         break
	 */

	struct timeval cur_t;
	gettimeofday(&cur_t, NULL);
	double t_lim = timeval_to_ms(&cur_t) - 300;
	while(!timer_queue_empty(tq)) {
		double t = timeval_to_ms(&timer_queue_get_min(tq)->t);
		if(t < t_lim) {
			timer_queue_del_min(tq);
			LOG_DEBUG("A timer deleted [time: lim] [%lf %lf]", t, t_lim);
		}
		else
			break;
	}
	pthread_mutex_unlock(&tq_mutex);
}

timer_node* timer_queue_get_min(timer_queue* tq) {
	timer_node* ret;
	ret = tq->p[0];
	return ret;
}

int timer_queue_del_min(timer_queue* tq) {
	if(tq->n <= 0) {
		return 0;
	}
	timer_node* tn = timer_queue_get_min(tq);
	if(tn->deleted) {
		/* connection closed by client */
		free(tn); 
	}
	else {
		/* for those sockets which are timeout */
		/* need to free request and close fd */
		if(((http_request_t*)tn->req)->fd == 0) {
			LOG_ERR("");
		}
		close(((http_request_t*)tn->req)->fd);
		free(tn->req);
		free(tn); 
	}
	/* A timenode deleted, fixing heap */
	tq->n--;
	tq->p[0] = tq->p[tq->n];
	int i = 0;
	while(i < tq->n) {
		int j = -1;
		if(2 * i + 2 < tq->n) {
			j = timeval_cmp_le(&tq->p[2 * i + 1]->t, &tq->p[2 * i + 2]->t) ? 
						2 * i + 1 : 2 * i + 2;
		}
		else if(2 * i + 1 < tq->n) {
			j = 2 * i + 1;
		}
		else
			break;

		if(j >= 0 && timeval_cmp_le(&tq->p[j]->t, &tq->p[i]->t)) {
			timer_node* tn;
			tn = tq->p[i];
			tq->p[i] = tq->p[j];
			tq->p[j] = tn;
			i = j;
		}
		else
			break;
	}
	return 0;
}

int timer_queue_empty(timer_queue* tq) {
	if(tq->n <= 0)
		return 1;
	else
		return 0;
}

int timeval_cmp_le(struct timeval* a, struct timeval* b) {
	double ta = a->tv_sec + a->tv_usec / 1000000;
	double tb = b->tv_sec + b->tv_usec / 1000000;
	if(ta <= tb)
		return 1;
	else
		return 0;
}

double timeval_to_ms(struct timeval* tv) {
	return tv->tv_sec * 1000 + (double)tv->tv_usec / 1000;
}
