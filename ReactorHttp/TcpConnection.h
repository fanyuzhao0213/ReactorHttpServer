#pragma once
#include "EventLoop.h"
#include "Buffer.h"
#include "Channel.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

struct TcpConnection
{
    struct EventLoop* evloop;
    struct Channel* channel;
    struct Buffer* readBuf;
    struct Buffer* writeBuf;
    char name[32];
    // http 协议
    struct HttpRequest* request;
    struct HttpResponse* response;
};

struct TcpConnection* tcpConnectionInit(int fd, struct EventLoop* evloop);  // TcpConnection初始化
int tcpConnectionDestroy(void* conn);                                       // TcpConnection销毁