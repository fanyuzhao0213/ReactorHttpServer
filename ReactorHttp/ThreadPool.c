#include "ThreadPool.h"
#include <assert.h>
#include <stdlib.h>
#include "Log.h"


struct ThreadPool* threadPoolInit(struct EventLoop* mainLoop, int count)   // 初始化线程池
{
    struct ThreadPool* pool = (struct ThreadPool*)malloc(sizeof(struct ThreadPool));
    pool->index = 0;
    pool->isStart = false;
    pool->mainloop = mainLoop;
    pool->threadNum = count;
    pool->workThreas = (struct WorkerThread*)malloc(sizeof(struct WorkerThread) * count);
    return pool;
}

/*主线程运行此函数*/
// 启动线程池
void threadPoolRun(struct ThreadPool* pool)
{
    assert(pool && !pool->isStart);        //判断pool存在，并且线程池没有启动
    // Debug("assert pool...");
    // Debug("pool->mainloop->thread_Id :%ld",pool->mainloop->thread_Id);
    // Debug("pthread_self() :%ld",pthread_self());
    if(pool->mainloop->thread_Id != pthread_self())
    {
        exit(0);
    }
    pool->isStart = true;
    if(pool->threadNum)
    {
        for(int i = 0; i < pool->threadNum; i++)
        {
            workerThreadInit(&pool->workThreas[i],i);        //子线程初始化
            workerThreadRun(&pool->workThreas[i]);           //运行子线程
        }
    }

}

/*主线程运行此函数*/
// 取出线程池中的某个子线程的反应堆实例
struct EventLoop* takeWorkerEventLoop(struct ThreadPool* pool)
{
    assert(pool->isStart);   //判断线程池必须运行状态

    if(pool->mainloop->thread_Id != pthread_self())
    {
        exit(0);
    }

    struct EventLoop* evloop = pool->mainloop;          //先将主线程的evloop赋值
    if(pool->threadNum > 0)
    {
        evloop = pool->workThreas[pool->index].evLoop;
        pool->index++;
        pool->index = pool->index % pool->threadNum;

    }
    return evloop;
}
 