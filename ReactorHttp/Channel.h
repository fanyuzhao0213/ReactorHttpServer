#pragma once
#include <stdbool.h>

//定义函数指针
typedef int(*handleFunc)(void* arg);    //返回值为int，参数为void*

//定义一个枚举来定义文件描述符的读写事件
enum FdEvrnt
{
    TimeOut = 0x01,
    ReadEvent = 0x02,
    WriteEvent = 0x04
};

struct Channel
{
    int fd;                             //文件描述符
    int events;                         //事件              32bit  用到三位，第一位超市，第二位读，第三位写
    //回调函数
    handleFunc readCallback;            //读函数回调函数
    handleFunc writeCallback;           //写函数回调函数
    handleFunc destroyCallback;         //销毁函数回调函数
    void* arg;                          //回调函数的参数
};

//初始化一个channel
struct Channel* channelInit(int fd, int events,handleFunc readFunc,handleFunc writeFunc,handleFunc destroyFunc,void *arg);
//修改fd的写事件
void writeEventEnable(struct Channel* channel, bool flag);
//判断是否需要检测文件描述符的写事件
bool isWriteEventEnable(struct Channel* channel);
