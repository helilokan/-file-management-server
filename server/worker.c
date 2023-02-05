#include"threadPool.h"
#include "worker.h"
// 优雅的退出，完成任务再退出
void cleanFunc(void *arg)
{
    threadPool_t * pThreadPool = (threadPool_t*)arg;
    pthread_mutex_unlock(&pThreadPool->taskQueue.mutex);
}
void *handleEvent(void *arg)
{
    threadPool_t * pThreadPool = (threadPool_t*)arg;
    int netFd;
    int user_id;
    while(1)
    {
        printf("I am free!tid = %lu\n", pthread_self());
        pthread_mutex_lock(&pThreadPool->taskQueue.mutex);
        pthread_cleanup_push(cleanFunc, (void*)pThreadPool);// 加锁后就push
        while(pThreadPool->taskQueue.size == 0 && pThreadPool->exitFlag == 0)// wait条件需要更改，否则唤醒后又会wait
        {
            pthread_cond_wait(&pThreadPool->taskQueue.cond, &pThreadPool->taskQueue.mutex);
        }
        if(pThreadPool->exitFlag != 0)
        {
            // printf("I am going to die!\n");
            pthread_exit(NULL);
        }
        netFd = pThreadPool->taskQueue.pFront->netFd;
        user_id = pThreadPool->taskQueue.pFront->code;
        taskDequeue(&pThreadPool->taskQueue);// 任务内容获取完成
        pthread_cleanup_pop(1);// 原来解锁的位置改为pop

        printf("I am working! tid = %lu\n", pthread_self());

        handle_long_cmd(netFd, user_id);// 开始处理命令，直接用netFd，监听客户端发来的命令，这个netFd是客户端子线程发来的

        printf("done!\n");
        close(netFd);
    }
}
int makeWorker(threadPool_t *pThreadPool)
{
    for(int i = 0; i < pThreadPool->threadNum; ++i)
    {
        pthread_create(&pThreadPool->tid[i], NULL, handleEvent, (void *)pThreadPool);
    }
}
int handle_long_cmd(int netFd, int user_id)
{
    puts("I am long cmd!");
    // 接收命令
    char cmd[100] = {0};
    int datalength;
    recvn(netFd, &datalength, sizeof(datalength));
    recvn(netFd, cmd, datalength);
    sysadd(user_id, cmd);
    
    // printf("cmd = %s\n", cmd);
    
    if (strncmp("puts", cmd, 3) == 0)
    {
        Download(netFd, user_id);
    }
    else// 不是puts就是gets，其他的以后再说
    {
        char file[100] = {0};
        int i = 4;
        while (cmd[i] == ' ')
        {
            ++i;
        }
        strcpy(file, cmd + i);
        // printf("file = %s\n", file);
        Upload(netFd, file, user_id);
    }
}