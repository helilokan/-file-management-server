#include"threadPool.h"
#include "worker.h"

int sysadd(int user_id, char *cmd)
{
    time_t now = time(NULL);
    struct tm *pTm = localtime(&now);
    char user_name[100] = {0};
    get_user_name(user_name, user_id);
    syslog(LOG_INFO,"%4d%2d%2d %2d:%2d:%2d user:%s %s\n",
            pTm->tm_year+1900,pTm->tm_mon+1,pTm->tm_mday, pTm->tm_hour, pTm->tm_min, pTm->tm_sec,
            user_name, cmd);
}
    

int do_cd(char *dir_name, int netFd, int user_id)
{
    // 不考虑目标目录地址以/开头，这种情况直接报错
    int check = 0;
    if(dir_name[0] == '/')
    {
        check = 1;
        send(netFd, &check, sizeof(check), MSG_NOSIGNAL);
        return 0;
    }
    int code;
    int parent_code;
    get_user_code(user_id, &code);// 获得本用户的当前目录编号
    parent_code = code;
    char pwd[1024] = {0};
    get_user_pwd(user_id, pwd);
    char *words[20] = {0};
    int i = 0;
    char s[100] = {0};
    // 如果是..就返回上级目录
    if(strcmp(dir_name, "..") == 0 || strcmp(dir_name, "../") == 0)
    {
        if(code == 0)// 已经在根目录，不能返回上级
        {
            check = 1;
            send(netFd, &check, sizeof(check), MSG_NOSIGNAL);
            return 0;
        }
        get_parent_id(code, &parent_code);
        code = parent_code;

        // 切分当前路径
        strcpy(s, pwd);
        words[i] = strtok(s, "/");
        ++i;
        while ((words[i] = strtok(NULL, "/")) != NULL)
        {
            ++i;
        }
        char type[2];
        bzero(pwd, sizeof(pwd));
        for(int j = 0; j < i - 1; ++j)
        {
            strcat(pwd, words[j]);
            strcat(pwd, "/");
        }
    }
    else if(strcmp(dir_name, "~/") == 0 || strcmp(dir_name, "~") == 0)// 回到根目录
    {
        strcpy(pwd, "~/");
        code = 0;
    }
    // 不是..就去往目的地址
    else
    {
        // 将目标地址切分
        bzero(s, sizeof(s));
        bzero(words, sizeof(words));
        strcpy(s, dir_name);
        words[i] = strtok(s, "/");
        ++i;
        while ((words[i] = strtok(NULL, "/")) != NULL)
        {
            ++i;
        }

        for (int j = 0; j < i; ++j)
        {
            if (strcmp(words[j], ".") == 0)
            {
                continue;
            }
            if (strcmp(words[j], "~") == 0)
            {
                parent_code = code = 0;
                continue;
            }
            if (strcmp(words[j], "..") == 0)
            {
                if (code == 0) // 已经在根目录，不能返回上级
                {
                    check = 1;
                    send(netFd, &check, sizeof(check), MSG_NOSIGNAL);
                    return 0;
                }
                get_parent_id(code, &parent_code);
                code = parent_code;
                continue;
            }
            // printf("%s\n", words[j]);
            char type[2];
            int ret = get_file_code(user_id, words[j], &code, parent_code, type); // 获取当前文件在虚拟文件目录的id和类型
            if (ret != 0 || strcmp(type, "d") != 0)  // 文件查找失败；不是目录文件；不在可搜索路径下
            {
                check = 1;
                send(netFd, &check, sizeof(check), MSG_NOSIGNAL);
                return 0;
            }
            if (parent_code != 0)
            {
                strcat(pwd, "/");
            }
            strcat(pwd, words[j]);
            parent_code = code; // 继续往下遍历
        }
    }
    // printf("pwd = %s, code = %d\n", pwd, code);
    update_user_pwd(user_id, pwd, code);
    send(netFd, &check, sizeof(check), MSG_NOSIGNAL);
    train_t train;
    train.length = strlen(pwd);
    strcpy(train.buf, pwd);
    send(netFd, &train, sizeof(train.length) + train.length, MSG_NOSIGNAL);
    return 0;
}

int do_ls(char *file_name, int netFd, int user_id)
{
    // puts("ls");
    // 查看当前目录
    // printf("file_name lth = %ld\n", strlen(file_name));
    int code;
    int parent_code;
    int check = 0;
    char type[2];
    get_user_code(user_id, &code);// 获得当前所在目录的id
    parent_code = code;
    if(file_name[0] == '/')
    {
        check = 1;
        send(netFd, &check, sizeof(check), MSG_NOSIGNAL);
        return 0;
    }
    if(strlen(file_name) == 0 || strcmp(file_name, ".") == 0 || strcmp(file_name, "./") == 0)
    {
        // code不用改变
        printf("local!\n");
    }
    else// 与cd一样的判断
    {
        char pwd[1024] = {0};
        get_user_pwd(user_id, pwd);
        char *words[20] = {0};
        int i = 0;
        char s[100] = {0};
        // 是..的情况
        if (strcmp(file_name, "..") == 0 || strcmp(file_name, "../") == 0)
        {
            if (code == 0) // 已经在根目录，不能返回上级
            {
                check = 1;
                send(netFd, &check, sizeof(check), MSG_NOSIGNAL);
                return 0;
            }
            get_parent_id(code, &parent_code);
            code = parent_code;
        }
        // 不是..就去往目的地址
        else
        {
            // 将目标地址切分
            bzero(s, sizeof(s));
            bzero(words, sizeof(words));
            strcpy(s, file_name);
            words[i] = strtok(s, "/");
            ++i;
            while ((words[i] = strtok(NULL, "/")) != NULL)
            {
                ++i;
            }

            for (int j = 0; j < i; ++j)
            {
                if (strcmp(words[j], ".") == 0)
                {
                    continue;
                }
                if (strcmp(words[j], "~") == 0)// 如果以根目录开始，则改变code值为0
                {
                    parent_code = code = 0;
                    continue;
                }
                if (strcmp(words[j], "..") == 0)
                {
                    if (code == 0) // 已经在根目录，不能返回上级
                    {
                        check = 1;
                        send(netFd, &check, sizeof(check), MSG_NOSIGNAL);
                        return 0;
                    }
                    get_parent_id(code, &parent_code);
                    code = parent_code;
                    continue;
                }
                // printf("%s\n", words[j]);
                char type[2];
                int ret = get_file_code(user_id, words[j], &code, parent_code, type); // 获取当前文件在虚拟文件目录的id和类型
                if (ret != 0 || strcmp(type, "d") != 0)  // 文件查找失败；不是目录文件；不在可搜索路径下
                {
                    check = 1;
                    send(netFd, &check, sizeof(check), MSG_NOSIGNAL);
                    return 0;
                }
                parent_code = code; // 继续往下遍历，遍历结束code就是要浏览的目录的文件id
            }
        }
    }
    printf("code = %d\n", code);
    send(netFd, &check, sizeof(check), MSG_NOSIGNAL);
    find_code_files(netFd, code, user_id);// 把找到的文件发给客户端

}

int do_pwd(char *file_name, int user_id, int netFd)
{
    // puts("pwd");
    char pwd[1024] = {0};
    get_user_pwd(user_id, pwd);
    printf("pwd : %s\n", pwd);
    train_t train;
    train.length = strlen(pwd);
    strcpy(train.buf, pwd);
    send(netFd, &train, sizeof(train.length) + train.length, MSG_NOSIGNAL);
}

int do_rm(char *file_name, int user_id, int netFd)
{
    puts("rm");
    int code;
    int parent_code;
    char type[2] = {0};
    char file[100] = {0};
    int check = 0;
    get_user_code(user_id, &code);// 获得当前所在目录的id
    parent_code = code;
    if(file_name[0] == '/' || strcmp(file_name, ".") == 0 || strcmp(file_name, "~/") == 0 || strcmp(file_name, "./") == 0 || strcmp(file_name, "..") == 0 || strcmp(file_name, "../") == 0)
    {
        check = 1;
        send(netFd, &check, sizeof(check), MSG_NOSIGNAL);
        return 0;
    }
    else// 不是以上几种情况就遍历获得要删除的文件的id和类型
    {
        // 将目标地址切分
        char *words[20] = {0};
        int i = 0;
        char s[100] = {0};
        strcpy(s, file_name);
        words[i] = strtok(s, "/");
        ++i;
        while ((words[i] = strtok(NULL, "/")) != NULL)
        {
            ++i;
        }

        for (int j = 0; j < i; ++j)
        {
            if (strcmp(words[j], ".") == 0)
            {
                continue;
            }
            if (strcmp(words[j], "~") == 0)
            {
                parent_code = code = 0;
                continue;
            }
            if (strcmp(words[j], "..") == 0)
            {
                if (code == 0) // 已经在根目录，不能返回上级
                {
                    check = 1;
                    send(netFd, &check, sizeof(check), MSG_NOSIGNAL);
                    return 0;
                }
                get_parent_id(code, &parent_code);
                code = parent_code;
                continue;
            }
            // printf("%s\n", words[j]);
            
            int ret = get_file_code(user_id, words[j], &code, parent_code, type); // 获取当前文件在虚拟文件目录的id和类型
            printf("ret = %d\n", ret);
            if(ret != 0)// 路径错误
            {
                check = 1;
                send(netFd, &check, sizeof(check), MSG_NOSIGNAL);
                return 0;
            }
            if(j != i-1)
            {
                parent_code = code; // 继续往下遍历，遍历结束code就是要浏览的目录的文件id
            }
        }
        strcpy(file, words[i - 1]);
    }
    if(code == 0)
    {
        check = 1;
        send(netFd, &check, sizeof(check), MSG_NOSIGNAL);
        return 0;
    }
    // printf("code = %d\n", code);
    if(strcmp(type, "f") == 0)
    {
        char md5_str[MD5_STR_LEN + 1] = {0};
        get_virtual_file_md5(file, user_id, parent_code, md5_str);// 要先查询再删除
        minus_link(md5_str);// 本地链接数减一
    }
    del_virtual_file(code, user_id);// 先查询后删除
    send(netFd, &check, sizeof(check), MSG_NOSIGNAL);
}

int do_mkdir(char *dir_name, int netFd, int user_id)
{
    // puts("mkdir");
    int check = 0;
    int code;
    get_user_code(user_id, &code);
    // printf("code = %d\n", code);
    int test;
    char type[2] = {0};
    int ret = get_file_code(user_id, dir_name, &test, code, type);
    if(ret == 0)// 说明该名称的文件夹已经存在
    {
        check = 1;
        send(netFd, &check, sizeof(check), MSG_NOSIGNAL);
        return 0;
    }
    check = add_virtual_dir(code, dir_name, user_id);
    send(netFd, &check, sizeof(check), MSG_NOSIGNAL);
    return 0;
}

int handle_short_cmd(train_state_t train_get, int netFd, int user_id)
{
    sysadd(user_id, train_get.buf);
    char file_name[100] = {0};
    int i = train_get.pre_length;
    while(train_get.buf[i] == ' ')
    {
        ++i;
    }
    strcpy(file_name, train_get.buf + i);

    if (strncmp("cd", train_get.buf, 2) == 0)
    {
        do_cd(file_name, netFd, user_id);
    }
    else if (strncmp("ls", train_get.buf, 2) == 0)
    {
        do_ls(file_name, netFd, user_id);
    }
    else if (strncmp("pwd", train_get.buf, 3) == 0)
    {
        do_pwd(file_name, user_id, netFd);
    }
    else if (strncmp("rm", train_get.buf, 2) == 0)
    {
        do_rm(file_name, user_id, netFd);
    }
    else if (strncmp("mkdir", train_get.buf, 5) == 0)
    {
        do_mkdir(file_name, netFd, user_id);
    }
}
