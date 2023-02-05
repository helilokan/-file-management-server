#include"threadPool.h"
#include"worker.h"
// 优雅的退出，完成任务再退出
void cleanFunc(void *arg)
{
    threadPool_t * pThreadPool = (threadPool_t*)arg;
    pthread_mutex_unlock(&pThreadPool->taskQueue.mutex);
}
void *handleEvent(void *arg)
{
    threadPool_t * pThreadPool = (threadPool_t*)arg;
    server_t *pserver;
    while(1)
    {
        // printf("I am free!tid = %lu\n", pthread_self());
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
        pserver = pThreadPool->taskQueue.pFront->pserver;
        taskDequeue(&pThreadPool->taskQueue);// 任务内容获取完成
        pthread_cleanup_pop(1);// 原来解锁的位置改为pop

        // printf("I am working! tid = %lu\n", pthread_self());

        handle_long_cmd(pserver);// 开始处理命令，建立新连接，用于长命令的发送

        // printf("done!\n");
    }
}
int makeWorker(threadPool_t *pThreadPool)
{
    for(int i = 0; i < pThreadPool->threadNum; ++i)
    {
        pthread_create(&pThreadPool->tid[i], NULL, handleEvent, (void *)pThreadPool);
    }
}
int handle_long_cmd(server_t *pserver)
{
    // puts("I am long cmd!");
    int sockFd_child = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_CHECK(sockFd_child, -1, "socket");
    tcpInit(&sockFd_child, "./config/client.conf");
    // printf("Child connecting\n");
    int check = 0;
    int times = 0;
    // 发送token验证消息
    while(1)
    {
        train_state_t train_state;
        strcpy(train_state.buf, pserver->user_name);
        strcat(train_state.buf, pserver->token);
        train_state.state = TOKEN;
        train_state.length = strlen(train_state.buf);
        train_state.pre_length = strlen(pserver->user_name);
        send(sockFd_child, &train_state, sizeof(train_state.length) + sizeof(train_state.state) + sizeof(train_state.pre_length) + train_state.length, 0);
        recvn(sockFd_child, &check, sizeof(check));
        if(check == 0)
        {
            // 验证成功退出
            break;
        }
        if (check == 1)
        {
            printf("Try again!\n");// 利用key重新生成token
            encode(pserver->key, pserver->user_name, pserver->token);
            ++times;
        }
        if(times == 6)// 可以试5次
        {
            close(sockFd_child);
            return -1;  
        }
    }

    // 验证成功发送命令
    // printf("cmd = %s\n", pserver->cmd);
    train_t train;
    train.length = strlen(pserver->cmd);
    strcpy(train.buf, pserver->cmd);
    send(sockFd_child, &train, sizeof(train.length) + train.length, 0);
    if (strncmp("puts", pserver->cmd, 4) == 0)
    {
        // 获取文件名
        char file[100] = {0};
        int i = 4;
        while (pserver->cmd[i] == ' ')
        {
            ++i;
        }
        strcpy(file, pserver->cmd + i);
        // printf("file = %s\n", file);
        Upload(sockFd_child, file);
    }
    else// 不是puts就是gets，其他的以后再说
    {
        Download(sockFd_child);
    }
    close(sockFd_child);
}