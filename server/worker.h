#ifndef _WORKER_H// 头文件在编译时被多次引用，避免重复定义
#define _WORKER_H
#include<func.h>
#define READ_DATA_SIZE 1024
#define MD5_SIZE 16
#define MD5_STR_LEN (MD5_SIZE * 2)
#define SATL_LEN 14// 盐值长度
#define CLIENT_NUM 50// 客户端表大小
#define TIME_SLICE 300// 时间片大小
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

typedef struct slotNode_s {
    int netFd;
    struct slotNode_s* next;
} slotNode_t;

typedef struct slotList_s {
    slotNode_t* head;
    slotNode_t* tail;
    int size;
} slotList_t;

typedef struct clientQueue_s{// 已连接客户队列
    int client[CLIENT_NUM];// 用于记录每个客户端的user_id
    slotList_t time_out[TIME_SLICE];// 循环队列
    int index[CLIENT_NUM];// 每个netFd当前在队列的位置，下标netFd对应
    int timer;// 记录该查询队列那个下标，初始为0
} clientQueue_t;



// 客户端队列的操作
int init_clientQueue(clientQueue_t *pclientQueue);
int fdAdd(int netFd, clientQueue_t *pclientQueue);
int fdDel(int netFd, clientQueue_t *pclientQueue);

// 用户注册登录
int register_user(train_state_t train_get, int netFd);
int login(train_state_t train_get, int netFd, clientQueue_t *pclientQueue);
int token(train_state_t train_get, int netFd);
int encode(char *key, char *user_name, char *token);// 生成token
int decode(char *key, char *user_name, char *token);// 验证token

// 处理短命令
int handle_short_cmd(train_state_t train_get, int netFd, int user_id);
int recvn(int sockFd, void *pstart, int len);

// 处理长命令
int handle_long_cmd(int netFd, int user_id);
int Upload(int netFd, char *file, int user_id);
int Download(int sockFd, int user_id);

int Compute_file_md5(char *file_path, char *md5_str);// 生成文件的md5码

// 数据库基本连接和执行操作
int mysqlconnect(MYSQL **conn);
int do_query(char *query, MYSQL **conn);

// 操作用户信息表
int find_user_id(char *user_name, int *puser_id);
int get_user_salt(char *user_name, char *salt, char *crptpasswd);
int get_user_name(char *user_name, const int user_id);
int get_user_code(int user_id, int *pcode);
int get_user_pwd(int user_id, char *pwd);
int update_user_pwd(int user_id, char *pwd, int code);

// 操作虚拟文件表
int init_local_file();
int add_link(char *md5_str);// 增加链接数
int minus_link(char *md5_str);// 减少链接数
int get_virtual_file_md5(char *file, int user_id, int parent_id, char *md5_str);// 获取文件md5值，也可以检查文件是否存在，存在返回0，不存在返回1
int get_real_file_name(char *md5_str, char *real_file_name);// 获取文件本地真实名称
int get_file_code(int user_id, char *filename, int *pcode, int parent_id, char *type);
int get_parent_id(int id, int *pparent_code);
int add_local_file(char *md5_str, char *real_name);// 在本地文件表中插入新数据
int add_virtual_dir(int parent_id, char *filename, int owner_id);
int add_virtual_file(int parent_id, char *filename, int owner_id, char *md5_str, int filesize);
int del_virtual_file(int code, int user_id);// 删除虚拟文件/目录
int if_local_exist(char *md5_str);
int find_code_files(int netFd, int code, int user_id);


// 日志记录
int sysadd(int user_id, char *cmd);

#endif