#include"threadPool.h"
#include "worker.h"

int init_clientQueue(clientQueue_t *pclientQueue)
{
    pclientQueue->timer = 0;
    memset(pclientQueue->client, -1, sizeof(pclientQueue->client));
    memset(pclientQueue->index, 0, sizeof(pclientQueue->index));
    for(int i = 0; i < TIME_SLICE; ++i)// 结构体数组初始化
    {
        pclientQueue->time_out[i].head = NULL;
        pclientQueue->time_out[i].tail = NULL;
        pclientQueue->time_out[i].size = 0;
    }
}

int fdAdd(int netFd, clientQueue_t *pclientQueue)// 尾插法
{
    slotNode_t *newSlot = (slotNode_t *)calloc(1, sizeof(slotNode_t));
    newSlot->netFd = netFd;
    pclientQueue->index[netFd] = (pclientQueue->timer + TIME_SLICE - 1) % TIME_SLICE;// 加入时有TIME_SLICE的时间，timer是下一次要检查的地方
    int index = (pclientQueue->timer + TIME_SLICE - 1) % TIME_SLICE;
    ++pclientQueue->time_out[index].size;
    if(pclientQueue->time_out[index].head == NULL)
    {
        pclientQueue->time_out[index].head = newSlot;
        pclientQueue->time_out[index].tail = newSlot;
    }
    else
    {
        pclientQueue->time_out[index].tail->next = newSlot;
        pclientQueue->time_out[index].tail = newSlot;
    }
}

int fdDel(int netFd, clientQueue_t *pclientQueue)
{
    int index = pclientQueue->index[netFd];
    pclientQueue->index[netFd] = 0;// 重设为0
    slotNode_t *cur = pclientQueue->time_out[index].head;
    slotNode_t *pre = cur;
    if(cur == NULL)// 可能已经为空
    {
        return 0;
    }
    if(cur->netFd == netFd)
    {
        pclientQueue->time_out[index].head = cur->next;
        free(cur);
        --pclientQueue->time_out[index].size;
    }
    else
    {
        while(cur != NULL)
        {
            if (cur->netFd == netFd)
            {
                pre->next = cur->next;
                free(cur);
                --pclientQueue->time_out[index].size;
                break;
            }
            pre = cur;
            cur = cur->next;
        }
    }
}