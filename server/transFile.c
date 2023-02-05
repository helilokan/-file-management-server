#include"threadPool.h"
#include"worker.h"
// TCP是一种字节流传输，信息无边界，对方接收时以最大字节数为界限
// 利用小火车协议，先传输文件大小，固定的int类型，4个字节，再传输文件内容
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

int Upload(int netFd, char *file, int user_id)
{
    // 打开文件之前需要先获取文件在本地的真是名称
    int code;
    get_user_code(user_id, &code);
    char md5_str[MD5_STR_LEN + 1] = {0};
    get_virtual_file_md5(file, user_id, code, md5_str);// 根据文件名、user_id和用户当前目录id，在虚拟文件表中找到md5值
    char real_file_name[1024] = {0};

    get_real_file_name(md5_str, real_file_name);// 根据文件名md5值获取本地的真实文件名
    char path[1024] = {0};
    sprintf(path, "%s%s%s", "NetDisk", "/", real_file_name);// 拼出文件本地地址
    int fd = open(path, O_RDWR);// 打开本地文件
    // printf("path = %s\n", path);
    train_t train;
    if(fd == -1)// 打开文件错误需要通知对面
    {
        perror("open");
        bzero(&train, sizeof(train));
        train.length = 0;
        send(netFd, &train, sizeof(train.length) + train.length, 0);
    }
    // 发送文件名(虚拟文件表中存的文件名，客户端下载到本地也用这个名字)；
    // 后序与普通下载文件没有区别，客户端本地下载不存在秒传，默认下载的文件本地没有，且客户端本地的下载做了检查，文件存在则会将指针偏移到文件结尾
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

    // 获得文件已传大小
    int exitSize = 0;
    recvn(netFd, &exitSize, sizeof(exitSize));

    // 发送文件内容
    // 判断文件大小，大文件用一次性传输
    char *p = (char*)mmap(NULL, fileSize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    int total = exitSize;
    printf("exitSize = %d\n", exitSize);
    printf("fileSize = %d\n", fileSize);
    if(fileSize >= 1073741824)// 大于1G
    {
        send(netFd, p, fileSize, MSG_NOSIGNAL);
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
    int total_time;
    recvn(netFd, &total_time, sizeof(int));
    printf("total time = %ds\n",total_time);
    munmap(p, fileSize);
    return 0;
}

int Download(int sockFd, int user_id)
{
    // 先获取文件名，不同客户端传的文件可能文件名相同但文件内容不同，所以根据md5值来判断本地是否有该文件
    char name[1024] = {0};
    int dataLength;
    recvn(sockFd, &dataLength, sizeof(int));// 文件内容还够时一直将length长度的文件都收完
    if(dataLength == 0)
    {
        return EXIT_FAILURE;// 打开文件错误退出
    }
    recvn(sockFd, name, dataLength);
    printf("name = %s\n", name);
    
    // 获取文件大小
    int fileSize;
    recvn(sockFd, &dataLength, sizeof(int));
    recvn(sockFd, &fileSize, dataLength);

    // 获取文件md5值
    char md5_str[MD5_STR_LEN + 1] = {0};
    recvn(sockFd, &dataLength, sizeof(int));
    recvn(sockFd, md5_str, dataLength);

    // 获取当前目录id
    int code;
    get_user_code(user_id, &code);

    // 确认虚拟文件表中是否存在，存在就不再插入表中
    int ret1 = get_virtual_file_md5(name, user_id, code, md5_str);// 获取成功文件存在返回0，获取失败文件不存在返回1
    if(ret1 != 0)
    {
        add_virtual_file(code, name, user_id, md5_str, fileSize);// 文件不存在，在虚拟文件表中插入
    }

    time_t timeBeg, timeEnd;
    timeBeg = time(NULL);// 从检查文件是否存在开始计时

    // 检查文件在服务器本地是否存在，因为可能出现插入了虚拟文件表中在服务器本地却被删除了的情况
    int check = 0;
    int ret = if_local_exist(md5_str);
    char path[1024] = {0};
    char real_name[1024] = {0};
    int exitSize = 0;
    char buf[1000] = {0};
    int fd;
    // printf("ret = %d\n", ret);
    if(ret == 0)// 文件在本地存在
    {
        // 1. 获取文件本地真名
        get_real_file_name(md5_str, real_name);
        sprintf(path, "%s%s%s", "NetDisk", "/", real_name);
        // 2. 打开文件
        fd = open(path, O_RDWR | O_CREAT, 0666); // 文件在本地存在，考虑误删了本地文件但表里的信息未更新
        ERROR_CHECK(fd, -1, "open");
        // 3. 获取文件大小已有大小
        struct stat statbuf;
        ret = fstat(fd, &statbuf);
        ERROR_CHECK(ret, -1, "fstat");
        exitSize = statbuf.st_size;
        // 4. 判断是否需要断点续传
        if(exitSize < fileSize)// 需要上传，断点续传
        {
            check = 1;
            send(sockFd, &check, sizeof(int), MSG_NOSIGNAL);// 通知客户端文件需要上传
        }
        else// 不需要上传，秒传
        {
            send(sockFd, &check, sizeof(int), MSG_NOSIGNAL); // 通知客户端文件存在不需要上传
            // 将本地文件表中相应文件的link数加1
            if (ret1 != 0) // 文件原先在虚拟文件表中不存在，链接数加1
            {
                add_link(md5_str);
            }
            timeEnd = time(NULL);
            int total_time = timeEnd - timeBeg;
            printf("total time = %ds\n", total_time);
            send(sockFd, &total_time, sizeof(int), MSG_NOSIGNAL);
            return 0;
        }
    }
    else// 文件在本地不存在
    {
        check = 1;
        send(sockFd, &check, sizeof(int), MSG_NOSIGNAL); // 通知客户端文件需要上传
        // 文件不存在，需要下载到本地，且添加到本地文件表中
        // 1. 拼一个本地文件名
        // 拼接出一个本地文件名，用文件名、user_id和当前目录id，这样不同用户的同名文件，或同一用户不同文件夹中的同名文件，就不会在本地重名了
        // 这里的同名指文件名相同但文件内容可能不同
        bzero(path, sizeof(path));
        sprintf(real_name, "%s%d%d", name, user_id, code);
        // 2. 添加到本地文件表中
        add_local_file(md5_str, real_name);
        // 3. 拼出一个文件路径
        sprintf(path, "%s%s%s", "NetDisk", "/", real_name);
        fd = open(path, O_RDWR | O_CREAT, 0666); // 文件不存在就创建
        ERROR_CHECK(fd, -1, "open");
        
        // 4. 获取文件大小，这里肯定不存在，只是为了客户端不阻塞
        struct stat statbuf;
        ret = fstat(fd, &statbuf);
        ERROR_CHECK(ret, -1, "fstat");
        exitSize = statbuf.st_size;
    }
    // 发送本地文件大小
    send(sockFd, &exitSize, sizeof(exitSize), MSG_NOSIGNAL);
    lseek(fd, 0, SEEK_END);

    // 下载文件
    // 判断文件大小，是否一次接收
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
            if (dataLength == 0 )// 收到0代表传输结束
            {
                timeEnd = time(NULL);
                break;
            }
            if(ret == 0)// ret为0说明对面提前断开了
            {
                close(fd);
                timeEnd = time(NULL);
                int total_time = timeEnd - timeBeg;
                printf("total time = %ds\n", total_time);
                return EXIT_FAILURE;
            }
            recvn(sockFd, buf, dataLength);
            write(fd, buf, dataLength);
        }
    }
    close(fd);
    int total_time = timeEnd - timeBeg;
    printf("total time = %ds\n", total_time);
    send(sockFd, &total_time, sizeof(int), MSG_NOSIGNAL);
}