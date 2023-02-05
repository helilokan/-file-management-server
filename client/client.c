#include "threadPool.h"
#include "worker.h"
#define NUM 3
int exitPipe[2];
void sigFunc(int signum)
{
    // printf("signum = %d\n", signum);
    write(exitPipe[1], "1", 1);
    // printf("Parent process is going to die!\n");
}
void sigFunc2(int signum)
{
    
}
int main()
{
    pipe(exitPipe);
    if (fork() != 0) // 父进程实现退出
    {
        close(exitPipe[0]);
        signal(SIGUSR1, sigFunc);
        signal(SIGINT, sigFunc);
        wait(NULL);
        exit(0);
    }
    close(exitPipe[1]);
    int sockFd = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_CHECK(sockFd, -1, "socket");

    // 制作子线程
    int workerNum = NUM;
    threadPool_t threadPool;
    threadPoolInit(&threadPool, workerNum);
    makeWorker(&threadPool);// 全部都等待唤醒


    // 主线程建立连接
    tcpInit(&sockFd, "./config/client.conf");
    printf("connecting\n");

    int flag;
    char uName[100] = {0};
    
    // 整个登录注册的过程
    while(1)
    {
        fflush(stdin);
        flag = 0;
        printf("\033[1;37mPlease enter 1 to register a new user;");
        printf("or enter 2 to login: \033[0m");
        fflush(stdout);
        char buf[10] = {0};
        read(STDIN_FILENO, buf, sizeof(buf));
        // read(STDIN_FILENO, &flag, sizeof(int));
        // printf("buf = %s\n", buf);
        flag = atoi(buf);
        // scanf("%d", &flag);// 用scanf如果输入字母会死循环
        // printf("flag = %d\n", flag);
        if(flag == 1)
        {
            int ret = register_user(sockFd);
            if(ret == 1)
            {
                printf("\033[1;37mregister failed!\033[0m\n");
                continue;
            }
            else if(ret == 2)
            {
                printf("\033[1;37mUsername exist, register failed!\033[0m\n");
                continue;
            }
            else
            {
                printf("\033[1;30;46m注册成功，欢迎加入青青草原养猪场\033[0m\n");
                continue;
            }
        }
        else if(flag == 2)
        {
            int ret = login(sockFd, uName);
            if(ret == 2)// 用户未注册
            {
                printf("\033[1;37mUnregistered user!\033[0m\n");
                continue;
            }
            else
            {
                printf("\033[1;30;46m%s你好，欢迎来到青青草原养猪场，请开始你的养猪之旅吧~\033[0m\n", uName);
                break;
            }
        }
        else
        {
            printf("\033[1;37mmWrong num!\033[0m\n");
            continue;
        }
    }
    signal(SIGINT, sigFunc2);// 登录成功后注册信号
    int epfd = epoll_create(1);
    epollAdd(STDIN_FILENO, epfd);// 监听标准输入
    epollAdd(exitPipe[0], epfd); // 监听退出请求
    epollAdd(sockFd, epfd);// 监听服务器是否发送超时要求断开的消息
    struct epoll_event readyArr[2];

    // 获取token和key，用于子线程连接验证
    char token[1024] = {0};
    char key[100] = {0};
    int datalength;
    recvn(sockFd, &datalength, sizeof(int));
    recvn(sockFd, token, datalength);
    recvn(sockFd, &datalength, sizeof(int));
    recvn(sockFd, key, datalength);

    // 记录当前目录，刚登陆默认在家目录
    char pwd[1024] = {0};
    recvn(sockFd, &datalength, sizeof(int));
    recvn(sockFd, pwd, datalength);

    // 服务端保存token和key
    server_t server;
    bzero(&server, sizeof(server));
    strcpy(server.token, token);
    strcpy(server.key, key);
    strcpy(server.user_name, uName);

    // 开始接收命令，循环读取
    char cmd[1024] = {0};
    train_state_t train_state;
    while (1)
    {
        printf("\33[1m\33[32m[%s@ %s]: \33[0m", uName, pwd);
        fflush(stdout);
        fflush(stdin);
        int readyNum = epoll_wait(epfd, readyArr, 2, -1);
        // printf("epoll_wait ready!\n");
        for (int i = 0; i < readyNum; ++i)
        {
            if(readyArr[i].data.fd == sockFd)// 服务端发送了超时断开要求
            {
                int check;
                recv(sockFd, &check, sizeof(check), 0);
                printf("time out bye!\n");

                train_t train;
                bzero(&train, sizeof(train));
                train.length = 0;
                send(sockFd, &train, sizeof(train.length) + train.length, 0);
                // printf("child process, threadPool is going to die!\n");
                threadPool.exitFlag = 1;
                pthread_cond_broadcast(&threadPool.taskQueue.cond);
                pthread_cond_broadcast(&threadPool.taskQueue.cond); // 叫醒所有线程
                for (int j = 0; j < workerNum; ++j)
                {
                    pthread_join(threadPool.tid[j], NULL); // 等待子线程终结
                }
                pthread_exit(NULL);
            }
            else if (readyArr[i].data.fd == STDIN_FILENO)
            {
                bzero(cmd, sizeof(cmd));
                read(STDIN_FILENO, cmd, sizeof(cmd));
                cmd[strlen(cmd) - 1] = 0;
                printf("cmd = %s\n", cmd);
                bzero(&train_state, sizeof(train_state));
                if (strlen(cmd) < 2)
                {
                    printf("wrong cmd!\n");
                    continue;
                }
                if (strncmp("cd", cmd, 2) == 0)
                {
                    if (strlen(cmd) == 2 || cmd[2] != ' ')
                    {
                        printf("wrong cmd!\n");
                        continue;
                    }
                    train_state.state = CD;
                    strcpy(train_state.buf, cmd);
                    train_state.length = strlen(cmd);
                    train_state.pre_length = 2;
                    send(sockFd, &train_state, sizeof(train_state.length) + sizeof(train_state.state) + sizeof(train_state.pre_length) + train_state.length, 0);
                    int check = 0;
                    recvn(sockFd, &check, sizeof(int));
                    if (check != 0)
                    {
                        printf("Wrong dir name!\n");
                        continue;
                    }
                    recvn(sockFd, &datalength, sizeof(int));
                    bzero(pwd, sizeof(pwd));
                    recvn(sockFd, pwd, datalength);
                }

                else if (strncmp("ls", cmd, 2) == 0)
                {
                    if (strlen(cmd) > 2 && cmd[2] != ' ')
                    {
                        printf("wrong cmd!\n");
                        continue;
                    }
                    train_state.state = LS;
                    strcpy(train_state.buf, cmd);
                    train_state.length = strlen(cmd);
                    train_state.pre_length = 2;
                    send(sockFd, &train_state, sizeof(train_state.length) + sizeof(train_state.state) + sizeof(train_state.pre_length) + train_state.length, 0);
                    int check = 0;
                    recvn(sockFd, &check, sizeof(int));
                    printf("check = %d\n", check);
                    if (check != 0)
                    {
                        printf("Wrong dir name!\n");
                        continue;
                    }
                    char file_name[100] = {0};
                    while(1)
                    {
                        recvn(sockFd, &datalength, sizeof(datalength));
                        if(datalength == 0)
                        {
                            break;
                        }
                        bzero(file_name, sizeof(file_name));
                        recvn(sockFd, file_name, datalength);
                        printf("%-8s ", file_name);
                    }
                    printf("\n");
                }

                else if (strncmp("pwd", cmd, 3) == 0)
                {
                    if (strlen(cmd) > 3)
                    {
                        printf("wrong cmd!\n");
                        continue;
                    }
                    train_state.state = PWD;
                    strcpy(train_state.buf, cmd);
                    train_state.length = strlen(cmd);
                    train_state.pre_length = 3;
                    send(sockFd, &train_state, sizeof(train_state.length) + sizeof(train_state.state) + sizeof(train_state.pre_length) + train_state.length, 0);
                    bzero(pwd, sizeof(pwd));
                    recvn(sockFd, &datalength, sizeof(int));
                    recvn(sockFd, pwd, datalength);
                    printf("%s\n", pwd);
                }

                else if (strncmp("rm", cmd, 2) == 0)
                {
                    if (strlen(cmd) == 2 || cmd[2] != ' ')
                    {
                        printf("wrong cmd!\n");
                        continue;
                    }
                    train_state.state = REMOVE;
                    strcpy(train_state.buf, cmd);
                    train_state.length = strlen(cmd);
                    train_state.pre_length = 2;
                    send(sockFd, &train_state, sizeof(train_state.length) + sizeof(train_state.state) + sizeof(train_state.pre_length) + train_state.length, 0);
                    int check;
                    recvn(sockFd, &check, sizeof(check));
                    if(check != 0)
                    {
                        printf("Wrong file name!\n");
                    }
                }

                else if (strncmp("mkdir", cmd, 5) == 0)
                {
                    if (strlen(cmd) == 5 || cmd[5] != ' ')
                    {
                        printf("wrong cmd!\n");
                        continue;
                    }
                    train_state.state = MKDIR;
                    strcpy(train_state.buf, cmd);
                    train_state.length = strlen(cmd);
                    train_state.pre_length = 5;
                    send(sockFd, &train_state, sizeof(train_state.length) + sizeof(train_state.state) + sizeof(train_state.pre_length) + train_state.length, 0);
                    int check = 0;
                    recvn(sockFd, &check, sizeof(int));
                    if (check != 0)
                    {
                        printf("Can not mkdir!\n");
                        continue;
                    }
                }

                else if (strncmp("puts", cmd, 4) == 0)
                {
                    if (strlen(cmd) == 4 || cmd[4] != ' ')
                    {
                        printf("wrong cmd!\n");
                        continue;
                    }
                    // 这里只是为了重置一下时间
                    train_state.state = PUTS;
                    strcpy(train_state.buf, cmd);
                    train_state.length = strlen(cmd);
                    train_state.pre_length = 4;
                    send(sockFd, &train_state, sizeof(train_state.length) + sizeof(train_state.state) + sizeof(train_state.pre_length) + train_state.length, 0);
                    // 唤醒子线程
                    strcpy(server.cmd, cmd);
                    pthread_mutex_lock(&threadPool.taskQueue.mutex);
                    taskEnqueue(&threadPool.taskQueue, &server);
                    pthread_cond_signal(&threadPool.taskQueue.cond); // 唤醒子线程
                    pthread_mutex_unlock(&threadPool.taskQueue.mutex);
                }

                else if (strncmp("gets", cmd, 4) == 0)
                {
                    if (strlen(cmd) == 4 || cmd[4] != ' ')
                    {
                        printf("wrong cmd!\n");
                        continue;
                    }
                    // 这里只是为了重置一下时间
                    train_state.state = GETS;
                    strcpy(train_state.buf, cmd);
                    train_state.length = strlen(cmd);
                    train_state.pre_length = 4;
                    send(sockFd, &train_state, sizeof(train_state.length) + sizeof(train_state.state) + sizeof(train_state.pre_length) + train_state.length, 0);
                    // 唤醒子线程
                    strcpy(server.cmd, cmd);
                    pthread_mutex_lock(&threadPool.taskQueue.mutex);
                    taskEnqueue(&threadPool.taskQueue, &server);
                    pthread_cond_signal(&threadPool.taskQueue.cond); // 唤醒子线程
                    pthread_mutex_unlock(&threadPool.taskQueue.mutex);
                }

                else
                {
                    printf("wrong cmd!\n");
                    continue;
                }
            }
            else// 退出
            {
                train_t train;
                bzero(&train, sizeof(train));
                train.length = 0;
                send(sockFd, &train, sizeof(train.length) + train.length, 0);
                // printf("child process, threadPool is going to die!\n");
                threadPool.exitFlag = 1;
                pthread_cond_broadcast(&threadPool.taskQueue.cond); // 叫醒所有线程
                for (int j = 0; j < workerNum; ++j)
                {
                    pthread_join(threadPool.tid[j], NULL); // 等待子线程终结
                }
                pthread_exit(NULL);
            }
        }
    }
    close(sockFd);
}
int threadPoolInit(threadPool_t *pThreadPool, int workerNum)
{
    pThreadPool->threadNum = workerNum;
    pThreadPool->tid = (pthread_t *)calloc(workerNum, sizeof(pthread_t));
    pThreadPool->taskQueue.pFront = NULL;
    pThreadPool->taskQueue.pRear = NULL;
    pThreadPool->taskQueue.size = 0;
    pthread_mutex_init(&pThreadPool->taskQueue.mutex, NULL);
    pthread_cond_init(&pThreadPool->taskQueue.cond, NULL);
    pThreadPool->exitFlag = 0;
}