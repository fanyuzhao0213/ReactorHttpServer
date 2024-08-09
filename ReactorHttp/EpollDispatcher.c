#include "Dispatcher.h"
#include <sys/epoll.h>
#include "Dispatcher.h"
#include <sys/epoll.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "Log.h"

#define     max     520
struct EpollData
{
    int epfd;
    struct epoll_event* events;
};

static void* epollInit();
static int epollAdd(struct Channel* channel, struct EventLoop* evloop);
static int epollremove(struct Channel* channel, struct EventLoop* evloop);
static int epollmodify(struct Channel* channel, struct EventLoop* evloop);
static int epolldispatch(struct EventLoop* evloop, int timeout);
static int epollclear(struct EventLoop* evloop);        

struct Dispatcher EpollDispatcher = {
    .init = epollInit,
    .add = epollAdd,
    .remove = epollremove,
    .modify = epollmodify,
    .dispatch = epolldispatch,
    .clear = epollclear
};

static void* epollInit()
{
    //数据类型转换
    struct EpollData* data = (struct EpollData*)malloc(sizeof(struct EpollData));
    data->epfd = epoll_create(1);       //参数1没有实际意义，只要大于0就行
    if(data->epfd == -1)
    {
        perror("epoll_create");
        exit(0);
    }
    //calloc(size_t num, size_t size) 用于动态分配内存。
    //它会分配一块能够容纳 num 个 size 字节的内存，并将这块内存的所有字节初始化为0。
    data->events = (struct epoll_event*)calloc(max,sizeof(struct epoll_event));
    
    return data;  //返回EpollData类型数据，包含fd以及events
}

//公共模板控制epollevtnt
static int epollCtl(struct Channel* Channel,struct EventLoop* evloop,int op)
{
    struct EpollData* data = (struct EpollData*)evloop->dispatcherData;
    struct epoll_event ev;
    ev.data.fd = Channel->fd;
    ev.events = 0;
    int events = 0;
    if(Channel->events & ReadEvent)     //读事件
    {
        events |= EPOLLIN;
    }
    if(Channel->events & WriteEvent)
    {
        events |= EPOLLOUT;
    }
    ev.events = events;
    int ret = epoll_ctl(data->epfd,op,Channel->fd,&ev);
    return ret;
}

static int epollAdd(struct Channel* channel, struct EventLoop* evloop)
{
    int ret = epollCtl(channel,evloop,EPOLL_CTL_ADD);
    if(ret == -1)
    {
        perror("epoll_ctl add");
        exit(0);
    }
    
    return ret;
}
static int epollremove(struct Channel* channel, struct EventLoop* evloop)
{
    int ret = epollCtl(channel,evloop,EPOLL_CTL_DEL);
    if(ret == -1)
    {
        perror("epoll_ctl del");
        exit(0);
    }
    
    return ret;
}
static int epollmodify(struct Channel* channel, struct EventLoop* evloop)
{
    int ret = epollCtl(channel,evloop,EPOLL_CTL_MOD);
    if(ret == -1)
    {
        perror("epoll_ctl mod");
        exit(0);
    }

    return ret;
}
static int epolldispatch(struct EventLoop* evloop, int timeout)
{
    struct EpollData* data = (struct EpollData*)evloop->dispatcherData;
    //timeout 传入参数为秒，而epoll_wait最后一个参数为ms，因此要乘 1000
    //阻塞函数
    // Debug("start epoll wait");
    int count = epoll_wait(data->epfd,data->events,max, timeout * 1000);
    for(int i = 0; i<count;i++)
    {
        int events = data->events[i].events;
        int fd = data->events[i].data.fd;
        if(events & EPOLLERR || events & EPOLLHUP)
        {
            //对方断开了连接;
            //epollRemove(Channel, evLoop)
            continue;
        }
        if(events & EPOLLIN)
        {
            //读事件
            eventActivate(evloop, fd, ReadEvent);
        }
        if(events & EPOLLOUT)
        {
            //写事件
            eventActivate(evloop, fd, WriteEvent);
        }
    }
    return 0;
}
static int epollclear(struct EventLoop* evloop)
{
    struct EpollData* data = (struct EpollData*)evloop->dispatcherData;
    free(data->events);
    close(data->epfd);
    free(data);
    return 0;
}  