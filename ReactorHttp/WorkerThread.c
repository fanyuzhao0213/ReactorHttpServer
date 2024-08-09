#include "WorkerThread.h"
#include <stdio.h>

int workerThreadInit(struct WorkerThread* thread, int index)   // 初始化
{
    thread->evLoop = NULL;
    thread->threadID = 0;
    sprintf(thread->name,"SubThread-%d",index);
    pthread_mutex_init(&thread->mutex,NULL);
    pthread_cond_init(&thread->cond,NULL);

    return 0;
}

//子线程的回调函数
void* subThreadRunning(void* arg)
{
    struct WorkerThread* thread = (struct WorkerThread*)arg;
    pthread_mutex_lock(&thread->mutex);
    thread->evLoop = eventLoopInitEx(thread->name);      //子线程的evloop初始化
    pthread_mutex_unlock(&thread->mutex);
    pthread_cond_signal(&thread->cond);   //启动子线程的evloop初始化
    eventLoopRun(thread->evLoop);
    return NULL;
}

void workerThreadRun(struct WorkerThread* thread)              // 启动线程 
{
    //创建子线程
    pthread_create(&thread->threadID,NULL,subThreadRunning,thread);
    // 阻塞主线程, 让当前函数不会直接结束
    //TODO 是否可以写成线程分离，待测试
    pthread_mutex_lock(&thread->mutex);
    while (thread->evLoop == NULL)
    {
        pthread_cond_wait(&thread->cond, &thread->mutex);
    }
    pthread_mutex_unlock(&thread->mutex);

}    