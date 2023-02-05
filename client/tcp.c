#include "threadPool.h"
// connect
int tcpInit(int *pSockFd, char *file)
{
    FILE *fp = fopen(file, "r");
    char ip[1024] = {0};
    char port[1024] = {0};
    char buf[1024] = {0};
    fgets(buf, sizeof(buf), fp) != NULL;
    int i = 0;
    while (buf[i] < '0' || buf[i] > '9')
    {
        ++i;
    }
    strcpy(ip, buf + i);
    // printf("ip = %s\n", ip);
    bzero(buf, sizeof(buf));
    fgets(buf, sizeof(buf), fp) != NULL;
    i = 0;
    while (buf[i] < '0' || buf[i] > '9')
    {
        ++i;
    }
    strcpy(port, buf + i);
    // printf("port = %s\n", port);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(port));
    addr.sin_addr.s_addr = inet_addr(ip);
    int ret = connect(*pSockFd, (struct sockaddr *)&addr, sizeof(addr));
    ERROR_CHECK(ret, -1, "connect");
}
