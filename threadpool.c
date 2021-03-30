#include "threadpool.h"

#include <stdlib.h>
#include <stdio.h>

// init routine of a thread
void* worker(void* arg){
    if(arg == NULL){
        perror("worker arg is nullptr");
        exit(-1);
    }
    tp_task* cur_task = NULL;
    tp* pool = (tp*)arg;

    while(1){
        if(pthread_mutex_lock(&pool->mutex) != 0){
            perror("pthread mutex lock");
            exit(-1);
        }
        while(pool->task_cnt == 0 && pool->shutdown != 1){
            if(pthread_cond_wait(&pool->cond, &pool->mutex) != 0){
                perror("pthread cond wait");
                exit(-1);
            }
        }

        /* return if shutdown */
        if(pool->shutdown){
            if(pthread_mutex_unlock(&pool->mutex) != 0){
                perror("pthread mutex unlock");
                exit(-1);
            }
            pthread_exit(NULL);
        }

        // pop a task from task list
        cur_task = pool->firsttask;
        pool->firsttask = pool->firsttask->next;
        pool->task_cnt -= 1;

        if(pthread_mutex_unlock(&pool->mutex) != 0){
            perror("pthread mutex unlock");
            exit(-1);
        }
        
        if(cur_task == NULL){
            perror("task is nullptr"); // unexpected
            exit(-1);
        }

        // run task
        cur_task->f(cur_task->arg);

        // gc
        free(cur_task);
        cur_task = NULL;
    }
    return NULL;
}

// init a threadpool with default config
int threadpool_init(tp* pool){
    if(pool == NULL)
        return -1;
    pool->size = MAX_TH_NUM;
    pool->shutdown = 0;

    // init task
    pool->firsttask = NULL;
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
    Because threads may run before mutex and cond are initialized.
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
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    for(int i = 0; i < pool->size; i++){
        if(pthread_create(&pool->threads[i], &attr, worker, pool) != 0){
            perror("pthread create");
            exit(-1);
        }
    }

    return 0;
}

// add a [tp_task task] to [threadpool pool]
int threadpool_addtask(tp* pool, tp_task* task){
    if(pool == NULL || task == NULL){
        perror("pool or task is nullptr");
        exit(-1);
    }

    if(pthread_mutex_lock(&pool->mutex) != 0){
        perror("pthread mutex lock");
        exit(-1);
    }

    if(pool->shutdown){
        perror("add after shut");
        exit(-1);
    }

    if(pool->task_cnt == 0)
        pool->firsttask = task;
    else{
        tp_task* it = pool->firsttask;
        while(it->next != NULL)
            it = it->next;
        it->next = task;
    }
    pool->task_cnt += 1;
    
    if(pthread_mutex_unlock(&pool->mutex) != 0){
        perror("pthread mutex unlock");
        exit(-1);
    }
    if(pthread_cond_signal(&pool->cond) != 0){
        perror("pthread cond signal");
        exit(-1);
    }
    return 0;
}

// free & destroy
int threadpool_destroy(tp* pool){
    if(pool == NULL){
        perror("tp nullptr");
        exit(-1);
    }
    if(pthread_mutex_lock(&pool->mutex) != 0){
        perror("pthread mutex lock");
        exit(-1);
    }
    pool->shutdown = 1;
    if(pthread_cond_broadcast(&pool->cond) != 0){
        perror("pthread cond broadcast");
        exit(-1);
    }
    printf("Waiting for all threads to terminate...\n");
    for(int i = 0; i < MAX_TH_NUM; i++){
        if(pthread_join(pool->threads[i], NULL) != 0){
            perror("pthread join");
            exit(-1);
        }
    }
    printf("All threads terminated.\n");
    if(pthread_mutex_unlock(&pool->mutex) != 0){
        perror("pthread mutex unlock");
        exit(-1);
    }

    if(pthread_mutex_destroy(&pool->mutex) != 0){
        perror("pthread mutex destroy");
        exit(-1);
    }
    if(pthread_cond_destroy(&pool->cond) != 0){
        perror("pthread cond destroy");
        exit(-1);
    }
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
