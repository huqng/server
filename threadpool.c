#include "threadpool.h"

#include <stdlib.h>

void* routine(void* arg){
    while(1){
        tp* p = (tp*)arg;

        pthread_mutex_lock(&p->mutex);
        sem_wait(&p->sem);

        tp_task* cur_task = (tp_task*)arg;
        p->task = p->task->next;

        pthread_mutex_unlock(&p->mutex);

        cur_task->f(cur_task->arg);
    }
    return NULL;
}

int threadpool_init(tp* p){
    if(p == NULL)
        return -1;
    p->size = MAX_THREAD_N;
    p->shutdown = 0;
    p->threads = (pthread_t*)malloc(p->size * sizeof(pthread_t));
    for(int i = 0; i < p->size; i++){
        void* arg = &p;
        pthread_create(&p->threads[i], NULL, routine, arg);
    }
    p->task = NULL;
    pthread_mutex_init(&p->mutex, NULL);
    sem_init(&p->sem, 0, 0);
    return 0;
}

int threadpool_addtask(tp* pool, tp_task* task){
    if(pool->task == NULL)
        pool->task = task;
    else{
        tp_task* it = pool->task;
        while(it->next != NULL)
            it = it->next;
        it->next = task;
    }
    sem_post(&pool->sem);
    
    return 0;
}


int threadpool_destroy(tp* p){
    return -1;
}