#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>
#include "threadpool.h"

#define MAX_TH_N 2

int cnt = 0;
pthread_mutex_t mutex;

void* f(void* arg){
    while(cnt < 10000){
        pthread_mutex_lock(&mutex);
        printf("0x%0X: %d\n", pthread_self(), cnt++);
        pthread_mutex_unlock(&mutex);
    }
}

int main(){
    tp* t = threadpool_create();
    tp_task* task = threadpool_create_task(f, NULL);
    threadpool_init(t);
    threadpool_addtask(t, task);
    return 0;
}