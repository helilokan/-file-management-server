#include "threadPool.h"
#include "worker.h"

// 自己实现MSG_WAITALL
int recvn(int sockFd, void *pstart, int len)
{
    int total = 0;
    int ret;
    char *p = (char*)pstart;
    while(total < len)// 收完为止
    {
        ret = recv(sockFd, p + total, len - total, 0);
        if(ret == 0)// 进来说明没收完，没收完返回值却是0则说明对面断开了，可以退出
        {
            return 0;
        }
        ERROR_CHECK(ret, -1, "recv");
        total += ret;
    }
    return 0;
}

int Upload(int netFd, char *file)
{
    char file_path[1024] = {0};
    sprintf(file_path, "%s%s%s","upload", "/", file);
    int fd = open(file_path, O_RDWR);
    train_t train;
    if(fd == -1)// 打开文件错误需要通知对面
    {
        perror("open");
        bzero(&train, sizeof(train));
        train.length = 0;
        send(netFd, &train, sizeof(train.length) + train.length, 0);
    }

    // 发送文件名
    train.length = strlen(file);
    strcpy(train.buf, file);
    int ret = send(netFd, &train, sizeof(train.length) + train.length, MSG_NOSIGNAL);
    ERROR_CHECK(ret, -1, "send");

    // 发送文件总大小
    struct stat statbuf;
    ret = fstat(fd, &statbuf);
    ERROR_CHECK(ret, -1, "fstat");
    train.length = 4;
    int fileSize = statbuf.st_size;// 获得文件总大小
    memcpy(train.buf, &fileSize, sizeof(int));
    send(netFd, &train, sizeof(train.length) + train.length, MSG_NOSIGNAL);

    // 计算并发送文件md5值(这里大文件可能耗时会比较久)
    char md5_str[MD5_STR_LEN + 1] = {0};
    Compute_file_md5(file_path, md5_str);
    train.length = strlen(md5_str);
    strcpy(train.buf, md5_str);
    send(netFd, &train, sizeof(train.length) + train.length, MSG_NOSIGNAL);

    // 确认是否需要上传
    int check;
    recvn(netFd, &check, sizeof(check));
    if(check == 0)// 不需要上传
    {
        return 0;
    }

    // 获得文件已传大小
    int exitSize = 0;
    recvn(netFd, &exitSize, sizeof(exitSize));

    // printf("time to puts!\n");

    // 发送文件内容
    // 判断文件大小，大文件用一次性传输
    char *p = (char*)mmap(NULL, fileSize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    int total = exitSize;
    // printf("exitSize = %d\n", exitSize);
    // printf("fileSize = %d\n", fileSize);
    if(fileSize >= 1073741824)// 大于1G
    {
        send(netFd, p + total, fileSize, MSG_NOSIGNAL);
        printf("send once!\n");
    }
    else
    {
        // 小于1G用小火车分次传输
        while (total < fileSize)
        {
            if (fileSize - total < sizeof(train.buf))
            {
                train.length = fileSize - total;
            }
            else
            {
                train.length = sizeof(train.buf);
            }
            memcpy(train.buf, p + total, train.length);
            total += train.length;
            send(netFd, &train, sizeof(train.length) + train.length, MSG_NOSIGNAL);
        }
        // 发送长度为0代表发送结束
        train.length = 0;
        ret = send(netFd, &train, sizeof(train.length) + train.length, MSG_NOSIGNAL);
        ERROR_CHECK(ret, -1, "send");
    }

    close(fd);
    munmap(p, fileSize);
    int total_time;
    recvn(netFd, &total_time, sizeof(int));
    // printf("total time = %ds\n",total_time);
    return 0;
}

int Download(int sockFd)
{
    // 先获取文件名
    char name[1024] = {0};
    int dataLength;
    recvn(sockFd, &dataLength, sizeof(int));
    if(dataLength == 0)
    {
        printf("Wrong file name!\n");
        return EXIT_FAILURE;
    }
    recvn(sockFd, name, dataLength);
    // printf("name = %s\n", name);
    
    char file_path[1024] = {0};
    sprintf(file_path, "%s%s%s","download", "/", name);
    int fd = open(file_path, O_RDWR | O_CREAT, 0666);// 文件不存在就创建
    ERROR_CHECK(fd, -1, "open");
    char buf[1000] = {0};

    // 获取文件大小
    int fileSize;
    recvn(sockFd, &dataLength, sizeof(int));
    recvn(sockFd, &fileSize, dataLength);

    // 断点重传:
    // 检查本地是否有文件存在，不考虑有不同文件但文件名相同的情况，存在则文件大小不为0
    struct stat statbuf;
    int ret = fstat(fd, &statbuf);
    ERROR_CHECK(ret, -1, "fstat");
    int exitSize = statbuf.st_size;
    send(sockFd, &exitSize, sizeof(exitSize), MSG_NOSIGNAL);
    lseek(fd, 0, SEEK_END);

    time_t timeBeg, timeEnd;

    // 获取文件内容
    // 判断文件大小，是否一次接收
    timeBeg = time(NULL);
    if(fileSize >= 1073741824)// 大于1G
    {
        ftruncate(fd, fileSize); // 预设文件空洞(直接忽略原本文件已经存在的情况)
        char *p = (char *)mmap(NULL, fileSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        ERROR_CHECK(p, MAP_FAILED, "mmap");
        recvn(sockFd, p, fileSize);
        munmap(p, fileSize);
        timeEnd = time(NULL);
    }
    else // 小文件分次传
    {
        while (1)
        {
            bzero(buf, sizeof(buf));
            ret = recv(sockFd, &dataLength, sizeof(int), 0);
            if (dataLength == 0)
            {
                timeEnd = time(NULL);
                break;
            }
            if (ret == 0)// 对面连接断开
            {
                close(fd);
                return EXIT_FAILURE;
            }
            recvn(sockFd, buf, dataLength);
            write(fd, buf, dataLength);
        }
    }
    close(fd);
    int total_time = timeEnd - timeBeg;
    // printf("total time = %ds\n", total_time);
    send(sockFd, &total_time, sizeof(int), MSG_NOSIGNAL);
}