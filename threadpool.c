//
// Created by a7233 on 2024/8/21.
//

#include <memory.h>
#include <unistd.h>
#include "threadpool.h"
#include "pthread.h"

const int NUMBER = 2;

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
    do
    {
        if (pool == NULL)
        {
            printf("malloc failed for thread pool\n");
            break;
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
            break;
        }

        if (pthread_mutex_init(&pool->mutexPool, NULL) != 0
            || pthread_mutex_init(&pool->mutexBusy, NULL) != 0
            || pthread_cond_init(&pool->notFull, NULL) != 0
            || pthread_cond_init(&pool->notEmpty, NULL) != 0)
        {
            printf("Mutex or cond init failed\n");
            break;
        }


        pool->queueCapacity = queueSize;
        pool->TaskQ = (Task *) calloc(pool->queueCapacity, sizeof(Task));
        if (pool->TaskQ == NULL)
        {
            printf("malloc failed for task queue\n");
            break;
        }
        pool->queueSize = 0;
        pool->queueFront = 0;
        pool->queueRear = 0;

        pthread_create(&pool->managerID, NULL, manager, NULL);

        for (int i = 0; i < pool->minNum; i++)
        {
            pthread_create(pool->threadIDs + i, NULL, worker, pool);
        }
        pool->shutdown = 0;
        return pool;

    } while (0);


    if (pool->threadIDs) free(pool->threadIDs);
    if (pool->TaskQ) free(pool->TaskQ);
    if (pool) free(pool);

    return NULL;
}

void *worker(void *arg)
{
    ThreadPool *pool = (ThreadPool *) arg;
    while (1)
    {
        pthread_mutex_lock(&pool->mutexPool);
        while (pool->queueSize == 0 && !pool->shutdown)
        {
            pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);
            if (pool->exitNum > 0)
            {
                pool->exitNum--;
                pthread_mutex_unlock(&pool->mutexPool);
                pthread_exit(NULL);
            }
        }

        if (pool->shutdown)
        {
            pthread_mutex_unlock(&pool->mutexPool);
            pthread_exit(NULL);
        }

        Task task;
        task.function = pool->TaskQ[pool->queueFront].function;
        task.arg = pool->TaskQ[pool->queueFront].arg;
        pool->queueFront = pool->queueFront % pool->queueCapacity;
        pool->queueSize--;
        pthread_mutex_unlock(&pool->mutexPool);

        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyNum++;
        pthread_mutex_unlock(&pool->mutexBusy);

        printf("thread %ld is working...\n");
        (*task.function)(task.arg);
        free(task.arg);
        task.arg = NULL;

        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyNum--;
        pthread_mutex_unlock(&pool->mutexBusy);
    }
    return NULL;
}

void *manager(void *arg)
{
    ThreadPool *pool = (ThreadPool *) arg;
    int maxNum = pool->maxNum;
    int minNum = pool->minNum;
    int liveNum = pool->liveNum;
    while (!pool->shutdown)
    {
        sleep(1);
        // create threads
        pthread_mutex_lock(&pool->mutexPool);
        int queueSize = pool->queueSize;
        if (queueSize > liveNum && liveNum < maxNum)
        {
            int counter = 0;
            for (int i = 0; i < maxNum && counter < NUMBER; ++i)
            {
                if (pool->threadIDs[i] == 0)
                {
                    pthread_create(&pool->threadIDs[i], NULL, worker, pool);
                    pool->liveNum = ++liveNum;
                    ++counter;
                }

            }
        }
        pthread_mutex_unlock(&pool->mutexPool);
        sleep(1);
        // destroy threads
        pthread_mutex_lock(&pool->mutexBusy);
        int busyNum = pool->busyNum;
        if (busyNum * 2 < liveNum && liveNum > minNum)
        {
            pthread_mutex_lock(&pool->mutexPool);
            pool->exitNum = NUMBER;
            pthread_mutex_unlock(&pool->mutexPool);

            for (int i = 0; i < NUMBER; ++i) // wakeup NUMBER=2 threads
            {
                pthread_cond_signal(&pool->notEmpty);
            }

        }
        pthread_mutex_unlock(&pool->mutexBusy);
    }
    return NULL;
}