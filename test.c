#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>
#include "threadpool.h"
#include <time.h>
#include <unistd.h>
#include <math.h>

int cnt = 0;

void* f(void* arg){
    while(cnt < 0xFFFFFF){
        cnt++;
        if((double)rand() / RAND_MAX < (double)1 / 100000)
            printf("%d\n", cnt);
    }
}


int main(){
    tp* pool = threadpool_create();
    tp_task* task0 = threadpool_create_tp_task(f, NULL);
    tp_task* task1 = threadpool_create_tp_task(f, NULL);
    tp_task* task2 = threadpool_create_tp_task(f, NULL);
    tp_task* task3 = threadpool_create_tp_task(f, NULL);
    tp_task* task4 = threadpool_create_tp_task(f, NULL);
    tp_task* task5 = threadpool_create_tp_task(f, NULL);
    tp_task* task6 = threadpool_create_tp_task(f, NULL);
    tp_task* task7 = threadpool_create_tp_task(f, NULL);

    threadpool_addtask(pool, task0);
    threadpool_addtask(pool, task1);
    threadpool_addtask(pool, task2);
    threadpool_addtask(pool, task3);
    threadpool_addtask(pool, task4);
    threadpool_addtask(pool, task5);
    threadpool_addtask(pool, task6);
    threadpool_addtask(pool, task7);

    pthread_join(pool->threads[0], NULL);
    pthread_join(pool->threads[1], NULL);
    pthread_join(pool->threads[2], NULL);
    pthread_join(pool->threads[3], NULL);

    return 0;
}