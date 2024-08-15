// threadpool.h: 标准系统包含文件的包含文件
// 或项目特定的包含文件。

#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

typedef struct ThreadPool ThreadPool;

// 创建线程池并初始化
ThreadPool* threadPoolCreate(int min, int max, int queueCapacity);

// 销毁线程池
int threadPoolDestroy(ThreadPool* pool);

// 添加任务
void threadPoolAdd(ThreadPool* pool, void (*function) (void *), void* arg);

// 获取线程池中工作的线程个数
int threadPoolBusyNum(ThreadPool* pool);

// 获取线程池中活着的线程的个数
int threadPoolAliveNum(ThreadPool* pool);




void* worker(void* arg);
void* manager(void* arg);
void threadExit(ThreadPool* pool);


#endif // !_THREADPOOL_H_


