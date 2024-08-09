#pragma once
#include <pthread.h>
#include "EventLoop.h"


struct WorkerThread
{
    pthread_t  threadID;                                        //线程ID
    char name[24];                                              //线程名字
    pthread_mutex_t mutex;              
    pthread_cond_t cond;
    struct EventLoop* evLoop;                                   //反应堆模型
};



int workerThreadInit(struct WorkerThread* thread, int index);   // 初始化
void workerThreadRun(struct WorkerThread* thread);              // 启动线程          
