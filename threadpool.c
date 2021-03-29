#include "threadpool.h"

#include <stdlib.h>
#include <stdio.h>

// init routine of a thread
void* start(void* arg){
    tp_task* cur_task = NULL;
    tp* pool = (tp*)arg; // arg should be thread pool itself
    while(1){
        if(pthread_mutex_lock(&pool->mutex) != 0){
            perror("lock mutex");
            exit(-1);
        }
        while(pool->task_cnt == 0){
            if(pthread_cond_wait(&pool->cond, &pool->mutex) != 0){
                perror("cond wait");
                exit(-1);
            }
        }

        // pop a task from task list
        cur_task = pool->task;
        pool->task = pool->task->next;
        pool->task_cnt -= 1;

        if(pthread_mutex_unlock(&pool->mutex) != 0){
            perror("unlock mutex");
            exit(-1);
        }
    
        // run task
        cur_task->f(cur_task->arg);

        // gc
        free(cur_task);
        cur_task = NULL;

        // TODO -
        //pthread_testcancel();
    }
    return NULL;
}

// init a threadpool with default config
int threadpool_init(tp* pool){
    if(pool == NULL)
        return -1;
    pool->size = INIT_TH_CNT;
    //p->shutdown = 0;

    // init task
    pool->task = NULL;
    pool->task_cnt = 0;

    // init mutex & cond
    if(pthread_mutex_init(&pool->mutex, NULL) != 0){
        perror("pthread mutex init");
        exit(-1);
    }
    if(pthread_cond_init(&pool->cond, NULL) != 0){
        perror("pthread cond init");
        exit(-1);
    }
    /*
    bug record: 
    Initialization of mutex and cond was before pthread_create,
    and then something wrong happened.
    */

    // create threads
    pool->threads = (pthread_t*)malloc(pool->size * sizeof(pthread_t));
    if(pool->threads == NULL){
        perror("malloc");
        exit(-1);
    }
    pthread_t new_thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    for(int i = 0; i < pool->size; i++){
        if(pthread_create(&pool->threads[i], &attr, start, pool) != 0){
            perror("pthread create");
            exit(-1);
        }
    }

    return 0;
}

// add a [tp_task task] to [threadpool pool]
int threadpool_addtask(tp* pool, tp_task* task){
    if(pool == NULL || task == NULL){
        perror("nullptr");
        exit(-1);
    }
    
    if(pthread_mutex_lock(&pool->mutex) != 0){
        perror("lock mutex");
        exit(-1);
    }

    if(pool->task_cnt == 0)
        pool->task = task;
    else{
        tp_task* it = pool->task;
        while(it->next != NULL)
            it = it->next;
        it->next = task;
    }
    pool->task_cnt += 1;
    
    if(pthread_mutex_unlock(&pool->mutex) != 0){
        perror("unlock mutex");
        exit(-1);
    }
    if(pthread_cond_signal(&pool->cond) != 0){
        perror("signal cond");
        exit(-1);
    }
        
    return 0;
}

// ???
int threadpool_destroy(tp* pool){
    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->cond);
    // TODO -
/*    for(int i = 0; i < pool->size; i++){
        if(pthread_cancel(pool->threads[i]) != 0){
            perror("pthread cancel");
            exit(-1);
        }
    }*/
    free(pool->threads);
}

// create & init & return a basic threadpool
tp* threadpool_create(){
	tp* pool = (tp*)malloc(sizeof(tp));
    if(pool == NULL){
        perror("malloc");
        exit(-1);
    }
	if(threadpool_init(pool) < 0){
        perror("create tp");
        exit(-1);
    }
    return pool;
}

// create & return a tp_task struct(a node of linked list)
tp_task* threadpool_create_tp_task(void*(*f)(void*), void* arg){
	tp_task* task = (tp_task*)malloc(sizeof(tp_task));
    if(task == NULL){
        perror("malloc");
        exit(-1);
    }
	task->f = f;
	task->arg = arg;
	task->next = NULL;
	return task;
}
