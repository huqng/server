#include "threadpool.h"


/* init routine of a thread */
void* worker(void* arg){
    //if(arg == NULL){
    //    perror("unexpected NULL in worker");
    //    exit(-1);
    //}
    tp_task* cur_task = NULL;
    tp* pool = (tp*)arg;

    /* run forever */
    while(1){
        //LOG_DEBUG("tp worker: entering loop");
        if(pthread_mutex_lock(&pool->mutex) != 0){
            perror("pthread mutex lock");
            exit(-1);
        }
        //LOG_DEBUG("tp worker: lock success, getting new task");
        /* if no task and pool is not shutdown, wait for new tasks */
        while(pool->task_cnt == 0 && pool->shutdown != 1){
            //LOG_DEBUG("tp worker: waiting for new tasks");
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

        /* pool has any tasks, get one to run */
        cur_task = pool->firsttask;
        pool->firsttask = cur_task->next;
        pool->task_cnt -= 1;

        //LOG_DEBUG("tp worker: get new task success");

        if(pthread_mutex_unlock(&pool->mutex) != 0){
            perror("pthread mutex unlock");
            exit(-1);
        }
        //LOG_DEBUG("tp worker: unlock success");
        
        if(cur_task == NULL){
            perror("task is NULL");
            exit(-1);
        }

        //LOG_DEBUG("tp worker: starting run");

        /* run task */
        cur_task->f(cur_task->arg);

        /* gc */
        free(cur_task->arg);
        free(cur_task);
        cur_task = NULL;
        //LOG_INFO("a task finished");
    }
    /* should not return */
    return NULL;
}

/* initialize a threadpool */
int threadpool_init(tp* pool, int nth){
    if(pool == NULL)
        return -1;
    pool->th_cnt = nth;
    pool->shutdown = 0;

    /* init head of task linked-list */
    pool->firsttask = NULL;
    pool->task_cnt = 0;

    /* init mutex & cond */
    if(pthread_mutex_init(&pool->mutex, NULL) != 0){
        perror("pthread mutex init");
        exit(-1);
    }
    if(pthread_cond_init(&pool->cond, NULL) != 0){
        perror("pthread cond init");
        exit(-1);
    }

    /* create threads */
    pool->threads = (pthread_t*)malloc(pool->th_cnt * sizeof(pthread_t));
    if(pool->threads == NULL){
        perror("malloc");
        exit(-1);
    }
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    for(int i = 0; i < pool->th_cnt; i++){
        if(pthread_create(&pool->threads[i], &attr, worker, pool) != 0){
            perror("pthread create");
            exit(-1);
        }
    }
    return 0;
}

/* add a task to task queue of threadpool */
int threadpool_addtask(tp* pool, tp_task* task){
    //LOG_DEBUG("add task: starting");
    if(pool == NULL || task == NULL){
        perror("pool or task is NULL");
        exit(-1);
    }

    //LOG_DEBUG("add task: locking");

    if(pthread_mutex_lock(&pool->mutex) != 0){
        perror("pthread mutex lock");
        exit(-1);
    }

    //LOG_DEBUG("add task: locked");

    if(pool->shutdown){
        perror("add after shut");
        exit(-1);
    }

    //LOG_DEBUG("add task: adding");

    if(pool->task_cnt == 0) {
        //LOG_DEBUG("add task: added to head");
        pool->firsttask = task;
    }
    else{
        // TODO - add a tail pointer of linked list
        tp_task* it = pool->firsttask;
        while(it->next != NULL)
            it = it->next;
        it->next = task;
        //LOG_DEBUG("add task: added to tail");
    }
    pool->task_cnt += 1;

    //LOG_DEBUG("add task: unlocking");
    
    if(pthread_mutex_unlock(&pool->mutex) != 0){
        perror("pthread mutex unlock");
        exit(-1);
    }

    //LOG_DEBUG("add task: unlocked & signaling");

    if(pthread_cond_signal(&pool->cond) != 0){
        perror("pthread cond signal");
        exit(-1);
    }

    //LOG_DEBUG("add task: signaled & quit");

    return 0;
}

/* ... */
int threadpool_destroy(tp* pool){
    if(pool == NULL){
        perror("tp NULL");
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
    for(int i = 0; i < pool->th_cnt; i++){
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

/* create & init & return a (pointer to a) threadpool */
tp* threadpool_create(int nth){
	tp* pool = (tp*)malloc(sizeof(tp));
    if(pool == NULL){
        perror("malloc");
        exit(-1);
    }
	threadpool_init(pool, nth);
    return pool;
}

/* create & return a tp_task struct(a node of linked list) */
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
