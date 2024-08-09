#include "TcpServer.h"
#include <arpa/inet.h>
#include "TcpConnection.h"
#include <stdio.h>
#include <stdlib.h>
#include "Log.h"


// 初始化
struct TcpServer* tcpServerInit(unsigned short port, int threadNum)
{
    struct TcpServer* tcp = (struct TcpServer*)malloc(sizeof(struct TcpServer));
    tcp->listener = listenerInit(port);                 //初始化得到1个监听描述符，包含fd和port
    tcp->mainLoop = eventLoopInit();                    //eventLoopInit()初始化没有参数，说明初始化的是主线程的eventloop，name = mainthread
    tcp->threadNum = threadNum;
    tcp->threadPool = threadPoolInit(tcp->mainLoop,threadNum);  //线程池初始化，threadNum初始化几个子线程

    // Debug("tcp->mainLoop->threadName :%s",tcp->mainLoop->threadName);
    // Debug("tcp->mainLoop->thread_Id:%ld",tcp->mainLoop->thread_Id);
    return tcp;
}


// 初始化监听
struct Listener* listenerInit(unsigned short port)
{
    struct Listener* listener = (struct Listener*)malloc(sizeof(struct Listener));
    // 1. 创建监听的fd
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1)
    {
        perror("socket");
        return NULL;
    }
    // 2. 设置端口复用
    int opt = 1;
    int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
    if (ret == -1)
    {
        perror("setsockopt");
        return NULL;
    }
    // 3. 绑定
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    ret = bind(lfd, (struct sockaddr*)&addr, sizeof addr);
    if (ret == -1)
    {
        perror("bind");
        return NULL;
    }
    // 4. 设置监听
    ret = listen(lfd, 128);
    if (ret == -1)
    {
        perror("listen");
        return NULL;
    }
    // 返回fd
    listener->lfd = lfd;
    listener->port = port;
    return listener;
}

int acceptConnection(void* arg)
{
    struct TcpServer* server = (struct TcpServer*)arg;
    // 和客户端建立连接
    int cfd = accept(server->listener->lfd, NULL, NULL);
    // 从线程池中取出一个子线程的反应堆实例, 去处理这个cfd
    struct EventLoop* evLoop = takeWorkerEventLoop(server->threadPool);
    // 将cfd放到 TcpConnection中处理
    Debug("take evLoop success...");
    Debug("takeWorkerEventLoop evLoop->threadName : %s",evLoop->threadName);
    Debug("takeWorkerEventLoop evLoop->thread_Id : %ld",evLoop->thread_Id);
    tcpConnectionInit(cfd, evLoop);
    return 0;
}

void tcpServerRun(struct TcpServer* server)
{
    Debug("服务器程序已经启动了...");
    // 启动线程池
    threadPoolRun(server->threadPool);                      //主线程启动线程池
    Debug("线程池已经启动了...");
    // 添加检测的任务
    // 初始化一个channel实例                                 //往主线程添加1个channel实例，因为是与服务器进行连接，因此需要readevent事件
    struct Channel* channel = channelInit(server->listener->lfd,
        ReadEvent, acceptConnection, NULL, NULL, server);
    Debug("Channel 初始化成功..."); 
    eventLoopAddTask(server->mainLoop, channel, ADD);       // 添加任务到主线程的任务队列     
    Debug("Channel add 成功..."); 
    // 启动反应堆模型
    eventLoopRun(server->mainLoop);                         // 启动主线程反应堆，启动以后，会循环检测evloop里面的dispcher里面的任务函数，做对应的操作       
    Debug("eventLoop已经启动了...");
}