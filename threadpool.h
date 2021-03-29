#ifndef _THREADPOOL_H
#define _THREADPOOL_H 1

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>

#define MAX_THREAD_N 32

typedef struct tp_task{
	void*(*f)(void*);
	void* arg;
	struct tp_task* next;
}tp_task;

typedef struct threadpool{
	int             size;       // max number of threads
	int             shutdown;
	pthread_t*      threads;    // linked list
	tp_task*        task;
	pthread_mutex_t mutex;
	sem_t			sem;
}threadpool;

typedef threadpool tp;

tp* threadpool_create(){
	tp* pool = (tp*)malloc(sizeof(tp));
	threadpool_init(pool);
}
int threadpool_init(tp* p);
int threadpool_addtask(tp* p, tp_task* t);
int threadpool_destroy(tp* p);

tp_task* threadpool_create_task(void*(*f)(void*), void* arg){
	tp_task* task = (tp_task*)malloc(sizeof(tp_task));
	task->f = f;
	task->arg = arg;
	task->next = NULL;
	return task;
}



#endif