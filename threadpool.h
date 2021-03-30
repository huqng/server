#ifndef _THREADPOOL_H
#define _THREADPOOL_H 1

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>

#define MAX_TH_NUM 4

typedef struct tp_task{
	void*(*f)(void*);
	void* arg;
	struct tp_task* next;
}tp_task;

typedef struct threadpool{
	int             size;       // num of threads
	int             shutdown;	// 

	pthread_t*      threads;    // 
	tp_task*        firsttask;		// head of task-linked-list 
	int 			task_cnt;	//

	pthread_mutex_t mutex;		// 
	pthread_cond_t	cond;		//
}threadpool;

typedef threadpool tp;

tp* threadpool_create();
int threadpool_init(tp* p);
int threadpool_addtask(tp* p, tp_task* t);
int threadpool_destroy(tp* p);
int threadpool_terminate(tp* p);

tp_task* threadpool_create_tp_task(void*(*f)(void*), void* arg);



#endif