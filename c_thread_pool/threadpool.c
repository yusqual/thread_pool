// threadpool.c: 线程池实现。
//

#include "threadpool.h"

const int NUMBER = 2;	// 每次添加或销毁线程的数量

// 任务结构体
typedef struct Task {
	void (*function) (void* arg);
	void* arg;
}Task;

// 线程池结构体
struct ThreadPool {
	Task* taskQ;
	int queueCapacity;	// 容量
	int queueSize;		// 当前任务个数
	int queueFront;	// 对头 -> 取数据
	int queueRear;	// 队尾 -> 放数据

	pthread_t managerId;	// 管理者线程id
	pthread_t* threadIds;	// 工作的线程id
	int minNum;		// 最少的线程数
	int maxNum;		// 最多的线程数
	int busyNum;	// 正在工作的线程数
	int liveNum;	// 存活的线程
	int exitNum;	// 要销毁的线程数

	pthread_mutex_t mutexPool;	// 锁整个的线程池
	pthread_mutex_t mutexBusy;	// 锁busyNum变量
	pthread_cond_t notFull;		// 任务队列是不是满了
	pthread_cond_t notEmpty;	// 任务队列是不是空了

	int shutdown;	// 是否要销毁线程池, 销毁为1, 不销毁为0
	int canAdd;		// 先设置不可添加任务,待任务全部执行完毕后设置shutdown为1

};

ThreadPool* threadPoolCreate(int min, int max, int queueCapacity) {
	ThreadPool* pool = (ThreadPool*)malloc(sizeof(ThreadPool));
	do {
		if (pool == NULL) {
			perror("pool malloc error");
			break;
		}
		pool->threadIds = (pthread_t*)malloc(sizeof(pthread_t) * max);
		if (pool->threadIds == NULL) {
			perror("pool.threadIds malloc error");
			break;
		}
		memset(pool->threadIds, 0, sizeof(pthread_t) * max);
		pool->minNum = min;
		pool->maxNum = max;
		pool->busyNum = 0;
		pool->liveNum = min;
		pool->exitNum = 0;
		pool->canAdd = 1;

		if (pthread_mutex_init(&pool->mutexPool, NULL) != 0 || pthread_mutex_init(&pool->mutexBusy, NULL) != 0 ||
			pthread_cond_init(&pool->notFull, NULL) != 0 || pthread_cond_init(&pool->notEmpty, NULL) != 0) {
			perror("pool mutex or cond init error");	// 此处若初始化失败, 无法释放资源. 需分开写才能释放资源
			break;
		}

		// 任务队列
		pool->taskQ = (Task*)malloc(sizeof(Task) * queueCapacity);
		if (pool->taskQ == NULL) {
			perror("pool taskQ malloc error");
			break;
		}
		memset(pool->taskQ, 0, sizeof(Task) * queueCapacity);
		pool->queueCapacity = queueCapacity;
		pool->queueSize = 0;
		pool->queueFront = 0;
		pool->queueRear = 0;

		pool->shutdown = 0;
		pool->canAdd = 1;

		// 创建线程
		pthread_create(&pool->managerId, NULL, manager, pool);
		for (int i = 0; i < min; ++i) {
			pthread_create(&pool->threadIds[i], NULL, worker, pool);
		}
		return pool;
	} while (0);

	if (pool && pool->threadIds) free(pool->threadIds);
	if (pool && pool->taskQ) free(pool->taskQ);
	if (pool) free(pool);
	return NULL;
}

int threadPoolDestroy(ThreadPool* pool) {
	if (pool == NULL) return -1;
	while (1) {
		pthread_mutex_lock(&pool->mutexPool);
		pool->canAdd = 0;
		if (pool->queueSize || pool->busyNum) {
			pthread_mutex_unlock(&pool->mutexPool);
			sleep(1);
		} else break;
	}
	pool->shutdown = 1;
	// 等待manager时需解锁
	pthread_mutex_unlock(&pool->mutexPool);
	pthread_join(pool->managerId, NULL);
	// 让其余线程退出
	for (int i = 0; i < pool->maxNum; ++i) {
		pthread_cond_signal(&pool->notEmpty); // 唤醒所有等待线程, 让其退出
		usleep(500);	// 等待线程退出
	}
	pthread_mutex_lock(&pool->mutexPool);

	// 回收子线程资源
	for (int i = 0; i < pool->maxNum; ++i) {
		if (pool->threadIds[i] != 0) pthread_join(pool->threadIds[i], NULL);
	}

	// 释放堆内存
	if (pool->taskQ) free(pool->taskQ);
	if (pool->threadIds) free(pool->threadIds);
	pool->taskQ = NULL;
	pool->threadIds = NULL;

	// 释放条件变量和锁
	pthread_cond_destroy(&pool->notEmpty);
	pthread_cond_destroy(&pool->notFull);
	pthread_mutex_destroy(&pool->mutexBusy);
	pthread_mutex_unlock(&pool->mutexPool);
	pthread_mutex_destroy(&pool->mutexPool);
	free(pool);
	pool = NULL;
	return 0;
}

void threadPoolAdd(ThreadPool* pool, void(*function)(void*), void* arg) {
	pthread_mutex_lock(&pool->mutexPool);
	if (pool->queueSize == pool->queueCapacity && !pool->shutdown && pool->canAdd) {
		pthread_cond_wait(&pool->notFull, &pool->mutexPool);
	}
	if (pool->shutdown || !pool->canAdd) {
		pthread_mutex_unlock(&pool->mutexPool);
		return;
	}
	pool->taskQ[pool->queueRear].function = function;
	pool->taskQ[pool->queueRear].arg = arg;
	pool->queueRear = (pool->queueRear + 1) % pool->queueCapacity;
	++pool->queueSize;
	pthread_cond_signal(&pool->notEmpty);

	pthread_mutex_unlock(&pool->mutexPool);
}

int threadPoolBusyNum(ThreadPool* pool) {
	pthread_mutex_lock(&pool->mutexBusy);
	int busyNum = pool->busyNum;
	pthread_mutex_unlock(&pool->mutexBusy);
	return busyNum;
}

int threadPoolAliveNum(ThreadPool* pool) {
	pthread_mutex_lock(&pool->mutexPool);
	int AliveNum = pool->liveNum;
	pthread_mutex_unlock(&pool->mutexPool);
	return AliveNum;
}

void* worker(void* arg) {
	ThreadPool* pool = (ThreadPool*)arg;
	// 不断查询任务队列
	while (1) {
		pthread_mutex_lock(&pool->mutexPool);

		// 若任务队列为空并且线程池启用, 则线程阻塞等待任务
		while (pool->queueSize == 0 && !pool->shutdown) {
			pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);
			// 被唤醒, 若需要销毁线程，则销毁线程.
			if (pool->exitNum) {
				--pool->exitNum;
				--pool->liveNum;
				pthread_mutex_unlock(&pool->mutexPool);
				threadExit(pool);
			}
		}

		// 若线程池被销毁, 则不会进入上面的循环, 需要解锁释放资源
		if (pool->shutdown) {
			pthread_mutex_unlock(&pool->mutexPool);
			threadExit(pool);
		}

		// 正常工作, 取任务(被唤醒后自动获得mutexpool锁)
		Task task;
		task.function = pool->taskQ[pool->queueFront].function;
		task.arg = pool->taskQ[pool->queueFront].arg;
		// 任务队列空闲+1
		pool->queueFront = (pool->queueFront + 1) % pool->queueCapacity;
		--pool->queueSize;	// 池中任务数减一

		// 唤醒生产者
		pthread_cond_signal(&pool->notFull);
		pthread_mutex_unlock(&pool->mutexPool);



		pthread_mutex_lock(&pool->mutexBusy);
		++pool->busyNum;	// 池中正在工作的线程数加一
		pthread_mutex_unlock(&pool->mutexBusy);
		printf("thread %ld start working...\n", pthread_self());

		// 开始执行任务
		task.function(task.arg);
		if (task.arg) {
			free(task.arg);
			task.arg = NULL;
		}

		// 任务执行完毕, 正在工作的线程数减一
		printf("thread %ld end working...\n", pthread_self());
		pthread_mutex_lock(&pool->mutexBusy);
		--pool->busyNum;
		pthread_mutex_unlock(&pool->mutexBusy);

	}

}

void* manager(void* arg) {
	ThreadPool* pool = (ThreadPool*)arg;
	while (1) {
		// 每隔3s检测一次
		sleep(3);

		pthread_mutex_lock(&pool->mutexPool);
		if (pool->shutdown) {
			pthread_mutex_unlock(&pool->mutexPool);
			break;
		}
		int queueSize = pool->queueSize;
		int liveNum = pool->liveNum;
		pthread_mutex_unlock(&pool->mutexPool);

		pthread_mutex_lock(&pool->mutexBusy);
		int busyNum = pool->busyNum;
		pthread_mutex_unlock(&pool->mutexBusy);

		// 添加线程
		// 若任务数 > 存活线程数 && 存活线程数 < 最大线程数, 则添加两个线程
		if (queueSize > liveNum && liveNum < pool->maxNum) {
			pthread_mutex_lock(&pool->mutexPool);
			int count = 0;
			for (int i = 0; i < pool->maxNum && count < NUMBER && pool->liveNum < pool->maxNum; ++i) {
				if (pool->threadIds[i] == 0) {
					pthread_create(&pool->threadIds[i], NULL, worker, pool);
					++count;
				}
			}
			pthread_mutex_unlock(&pool->mutexPool);
		}

		// 销毁线程
		// 若正在工作的线程数 * 2 < 存活线程数 && 存活线程数 > 最少线程数
		if (busyNum * 2 < liveNum && liveNum > pool->minNum) {
			pthread_mutex_lock(&pool->mutexPool);
			for (int i = 0; i < NUMBER && pool->liveNum > pool->minNum; ++i) {
				++pool->exitNum;
				pthread_cond_signal(&pool->notEmpty);
			}
			pthread_mutex_unlock(&pool->mutexPool);
		}
	}
	printf("manager exit\n");
	pthread_exit(NULL);
}

void threadExit(ThreadPool* pool) {
	pthread_t tid = pthread_self();
	pthread_mutex_lock(&pool->mutexPool);
	for (int i = 0; i < pool->maxNum; ++i) {
		if (pool->threadIds[i] == tid) {
			pool->threadIds[i] = 0;
			printf("threadExit() called, %ld exiting...\n", tid);
			break;
		}
	}
	pthread_mutex_unlock(&pool->mutexPool);
	pthread_exit(NULL);
}
