#include "threadPool.h"
#include "worker.h"
int make_salt(char *salt)
{
    int i , flag;
    srand(time(NULL));
    salt[0] = '$';
    salt[1] = '6';
    salt[2] = '$';
    for(i = 3; i < SATL_LEN; ++i)
    {
        flag = rand()%3; switch(flag)
        {
        case 0:
            salt[i] = rand()%26 + 'a';
            break; 
        case 1:
            salt[i] = rand()%26 + 'A';
            break; 
        case 2:
            salt[i] = rand()%10 + '0';
            break;
        }
    }
    return 0;
}

int register_user(train_state_t train_get, int netFd)
{
    char user_name[100] = {0};// username!!
    char passwd[100] = {0};
    int check = 0;
    int user_id;
    // 读取传入的用户名
    strncpy(user_name, train_get.buf, train_get.length);
    printf("user_name = %s\n", user_name);
    strcpy(passwd, train_get.buf + train_get.pre_length);

    int ret = find_user_id(user_name, &user_id);
    if (ret == 0) // 如果这里查找成功，说明用户已存在，注册失败(用户名不可重复)
    {
        check = 2;
    }

    // 发送盐值接收密文
    train_t train;
    char salt[SATL_LEN + 1] = {0};// 留一位存结束符
    make_salt(salt);// salt!!
    // 发送盐值
    train.length = strlen(salt);
    strcpy(train.buf, salt);
    send(netFd, &train, sizeof(train.length) + train.length, MSG_NOSIGNAL);

    // 收取密文
    int datalength;
    char crptpasswd[50] = {0};
    recvn(netFd, &datalength, sizeof(datalength));
    recvn(netFd, crptpasswd, datalength);

    char pwd[100] = "~/";// pwd!!

    if(check == 2)// 这里在密文盐值传输之后判断，防止阻塞
    {
        send(netFd, &check, sizeof(int), MSG_NOSIGNAL);
        return 0;
    }

    // 注册成功，插入数据库
    MYSQL *conn = NULL;

    ret = mysqlconnect(&conn);
    if(ret != 0)
    {
        check = 1;
    }
    ret = mysql_query(conn, "set names 'utf8'");
    if(ret != 0)
    {
        printf("error query1:%s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }

    char query[1024] = {0};
    sprintf(query, "insert into client_info(username, salt, cryptpasswd, pwd, code) \
                    values('%s', '%s', '%s', '%s', 0)", user_name, salt, crptpasswd, pwd);
    ret = do_query(query, &conn);// 插入注册信息
    if(ret != 0)
    {
        check = 1;
    }
    mysql_close(conn);

    // 把注册操作写入日记
    if(check == 0)
    { 
        find_user_id(user_name, &user_id);
        sysadd(user_id, "registed!");
    }
    send(netFd, &check, sizeof(int), MSG_NOSIGNAL);// 确认是否注册成功
}

int login(train_state_t train_get, int netFd, clientQueue_t *pclientQueue)
{
    // 初始化需要用到的信息
    char user_name[100] = {0};// username!!
    char token[1024] = {0};
    char key[100] = {0};
    int user_id = 0;
    int check = 0;// 用于确认登录是否正确

    // 从传入的信息中读取用户名和密码
    strncpy(user_name, train_get.buf, train_get.length);
    
    // 查找用户，获取盐值和密文
    char crptpasswd[50] = {0};
    char salt[SATL_LEN + 1] = {0};
    int ret = get_user_salt(user_name, salt, crptpasswd);
    if(ret == 0)// 查找成功
    {
        // 确认用户存在
        check = 0;
        send(netFd, &check, sizeof(check), MSG_NOSIGNAL);

        // 发送盐值
        train_t train;
        train.length = strlen(salt);
        strcpy(train.buf, salt);
        send(netFd, &train, sizeof(train.length) + train.length, MSG_NOSIGNAL);

        // 接收客户端生成的密文
        int datalength;
        char crptpasswd_client[50] = {0};
        recvn(netFd, &datalength, sizeof(int));
        recvn(netFd, crptpasswd_client, datalength);

        if(strncmp(crptpasswd, crptpasswd_client, 49) != 0)// 只比较前49位
        {
            check = 1;// 密码错误
        }
        // 发送验证结果
        send(netFd, &check, sizeof(check), MSG_NOSIGNAL);
        if(check == 1)// 只比较前49位
        {
            return EXIT_FAILURE;// 密码错误
        }
        else
        {
            printf("right\n");
            // 密码正确，用户连接成功，加入连接队列
            find_user_id(user_name, &user_id);
            sprintf(key, "YoUR sUpEr S3krEt %d HMAC kEy HeRE", user_id);// 每个用户的key值不同
            pclientQueue->client[netFd] = user_id;// 插入用户队列，下标为netFd
            sysadd(user_id, "login successful!");// 记入日志
            // 获取token值(未判断获取失败的情况)
            encode(key, user_name, token);
            // printf("user_name = %s\n", user_name);
            // printf("token = %s\n",token);
            // printf("key = %s\n", key);
        }
    }
    else
    {
        check = 2;// 查找失败，用户未注册
        send(netFd, &check, sizeof(int), MSG_NOSIGNAL);
    }

    if(check == 0)// 登录成功，发送token值，key值，发送当前目录
    {
        train_t train;

        train.length = strlen(token);
        strcpy(train.buf, token);
        send(netFd, &train, sizeof(train.length) + train.length, MSG_NOSIGNAL);

        train.length = strlen(key);
        strcpy(train.buf, key);
        send(netFd, &train, sizeof(train.length) + train.length, MSG_NOSIGNAL);

        char pwd[100] = "~/";// 刚登陆都在家目录
        train.length = strlen(pwd);
        bzero(train.buf, sizeof(train.buf));
        strcpy(train.buf, pwd);
        send(netFd, &train, sizeof(train.length) + train.length, MSG_NOSIGNAL);
    }
    return check;
}

int token(train_state_t train_get, int netFd)// 检查token值
{
    int check = 1;
    int user_id = -1;
    char token[1024] = {0};
    char key[100] = {0};
    char user_name[100] = {0};
    strncpy(user_name, train_get.buf, train_get.pre_length);
    // printf("user_name = %s\n", user_name);

    strcpy(token, train_get.buf + train_get.pre_length);
    // printf("token = %s\n",token);
    
    find_user_id(user_name, &user_id);
    sprintf(key, "YoUR sUpEr S3krEt %d HMAC kEy HeRE", user_id);
    // printf("key = %s\n", key);

    int ret = decode(key, user_name, token);
    if(ret == 0)
    {
        printf("right!\n");
        check = 0;
    }
    else// 不正确的话user_id变为-1
    {
        user_id = -1;
    }
    send(netFd, &check, sizeof(int), MSG_NOSIGNAL);// 确认是否登录成功
    return user_id;
}