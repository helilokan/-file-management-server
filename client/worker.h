#ifndef _WORKER_H// 头文件在编译时被多次引用，避免重复定义
#define _WORKER_H
#include<func.h>
#include "threadPool.h"
#define READ_DATA_SIZE 1024
#define MD5_SIZE 16
#define MD5_STR_LEN (MD5_SIZE * 2)
#define SATL_LEN 14

// 传输文件用的小火车
typedef struct tain_s{
    int length;
    char buf[1000];
} train_t;

// 传输命令用的小火车
typedef struct tain_state_s{
    int length;
    int state;
    int pre_length;// 如果是注册和登录，用于记录用户名长度，如果是命令，用于记录命令部分长度
    char buf[1000];
} train_state_t;

// 命令枚举
enum command{
    REGISTER,
    LOGIN,
    LS,
    CD,
    PWD,
    REMOVE,
    MKDIR,
    GETS,
    PUTS,
    TOKEN,
};

int handleCmd(train_state_t train_get, int netFd);
int recvn(int sockFd, void *pstart, int len);
int Upload(int netFd, char *file);
int Download(int sockFd);

int register_user(int sockFd);
int login(int sockFd, char *uName);
int token(int sockFd, char *token);
int encode(char *key, char *user_name, char *token);// 重新生成token值

int Compute_file_md5(char *file_path, char *md5_str);

int recvn(int sockFd, void *pstart, int len);
int print_stat(struct stat statbuf, char *name);
int Upload(int netFd, char *file);
int Download(int sockFd);
#endif