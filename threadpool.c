//
// Created by a7233 on 2024/8/21.
//

#include <memory.h>
#include "threadpool.h"
#include "pthread.h"

typedef struct Task
{
    void (*function)(void *arg);

    void *arg;
} Task;


struct ThreadPool
{
    Task *TaskQ;
    int queueCapacity;
    int queueSize;
    int queueFront;
    int queueRear;

    pthread_t managerID;
    pthread_t *threadIDs;

    int minNum;
    int maxNum;
    int busyNum;
    int liveNum;
    int exitNum;

    pthread_mutex_t mutexBusy;
    pthread_mutex_t mutexPool;
    pthread_cond_t notFull;
    pthread_cond_t notEmpty;

    int shutdown;
};

ThreadPool *threadPoolCreate(int min, int max, int queueSize)
{
    ThreadPool *pool = (ThreadPool *) malloc(sizeof(ThreadPool));
    if (pool == NULL)
    {
        printf("malloc failed for thread pool\n");
        return NULL;
    }

    pool->maxNum = max;
    pool->minNum = min;
    pool->busyNum = 0;
    pool->liveNum = min;
    pool->exitNum = 0;
    pool->threadIDs = (pthread_t *) calloc(max, sizeof(pthread_t));
    if (pool->threadIDs == NULL)
    {
        printf("malloc failed for working threads\n");
        return NULL;
    }

    if (pthread_mutex_init(&pool->mutexPool, NULL) != 0
        || pthread_mutex_init(&pool->mutexBusy, NULL) != 0
        || pthread_cond_init(&pool->notFull, NULL) != 0
        || pthread_cond_init(&pool->notEmpty, NULL) != 0)
    {
        printf("Mutex or cond init failed\n");
    }
    pool->queueCapacity = queueSize;
    pool->TaskQ = (Task *) calloc(pool->queueCapacity, sizeof(Task));
    pool->queueSize = 0;
    pool->queueFront = 0;
    pool->queueRear = 0;

    pool->shutdown = 0;
    return pool;
}


