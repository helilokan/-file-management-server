#ifndef _THREADPOOL_H// 头文件在编译时被多次引用，避免重复定义
#define _THREADPOOL_H
#include<func.h>
#include "worker.h"

typedef struct task_s{
    int code;
    int netFd;
    struct task_s *pNext;
} task_t;

typedef struct taskQueue_s{
    task_t *pFront;
    task_t *pRear;
    int size;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} taskQueue_t;

typedef struct threadPool_s{
    pthread_t *tid;
    int threadNum;
    taskQueue_t taskQueue;
    int exitFlag;
} threadPool_t;

// 线程池创建和传递命令所用函数
int taskEnqueue(taskQueue_t *pTaskQueue, int code, int netFd);
int taskDequeue(taskQueue_t *pTaskQueue);
int threadPoolInit(threadPool_t *pThreadPool, int workerNum);
int epollAdd(int fd, int epfd);
int epollDel(int fd, int epfd);
int tcpInit(int *pSockFd, char *file);
int makeWorker(threadPool_t *pThreadPool);

#endif