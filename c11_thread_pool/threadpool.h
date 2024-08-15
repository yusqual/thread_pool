#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include <myheads/cxxthread.h>
#include <iostream>
#include <vector>
#include <queue>
#include <map>

// 每次增删线程数量
#define CHANGE_NUM 2

class ThreadPool {
public:
    ThreadPool(uint32_t min = 2, uint32_t max = std::thread::hardware_concurrency());
    ~ThreadPool();
    void addTask(std::function<void(void)> task);
    template<typename Func, typename... Args>
    auto addTask(Func&& func, Args&&... args) -> std::future<decltype(func(args...))> {
        // 1. 创建package_task实例
        using resType = decltype(func(args...));
        // func 是一个未定义类型, 可能是左值也可能是右值, 将其类型完美保存下来需要使用forward完美转发
        auto mytask = std::make_shared<std::packaged_task<resType()>> (std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
        // 2. 得到future
        std::future<resType> res = mytask->get_future();
        // 3. 任务函数添加到任务队列
        m_queue_mutex.lock();
        m_tasks.emplace([mytask]() {
            (*mytask)();
        });
        m_queue_mutex.unlock();
        m_consume_cv.notify_one();

        return res;
    }

private:
    void managerTask(void);
    void workerTask(void);

private:
    std::thread* m_manager;                            // 管理者线程
    std::map<std::thread::id, std::thread> m_workers;  // 工作线程
    std::vector<std::thread::id> m_ids;                // 记录要退出的线程的id
    int m_min_size;                                    // 最少工作线程数
    int m_max_size;                                    // 最多工作线程数
    std::atomic<int> m_idle_size;                      // 空闲工作线程数
    std::atomic<int> m_curr_size;                      // 存活工作线程数
    std::atomic<int> m_exit_size;                      // 需要销毁线程数
    std::atomic<bool> m_stop;                          // 线程池是否关闭

    std::queue<std::function<void(void)>> m_tasks;  // 任务队列
    std::mutex m_queue_mutex;                       // 任务队列锁
    std::mutex m_ids_mutex;                         // m_ids锁
    std::condition_variable m_consume_cv;           // 工作线程条件变量
};
/**
 * 构成:
 *  管理者线程子线程，1个
 *   控制工作线程的数量: 增加或减少
 *  若干工作线程 -> 子线程n个
 *   从任务队列中取任务，并处理
 *   任务队列为空，被阻塞(被条件变量阻塞)
 *   线程同步(互斥锁)
 *   当前数量, 空闲的线程数量
 *   最小, 量最大线程数量
 *  任务队列 -> stl -> queue
 *   互斥锁
 *   条件变量
 *  线程池开关 -> bool
 */

#endif  // _THREADPOOL_H_