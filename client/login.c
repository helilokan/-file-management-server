#include "threadPool.h"
#include "worker.h"

int register_user(int sockFd)
{
    int check = 0;
    char user_name[100] = {0};
    char passwd[100] = {0};
    train_state_t train_state;
    bzero(&train_state, sizeof(train_state));
    printf("\033[1;37mPlease enter user_name:\033[0m");
    fflush(stdout);
    scanf("%s", user_name);
    // printf("user_name = %s\n", user_name);
    printf("\033[1;37mPlease enter passwd:\033[0m");
    fflush(stdout);
    scanf("%s", passwd);
    // 设置小火车，传输状态信息
    train_state.state = REGISTER;
    strcpy(train_state.buf, user_name);
    printf("train.buf = %s\n", train_state.buf);
    train_state.length = strlen(train_state.buf);
    train_state.pre_length = 0;
    send(sockFd, &train_state, sizeof(train_state.length) + sizeof(train_state.state) + sizeof(train_state.pre_length) + train_state.length, 0);
    
    // 接收盐值发送密文
    char crptpasswd[50] = {0};
    char salt[SATL_LEN + 1] = {0};

    // 接收盐值
    int datalength;
    recvn(sockFd, &datalength, sizeof(datalength));
    recvn(sockFd, salt, datalength);

    // 计算出密文并发送
    strncpy(crptpasswd, crypt(passwd, salt), 49);// 密文!!只存49位
    train_t train;
    train.length = strlen(crptpasswd);
    strcpy(train.buf, crptpasswd);
    send(sockFd, &train, sizeof(train.length) + train.length, MSG_NOSIGNAL);
    
    recvn(sockFd, &check, sizeof(check));// 获取注册结果
    return check;
}

int login(int sockFd, char *uName)
{
    int check = 0;
    char user_name[100] = {0};
    // 读取用户名
    printf("\033[1;37mPlease enter user_name:\033[0m");
    fflush(stdout);
    scanf("%s", user_name);
    train_state_t train_state;
    // 读取密码
    while(1)
    {
        check = 0;
        bzero(&train_state, sizeof(train_state));
        char *passwd = getpass("\033[1;37mPlease enter your password:\033[0m");
        // 设置小火车，传输状态信息和用户名
        train_state.state = LOGIN;
        strcpy(train_state.buf, user_name);
        train_state.length = strlen(train_state.buf);
        train_state.pre_length = 0;
        send(sockFd, &train_state, sizeof(train_state.length) + sizeof(train_state.state) + sizeof(train_state.pre_length) + train_state.length, 0);
        // 先接收看用户是否存在
        recvn(sockFd, &check, sizeof(check));
        if(check == 2)// 用户未注册
        {
            break;
        }

        // 用户已注册接收盐值
        char crptpasswd[50] = {0};
        char salt[SATL_LEN + 1] = {0};
        int datalength;
        recvn(sockFd, &datalength, sizeof(datalength));
        recvn(sockFd, salt, datalength);
        // 计算密文并发送
        strncpy(crptpasswd, crypt(passwd, salt), 49);
        printf("crptpasswd = %s\n", crptpasswd);
        train_t train;
        bzero(&train, sizeof(train));
        train.length = strlen(crptpasswd);
        strcpy(train.buf, crptpasswd);
        send(sockFd, &train, sizeof(train.length) + train.length, MSG_NOSIGNAL);

        // 接收验证结果
        recvn(sockFd, &check, sizeof(check));
        printf("check = %d\n", check);
        if(check == 0)// 密码验证成功
        {
            strcpy(uName, user_name);
            break;
        }
        else
        {
            printf("\033[1;37mPassword is not right!\n\033[0m");
        }
    }
    return check;
}