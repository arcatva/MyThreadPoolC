//
// Created by a7233 on 2024/8/21.
//

#ifndef MYTHREADPOOLC_THREADPOOL_H
#define MYTHREADPOOLC_THREADPOOL_H

#include <malloc.h>

typedef struct Task
{
    void (*function)(void *arg);

    void *arg;
} Task;


typedef struct ThreadPool ThreadPool;

ThreadPool *threadPoolCreate(int min, int max, int queueSize);

void *worker(void *arg);

#endif //MYTHREADPOOLC_THREADPOOL_H
