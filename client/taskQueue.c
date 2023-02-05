#include "threadPool.h"
#include "worker.h"
int taskEnqueue(taskQueue_t *pTaskQueue, server_t *pserver)
{
    task_t *pTask = (task_t *)calloc(1, sizeof(task_t));
    pTask->pserver = pserver;
    // memcpy(pTask->pserver, pserver, sizeof(server_t));
    // printf("check!\n"); 
    if(pTaskQueue->size == 0)
    {
        pTaskQueue->pFront = pTask;
        pTaskQueue->pRear = pTask;
    }
    else
    {
        pTaskQueue->pRear->pNext = pTask;
        pTaskQueue->pRear = pTask;
    }
    ++pTaskQueue->size;
    return 0;
}

int taskDequeue(taskQueue_t *pTaskQueue)
{
    if(pTaskQueue->pFront == NULL)
    {
        printf("Is empty!\n");
        return 0;
    }
    task_t *pCur = pTaskQueue->pFront;
    pTaskQueue->pFront = pCur->pNext;
    free(pCur);
    --pTaskQueue->size;
    return 0;
}