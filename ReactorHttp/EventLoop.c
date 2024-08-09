#include "EventLoop.h"
#include <assert.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Log.h"

// 写数据
void taskWakeup(struct EventLoop* evLoop)
{
    const char* msg = "我是要成为海贼王的男人!!!";
    write(evLoop->socketPair[0], msg, strlen(msg));
    Debug("主线程通知子线程....");
}

// 读数据       数据不处理，只是为了解决子线程阻塞的问题
int readLocalMessage(void* arg)
{
    Debug("子线程收到主线程通知子线程....");
    struct EventLoop* evLoop = (struct EventLoop*)arg;
    char buf[256];
    read(evLoop->socketPair[1], buf, sizeof(buf));
    return 0;
}


struct EventLoop* eventLoopInit()
{
    return eventLoopInitEx(NULL);
}

struct EventLoop* eventLoopInitEx(const char* threadName)
{
    struct EventLoop* evLoop = (struct EventLoop*)malloc(sizeof(struct EventLoop));
    evLoop->isQuit = false;                                 //默认evloop不关闭
    evLoop->thread_Id = pthread_self();                     //获取threadID
    pthread_mutex_init(&evLoop->mutex,NULL);                //mutex初始化
    strcpy(evLoop->threadName,threadName == NULL ? "mainthread":threadName);    //threadName初始化
    evLoop->dispatcher = &EpollDispatcher;                  //evloop中dispatcher初始化
    evLoop->dispatcherData = evLoop->dispatcher->init();    //evloop中dispatcherdata初始化
    evLoop->head = evLoop->tail = NULL;                     //任务队列初始化
    evLoop->channelMap = ChannelMapInit(128);               //channelMap初始化  初始化128个channel,不够可以扩容
    //初始化2个文件描述符用于线程间通信,
    //AF_UNIX  AF_LOCAL   用于本地通信
    //evLoop->socketPair
    int ret = socketpair(AF_UNIX,SOCK_STREAM,0,evLoop->socketPair);
    if(ret == -1)
    {
        perror("socketpair");
        exit(0);
    }

    Debug("evLoop->threadName : %s",evLoop->threadName);
    // 指定规则: evLoop->socketPair[0] 发送数据, evLoop->socketPair[1] 接收数据
    struct Channel* channel = channelInit(evLoop->socketPair[1],ReadEvent,
                                    readLocalMessage,NULL,NULL,evLoop);

    eventLoopAddTask(evLoop,channel,ADD);
    return evLoop;
} 

//启动反应堆
int eventLoopRun(struct EventLoop* evLoop)
{
    assert(evLoop !=NULL );  //断言，条件为真则跳过，否则程序结束，输出错误
    struct Dispatcher* dispatcher = evLoop->dispatcher;

    // Debug("evLoop->thread_Id : %ld",evLoop->thread_Id); 
    // Debug("pthread_self() : %ld",pthread_self()); 

    /*比较线程是否正常*/
    if(evLoop->thread_Id != pthread_self())
    {
        return -1;
    }
    // Debug("evLoop->isQuit: %d",evLoop->isQuit);     
    /*循环进行事件处理*/
    while(!evLoop->isQuit)
    {
        dispatcher->dispatch(evLoop,2);     //调用dispatch函数
        eventLoopProcessTask(evLoop);
    }

    return 0;
}

//处理被激活的文件fd
int eventActivate(struct EventLoop* evLoop, int fd, int event)
{
    if(fd < 0 || evLoop == NULL)
    {
        return -1;
    }

    //取出channel
    struct Channel* channel = evLoop->channelMap->list[fd];
    assert(channel->fd == fd);
    //&& channel->readCallback加这个的原因是因为这个channel里面的回调函数可能为空
    if(event & ReadEvent && channel->readCallback)
    {
        //调用最终的读回调处理函数
        channel->readCallback(channel->arg); 
    }
    if(event & WriteEvent && channel->writeCallback)
    {
        //调用最终的写回调处理函数
        channel->writeCallback(channel->arg);    
    }
    return 0;
}


// 添加任务到任务队列
int eventLoopAddTask(struct EventLoop* evLoop, struct Channel* channel, int type)
{
    //加锁，保护资源
    pthread_mutex_lock(&evLoop->mutex);
    //创建节点
    struct ChannelElement* node = (struct ChannelElement*)malloc(sizeof(struct ChannelElement));
    node->channel = channel;
    node->type = type;
    node->next = NULL;
    //如果链表为空
    if(evLoop->head == NULL)
    {
        evLoop->head = evLoop->tail = node;
    }
    else
    {
        evLoop->tail->next = node;  //追加
        evLoop->tail = node;        //后移
    }
    pthread_mutex_unlock(&evLoop->mutex);

    //处理节点
    /*
    * 细节:
    *   1. 对于链表节点的添加: 可能是当前线程也可能是其他线程(主线程)
    *       1). 修改fd的事件, 当前子线程发起,  当前子线程处理
    *       2). 添加新的fd, 添加任务节点的操作是由主线程发起的
    *   2. 不能让主线程处理任务队列, 需要由当前的子线程取处理
    */
    if(evLoop->thread_Id == pthread_self())
    {
        //如果是当前线程,（子线程）
        eventLoopProcessTask(evLoop);
    }
    else
    {
        Debug("主线程....");
        //主线程
        //需要告诉子线程处理任务队列中的任务
        //1.子线程在工作，2，子线程被阻塞
        //因此需要通知子线程，用初始化的fd
        taskWakeup(evLoop);
    }
    return 0;
}

// 处理任务队列中的任务
int eventLoopProcessTask(struct EventLoop* evLoop)
{
    //加锁，保护资源
    pthread_mutex_lock(&evLoop->mutex);
    //取出头节点
    struct ChannelElement* head = evLoop->head;
    while(head!=NULL)
    {
        struct Channel* channel = head->channel;
        if(head->type == ADD)
        {
            // Debug("channel->fd :%d",channel->fd);
            //将 channel中的fd 以及 类型增加到dispatch 
            eventLoopAdd(evLoop, channel);
        }
        else if(head->type  == DELETE)
        {
            //删除
            eventLoopRemove(evLoop, channel);
        }
        else if(head->type  == MODIFY)
        {
            //修改
            eventLoopModify(evLoop, channel);
        }
        //head被处理完之后需要指针后移
        //并且需要释放处理之前的地址
        struct ChannelElement* tmp = head;
        head = head->next;
        free(tmp);  
    }
    //如果全部处理完则需要将指针重新初始化
    evLoop->head = evLoop->tail = NULL;
    pthread_mutex_unlock(&evLoop->mutex);

    return 0;
}
int eventLoopAdd(struct EventLoop* evLoop, struct Channel* channel)

{
    int fd = channel->fd;
    struct ChannelMap* channelMap = evLoop->channelMap;
    //判断要添加的fd 是不是属于 channelmap中
    if(fd >= channelMap->size)
    {
        // 没有足够的空间存储键值对 fd - channel ==> 扩容
        if(!remakeMapRoom(channelMap,fd,sizeof(struct Channel*)))
        {
            return -1;
        }
    }

    //找到对应的fd
    if(channelMap->list[fd] == NULL)        //判断list[fd]不为空
    {
        channelMap->list[fd] = channel;     //赋值
        evLoop->dispatcher->add(channel,evLoop);    //调用底层的dispatcher
    }

    return 0;
}

int eventLoopRemove(struct EventLoop* evLoop, struct Channel* channel)
{
    int fd = channel->fd;
    struct ChannelMap* channelMap = evLoop->channelMap;
    if (fd >= channelMap->size)
    {
        return -1;
    }
    int ret = evLoop->dispatcher->remove(channel, evLoop);
    return ret;
}

int eventLoopModify(struct EventLoop* evLoop, struct Channel* channel)
{
    int fd = channel->fd;
    struct ChannelMap* channelMap = evLoop->channelMap;
    if (channelMap->list[fd] == NULL)
    {
        return -1;
    }
    int ret = evLoop->dispatcher->modify(channel, evLoop);
    return ret;
}

int destroyChannel(struct EventLoop* evLoop, struct Channel* channel)
{
    //删除channel 和 fd的对应关系
    evLoop->channelMap->list[channel->fd] = NULL;
    //关闭fd
    close(channel->fd);
    //释放channel
    free(channel);
    return 0;
}