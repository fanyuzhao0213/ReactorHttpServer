#pragma once

#include <stdbool.h>
#include "Dispatcher.h"
#include "ChannelMap.h"
#include <pthread.h>

extern struct Dispatcher EpollDispatcher;
extern struct Dispatcher PollDispatcher;
extern struct Dispatcher SelectDispatcher;


enum ElemType
{
    ADD,            //增加
    DELETE,         //删除
    MODIFY          //修改
};

//定义任务队列的节点
struct ChannelElement
{
    int type;                       //如何处理该节点，分别对应一个枚举类型ElemType
    struct Channel* channel;        //定义一个channel节点
    struct ChannelElement* next;    //链表节点
};

struct Dispatcher;
struct EventLoop
{
    bool isQuit;                        // evloop是否停止
    struct Dispatcher* dispatcher;      // dispatcher 实例 
    void* dispatcherData;               // dispatcher 实例 返回的数据，数据类型为void*，
                                        //因为要对应不同的多路复用，比如epoll,poll,select
    struct ChannelElement* head;        //头节点
    struct ChannelElement* tail;        //尾节点
    struct ChannelMap* channelMap;      //任务map
    pthread_t thread_Id;                //线程ID
    char threadName[32];                //线程name
    pthread_mutex_t mutex;              //加锁evloop对于任务队列的操作
    int socketPair[2];                  //存储本地通信的fd，通过socketpair初始化
};

struct EventLoop* eventLoopInit();                                                  //evloop初始化（主线程不传入参数，默认为mainthread）
struct EventLoop* eventLoopInitEx(const char* threadName);                          //evloop初始化    
int eventLoopRun(struct EventLoop* evloop);                                         //启动反应堆
int eventActivate(struct EventLoop* evloop, int fd, int event);                     //处理被激活的文件fd
int eventLoopAddTask(struct EventLoop* evLoop, struct Channel* channel, int type);  //添加任务到任务队列
int eventLoopProcessTask(struct EventLoop* evLoop);//处理任务队列中的任务
//处理dispatcher中的节点
int eventLoopAdd(struct EventLoop* evLoop, struct Channel* channel);
int eventLoopRemove(struct EventLoop* evLoop, struct Channel* channel);
int eventLoopModify(struct EventLoop* evLoop, struct Channel* channel);
int destroyChannel(struct EventLoop* evLoop, struct Channel* channel);//释放channel