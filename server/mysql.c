#include "threadPool.h"
#include "worker.h"

int mysqlconnect(MYSQL **pconn)
{
    char *host = "localhost";
    char *user = "root";
    char *passwd = "1234";
    char *db = "net_disk";

    *pconn = mysql_init(NULL);
    if(mysql_real_connect(*pconn, host, user, passwd, db, 0, NULL, 0) == NULL)
    {
        printf("error:%s\n", mysql_error(*pconn));
        return EXIT_FAILURE;
    }
    return 0;
}

int do_query(char *query, MYSQL **pconn)
{
    int ret = mysql_query(*pconn, query);
    if(ret != 0)
    {
        printf("error query: %s\n", mysql_error(*pconn));
        return EXIT_FAILURE;
    }
    return 0;
}

// 根据用户名获取盐值和密文
int get_user_salt(char *user_name, char *salt, char *crptpasswd)
{
    MYSQL *conn;
    int ret = mysqlconnect(&conn);
    if(ret != 0)
    {
        return EXIT_FAILURE;
    }
    ret = mysql_query(conn, "set names 'utf8'");
    if(ret != 0)
    {
        printf("error query1:%s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }
    ret = do_query("select username, salt, cryptpasswd from client_info", &conn);
    if(ret != 0)
    {
        return EXIT_FAILURE;
    }
    MYSQL_RES *result = mysql_use_result(conn);
    if(result == NULL)
    {
        printf("error result: %s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }
    MYSQL_ROW row;
    // printf("user_name = %s\n", user_name);
    while((row = mysql_fetch_row(result)) != NULL)
    {
        // printf("row[0] = %s\n", row[0]);
        if(strcmp(row[0], user_name) == 0)
        {
            strcpy(salt, row[1]);
            strcpy(crptpasswd, row[2]);
            mysql_free_result(result);
            mysql_close(conn);
            return 0;
        }
    }
    mysql_free_result(result);
    mysql_close(conn);
    return EXIT_FAILURE;
}

// 根据用户名获取用户id
int find_user_id(char *user_name, int *puser_id)
{
    MYSQL *conn;
    int ret = mysqlconnect(&conn);
    if(ret != 0)
    {
        return EXIT_FAILURE;
    }
    ret = mysql_query(conn, "set names 'utf8'");
    if(ret != 0)
    {
        printf("error query1:%s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }
    ret = do_query("select username, id from client_info", &conn);
    if(ret != 0)
    {
        return EXIT_FAILURE;
    }
    MYSQL_RES *result = mysql_use_result(conn);
    if(result == NULL)
    {
        printf("error result: %s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }
    MYSQL_ROW row;
    while((row = mysql_fetch_row(result)) != NULL)
    {
        if(strcmp(row[0], user_name) == 0)
        {
            *puser_id = atoi(row[1]);

            mysql_free_result(result);
            mysql_close(conn);
            return 0;
        }
    }
    mysql_free_result(result);
    mysql_close(conn);
    return EXIT_FAILURE;
}

// 根据用户id获取用户名
int get_user_name(char *user_name, const int user_id)
{
    MYSQL *conn;
    int ret = mysqlconnect(&conn);
    if(ret != 0)
    {
        return EXIT_FAILURE;
    }
    ret = mysql_query(conn, "set names 'utf8'");
    if(ret != 0)
    {
        printf("error query1:%s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }
    ret = do_query("select username, id from client_info", &conn);
    if(ret != 0)
    {
        return EXIT_FAILURE;
    }
    MYSQL_RES *result = mysql_use_result(conn);
    if(result == NULL)
    {
        printf("error result: %s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }
    MYSQL_ROW row;
    while((row = mysql_fetch_row(result)) != NULL)
    {
        if(user_id == atoi(row[1]))
        {
            strcpy(user_name, row[0]);

            mysql_free_result(result);
            mysql_close(conn);
            return 0;
        }
    }
    mysql_free_result(result);
    mysql_close(conn);
    return EXIT_FAILURE;
}

// 通过user_id获取当前目录的文件夹编号
int get_user_code(int user_id, int *pcode)
{
    MYSQL *conn;
    int ret = mysqlconnect(&conn);
    if(ret != 0)
    {
        return EXIT_FAILURE;
    }
    ret = do_query("select code, id from client_info", &conn);
    if(ret != 0)
    {
        return EXIT_FAILURE;
    }
    MYSQL_RES *result = mysql_use_result(conn);
    if(result == NULL)
    {
        printf("error result: %s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }
    MYSQL_ROW row;
    while((row = mysql_fetch_row(result)) != NULL)
    {
        if(user_id == atoi(row[1]))
        {
            *pcode = atoi(row[0]);

            mysql_free_result(result);
            mysql_close(conn);
            return 0;
        }
    }
    mysql_free_result(result);
    mysql_close(conn);
    return EXIT_FAILURE;
}

// 通过user_id获取当前目录的文件夹编号
int get_user_pwd(int user_id, char *pwd)
{
    MYSQL *conn;
    int ret = mysqlconnect(&conn);
    if(ret != 0)
    {
        return EXIT_FAILURE;
    }
    ret = mysql_query(conn, "set names 'utf8'");
    if(ret != 0)
    {
        printf("error query1:%s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }
    ret = do_query("select pwd, id from client_info", &conn);
    if(ret != 0)
    {
        return EXIT_FAILURE;
    }
    MYSQL_RES *result = mysql_use_result(conn);
    if(result == NULL)
    {
        printf("error result: %s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }
    MYSQL_ROW row;
    while((row = mysql_fetch_row(result)) != NULL)
    {
        if(user_id == atoi(row[1]))
        {
            strcpy(pwd, row[0]);

            mysql_free_result(result);
            mysql_close(conn);
            return 0;
        }
    }
    mysql_free_result(result);
    mysql_close(conn);
    return EXIT_FAILURE;
}

// 通过user_id更新用户当前目录
int update_user_pwd(int user_id, char *pwd, int code)
{
    MYSQL *conn;
    int ret = mysqlconnect(&conn);
    if(ret != 0)
    {
        return EXIT_FAILURE;
    }
    ret = mysql_query(conn, "set names 'utf8'");
    if(ret != 0)
    {
        printf("error query1:%s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }
    char query[1024] = {0};
    sprintf(query, "update client_info set pwd = '%s' where id = %d", pwd, user_id);
    ret = do_query(query, &conn);
    if(ret != 0)
    {
        return EXIT_FAILURE;
    }
    bzero(query, sizeof(query));
    sprintf(query, "update client_info set code = %d where id = %d", code, user_id);
    ret = do_query(query, &conn);
    if(ret != 0)
    {
        return EXIT_FAILURE;
    }

    mysql_close(conn);
    return EXIT_FAILURE;
}

// 初始化本地文件表
int init_local_file()
{
    // 连接数据库
    MYSQL *conn;
    int ret = mysqlconnect(&conn);
    if(ret != 0)
    {
        return EXIT_FAILURE;
    }
    ret = mysql_query(conn, "set names 'utf8'");
    if(ret != 0)
    {
        printf("error query1:%s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }
    char query[1024] = {0};

    // 读取目录流，获取本地存储目录中的每个文件
    DIR *dirp = opendir("NetDisk");
    ERROR_CHECK(dirp, NULL, "opendir");
    struct dirent *pdirent;
    char md5_str[MD5_STR_LEN + 1] = {0};// 这里长度需要加1，存结束符
    
    while ((pdirent = readdir(dirp)) != NULL)
    {
        // 跳过隐藏文件
        if (*(pdirent->d_name) == '.')
        {
            continue;
        }
        char real_name[100] = {0};
        strcpy(real_name, pdirent->d_name);
        sprintf(query, "select * from local_files where real_file_name like '%s'", real_name);
        ret = do_query(query, &conn);
        if (ret != 0)
        {
            return EXIT_FAILURE;
        }
        MYSQL_RES *result = mysql_store_result(conn); // 这里如果用use不会真正获得数据，rows仍为0
        if (ret != 0)
        {
            printf("error query:%s\n", mysql_error(conn));
            return EXIT_FAILURE;
        }
        int rows = mysql_num_rows(result);
        if (rows != 0) // 文件名存在就继续读
        {
            mysql_free_result(result);
            continue;
        }
        mysql_free_result(result);// 文件名不存在则计算md5值，插入本地

        char path[1024] = {0};
        sprintf(path, "%s%s%s", "NetDisk", "/", pdirent->d_name);
        bzero(md5_str, sizeof(md5_str));
        Compute_file_md5(path, md5_str);
        bzero(query, sizeof(query));
        sprintf(query, "insert into local_files(md5, link_num, real_file_name) values('%s', 0, '%s')", md5_str, pdirent->d_name);
        ret = do_query(query, &conn);
        if (ret != 0)
        {
            return EXIT_FAILURE;
        }
    }
    mysql_close(conn);
    return 0;
}

// 查找本地是否已经有文件存在
int if_local_exist(char *md5_str)
{
    MYSQL *conn;
    int ret = mysqlconnect(&conn);
    if(ret != 0)
    {
        return EXIT_FAILURE;
    }
    ret = mysql_query(conn, "set names 'utf8'");
    if(ret != 0)
    {
        printf("error query1:%s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }
    char query[1024] = {0};

    // 查询文件是否已在表中存在
    bzero(query, sizeof(query));
    sprintf(query, "select * from local_files where md5 like '%s'", md5_str);
    ret = do_query(query, &conn);
    if (ret != 0)
    {
        return EXIT_FAILURE;
    }
    MYSQL_RES *result = mysql_store_result(conn);// 这里如果用use不会真正获得数据，rows仍为0
    if (ret != 0)
    {
        printf("error query:%s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }
    int rows = mysql_num_rows(result);
    // printf("rows = %d\n", rows);
    if(rows == 0)// 文件不存在返回1
    {
        mysql_free_result(result);
        return 1;
    }
    mysql_free_result(result);
    mysql_close(conn);
    return 0;// 文件存在返回0
}

// 增加本地文件的链接数
int add_link(char *md5_str)
{
    MYSQL *conn;
    int ret = mysqlconnect(&conn);
    int link_num;
    if(ret != 0)
    {
        return EXIT_FAILURE;
    }
    char query[1024] = {0};

    // 获得当前链接数
    bzero(query, sizeof(query));
    sprintf(query, "select link_num from local_files where md5 like '%s'", md5_str);
    ret = do_query(query, &conn);
    if (ret != 0)
    {
        return EXIT_FAILURE;
    }
    MYSQL_RES *result = mysql_use_result(conn);
    if (ret != 0)
    {
        printf("error query:%s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }
    MYSQL_ROW row;
    if((row = mysql_fetch_row(result)) == NULL)
    {
        printf("error query:%s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }
    link_num = atoi(row[0]);// 获得目前的链接数

    mysql_free_result(result);// 查询完要先释放查询结果，否则无法更新表格
    ++link_num;
    bzero(query, sizeof(query));
    sprintf(query, "update local_files set link_num = %d where md5 like '%s'", link_num, md5_str);
    // printf("query = %s\n", query);

    ret = do_query(query, &conn);
    if (ret != 0)
    {
        return EXIT_FAILURE;
    }
    
    mysql_close(conn);
    return 0;
}

// 减少本地文件的链接数
int minus_link(char *md5_str)
{
    MYSQL *conn;
    int ret = mysqlconnect(&conn);
    int link_num;
    if(ret != 0)
    {
        return EXIT_FAILURE;
    }
    char query[1024] = {0};

    // 查询文件是否已在表中存在
    bzero(query, sizeof(query));
    sprintf(query, "select link_num from local_files where md5 like '%s'", md5_str);
    ret = do_query(query, &conn);
    if (ret != 0)
    {
        return EXIT_FAILURE;
    }
    MYSQL_RES *result = mysql_use_result(conn);
    if (ret != 0)
    {
        printf("error query:%s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }
    MYSQL_ROW row;
    if((row = mysql_fetch_row(result)) == NULL)
    {
        printf("error query:%s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }
    link_num = atoi(row[0]);// 获得目前的链接数
    // printf("link_num = %d\n", link_num);

    mysql_free_result(result);// 查询完要先释放查询结果，否则无法更新表格
    --link_num;
    if(link_num < 0)
    {
        printf("error minus link_num!\n");
        return EXIT_FAILURE;
    }
    bzero(query, sizeof(query));
    sprintf(query, "update local_files set link_num = %d where md5 like '%s'", link_num, md5_str);
    ret = do_query(query, &conn);
    if (ret != 0)
    {
        return EXIT_FAILURE;
    }
    
    mysql_close(conn);
    return 0;
}

// 在本地文件表中新增内容
int add_local_file(char *md5_str, char *real_name)
{
    MYSQL *conn;
    int ret = mysqlconnect(&conn);
    int link_num;
    if (ret != 0)
    {
        return EXIT_FAILURE;
    }
    ret = mysql_query(conn, "set names 'utf8'");
    if(ret != 0)
    {
        printf("error query1:%s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }
    char query[1024] = {0};

    bzero(query, sizeof(query));
    sprintf(query, "insert into local_files(md5, link_num, real_file_name) values('%s', 1, '%s')", md5_str, real_name);
    ret = do_query(query, &conn);
    if (ret != 0)
    {
        return EXIT_FAILURE;
    }
    mysql_close(conn);
    return 0;
}

// 获取文件本地真实名称
int get_real_file_name(char *md5_str, char *real_file_name)
{
    MYSQL *conn;
    int ret = mysqlconnect(&conn);
    if(ret != 0)
    {
        return EXIT_FAILURE;
    }
    ret = mysql_query(conn, "set names 'utf8'");
    if(ret != 0)
    {
        printf("error query1:%s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }
    char query[1024] = {0};

    // 按照md5值在表中查找本地文件名
    bzero(query, sizeof(query));
    sprintf(query, "select real_file_name from local_files where md5 like '%s'", md5_str);
    ret = do_query(query, &conn);
    if (ret != 0)
    {
        return EXIT_FAILURE;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if(result == NULL)
    {
        printf("error result: %s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }
    int rows = mysql_num_rows(result);
    
    if(rows == 0)// 文件不存在返回1
    {
        mysql_free_result(result);
        mysql_close(conn);
        return 1;
    }
    MYSQL_ROW row;
    if((row = mysql_fetch_row(result)) == NULL)
    {
        printf("error query:%s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }
    strcpy(real_file_name, row[0]);
    mysql_free_result(result);
    mysql_close(conn);
    return 0;
}

// 根据文件名和用户id获取文件在虚拟文件系统中的id，parent_id, 类型
int get_file_code(int user_id, char *filename, int *pcode, int parent_id, char *type)
{
    MYSQL *conn;
    int ret = mysqlconnect(&conn);
    if(ret != 0)
    {
        return EXIT_FAILURE;
    }
    ret = mysql_query(conn, "set names 'utf8'");
    if(ret != 0)
    {
        printf("error query1:%s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }
    char query[1024] = {0};
    sprintf(query, "select id, parent_id, type from virtual_file where owner_id = %d and filename like '%s' and parent_id = %d", user_id, filename, parent_id);
    ret = do_query(query, &conn);
    if(ret != 0)
    {
        return EXIT_FAILURE;
    }
    MYSQL_RES *result = mysql_use_result(conn);
    if(result == NULL)
    {
        printf("error result: %s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }

    MYSQL_ROW row;
    if((row = mysql_fetch_row(result)) == NULL)// 不存在直接返回
    {
        return EXIT_FAILURE;
    }
    *pcode = atoi(row[0]);
    strcpy(type, row[2]);
    mysql_free_result(result);
    mysql_close(conn);
    return 0;
}

// 增加虚拟文件夹
int add_virtual_dir(int parent_id, char *filename, int owner_id)
{
    MYSQL *conn;
    int ret = mysqlconnect(&conn);
    if(ret != 0)
    {
        return EXIT_FAILURE;
    }
    ret = mysql_query(conn, "set names 'utf8'");
    if(ret != 0)
    {
        printf("error query1:%s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }
    char query[1024] = {0};
    sprintf(query, "insert into virtual_file(parent_id, filename, owner_id, md5, filesize, type) \
            values(%d, '%s', %d, '0', 0, 'd')", parent_id, filename, owner_id);
    ret = do_query(query, &conn);
    if(ret != 0)
    {
        return EXIT_FAILURE;
    }
    mysql_close(conn);
    return 0;
}

// 增加虚拟文件
int add_virtual_file(int parent_id, char *filename, int owner_id, char *md5_str, int filesize)
{
    MYSQL *conn;
    int ret = mysqlconnect(&conn);
    if(ret != 0)
    {
        return EXIT_FAILURE;
    }
    ret = mysql_query(conn, "set names 'utf8'");
    if(ret != 0)
    {
        printf("error query1:%s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }
    char query[1024] = {0};
    sprintf(query, "insert into virtual_file(parent_id, filename, owner_id, md5, filesize, type) \
            values(%d, '%s', %d, '%s', %d, 'f')", parent_id, filename, owner_id, md5_str, filesize);
    ret = do_query(query, &conn);
    if(ret != 0)
    {
        return EXIT_FAILURE;
    }
    mysql_close(conn);
    return 0;
}

// ls功能的实现，把文件id为code的文件夹下的文件全部发给客户端
int find_code_files(int netFd, int code, int user_id)
{
    MYSQL *conn;
    int ret = mysqlconnect(&conn);
    if(ret != 0)
    {
        return EXIT_FAILURE;
    }
    ret = mysql_query(conn, "set names 'utf8'");
    if(ret != 0)
    {
        printf("error query1:%s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }
    char query[1024] = {0};
    sprintf(query, "select filename from virtual_file where parent_id = %d and owner_id = %d", code, user_id);
    ret = do_query(query, &conn);
    if(ret != 0)
    {
        return EXIT_FAILURE;
    }
    MYSQL_RES *result = mysql_use_result(conn);
    if(result == NULL)
    {
        printf("error result: %s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }
    MYSQL_ROW row;
    train_t train;
    while((row = mysql_fetch_row(result)) != NULL)
    {
        train.length = strlen(row[0]);
        bzero(train.buf, sizeof(train.buf));
        strcpy(train.buf, row[0]);
        send(netFd, &train, sizeof(train.length) + train.length, MSG_NOSIGNAL);
    }
    // 发送长度0表示结束
    train.length = 0;
    send(netFd, &train, sizeof(train.length) + train.length, MSG_NOSIGNAL);
    mysql_free_result(result);
    mysql_close(conn);
    return EXIT_FAILURE;
}

// 获取文件md5值，也可以检查文件是否存在，存在返回0，不存在返回1
int get_virtual_file_md5(char *file, int user_id, int parent_id, char *md5_str)
{
    MYSQL *conn;
    int ret = mysqlconnect(&conn);
    if(ret != 0)
    {
        return EXIT_FAILURE;
    }
    ret = mysql_query(conn, "set names 'utf8'");
    if(ret != 0)
    {
        printf("error query1:%s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }
    char query[1024] = {0};
    sprintf(query, "select md5 from virtual_file where owner_id = %d and filename like '%s' and parent_id = %d", user_id, file, parent_id);
    ret = do_query(query, &conn);
    if(ret != 0)
    {
        return EXIT_FAILURE;
    }
    MYSQL_RES *result = mysql_use_result(conn);
    if(result == NULL)
    {
        printf("error result: %s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }
    
    MYSQL_ROW row;
    
    if((row = mysql_fetch_row(result)) == NULL)// 不存在则直接返回
    {
        return EXIT_FAILURE;
    }
    strcpy(md5_str, row[0]);
    mysql_free_result(result);
    mysql_close(conn);
    return 0;
}

// 删除文件
int del_virtual_file(int code, int user_id)
{
    MYSQL *conn;
    int ret = mysqlconnect(&conn);
    if(ret != 0)
    {
        return EXIT_FAILURE;
    }
    ret = mysql_query(conn, "set names 'utf8'");
    if(ret != 0)
    {
        printf("error query1:%s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }
    char query[1024] = {0};
    sprintf(query, "delete from virtual_file where id = %d", code);
    ret = do_query(query, &conn);
    if(ret != 0)
    {
        return EXIT_FAILURE;
    }

    bzero(query, sizeof(query));
    sprintf(query, "delete from virtual_file where parent_id = %d and owner_id = %d", code, user_id);
    ret = do_query(query, &conn);
    if(ret != 0)
    {
        return EXIT_FAILURE;
    }

    mysql_close(conn);
    return 0;
}

// 获得父结点的id
int get_parent_id(int id, int *pparent_code)
{
    MYSQL *conn;
    int ret = mysqlconnect(&conn);
    if(ret != 0)
    {
        return EXIT_FAILURE;
    }
    ret = mysql_query(conn, "set names 'utf8'");
    if(ret != 0)
    {
        printf("error query1:%s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }
    char query[1024] = {0};
    sprintf(query, "select parent_id from virtual_file where id = %d", id);

    ret = do_query(query, &conn);
    if(ret != 0)
    {
        return EXIT_FAILURE;
    }
    MYSQL_RES *result = mysql_store_result(conn);
    if(result == NULL)
    {
        printf("error result: %s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }
    int rows = mysql_num_rows(result);
    
    if(rows == 0)// 文件不存在返回1
    {
        mysql_free_result(result);
        mysql_close(conn);
        return 1;
    }
    MYSQL_ROW row;
    if((row = mysql_fetch_row(result)) == NULL)
    {
        return EXIT_FAILURE;
    }
    *pparent_code = atoi(row[0]);
    mysql_free_result(result);
    mysql_close(conn);
    return 0;
}