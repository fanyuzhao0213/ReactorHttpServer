#include "Channel.h"
#include <stdlib.h>

struct Channel* channelInit(int fd, int events,handleFunc readFunc,handleFunc writeFunc,handleFunc destroyFunc,void *arg)
{
    //申请内存
    struct Channel* channel = (struct Channel *)malloc(sizeof(struct Channel));
    channel->fd = fd;
    channel->events = events;
    channel->readCallback = readFunc;
    channel->writeCallback = writeFunc;
    channel->destroyCallback =destroyFunc;
    channel->arg = arg;

    return channel; //返回初始化的channel
}


void writeEventEnable(struct Channel* channel, bool flag)
{
    if(flag)
    {
        channel->events |= WriteEvent;
    }
    else
    {
        channel->events &= ~WriteEvent;
    } 
}

bool isWriteEventEnable(struct Channel* channel)
{
    return channel->events & WriteEvent;
}