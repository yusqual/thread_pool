#include "threadpool.h"

void taskFunc(void* arg) {
	int num = *(int*)arg;
	printf("thread %ld is working, number = %d\n", pthread_self(), num);
	sleep(1);	// 模拟工作1s
}

int main() {
	ThreadPool* pool = threadPoolCreate(3, 10, 100);
	for (int i = 0; i < 50; ++i) {
		int* arg = (int*)malloc(sizeof(int));
		*arg = i;
		threadPoolAdd(pool, taskFunc, arg);
	}
	sleep(3);
	threadPoolDestroy(pool);

	return 0;
}