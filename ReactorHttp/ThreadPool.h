#pragma once
#include "EventLoop.h"
#include <stdbool.h>
#include "WorkerThread.h"


//定义线程池
struct ThreadPool
{
    struct EventLoop* mainloop;         //主线程的反应堆模型
    bool isStart;                       //线程池是否启动
    int threadNum;                      //工作线程的数量
    struct WorkerThread* workThreas;    //工作线程组
    int index;                          //记录当前哪个线程
};

struct ThreadPool* threadPoolInit(struct EventLoop* mainLoop, int count);   // 初始化线程池
void threadPoolRun(struct ThreadPool* pool);                                // 启动线程池
struct EventLoop* takeWorkerEventLoop(struct ThreadPool* pool);             // 取出线程池中的某个子线程的反应堆实例