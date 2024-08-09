#pragma once
#include "Channel.h"
#include "EventLoop.h"

struct EventLoop;
struct Dispatcher
{
    void* (*init)();                                                            // init, 初始化epoll,poll或者select需要的数据块
    int (*add)(struct Channel* channel, struct EventLoop* evloop);              //增加
    int (*remove)(struct Channel* channel, struct EventLoop* evloop);           //移除
    int (*modify)(struct Channel* channel, struct EventLoop* evloop);           //修改                              //modify
    int (*dispatch)(struct EventLoop* evloop, int timeout);                   // 事件检测  时间单位 s
    int (*clear)(struct EventLoop* evloop);                                     //清除数据 （关闭fd或者释放内存）
};