#include "threadpool.h"

ThreadPool::ThreadPool(uint32_t min, uint32_t max)
    : m_min_size(min), m_max_size(max), m_idle_size(min), m_curr_size(min), m_exit_size(0), m_stop(false) {
    m_manager = new std::thread(&ThreadPool::managerTask, this);
    for (int i = 0; i < min; ++i) {
        std::thread t(&ThreadPool::workerTask, this);
        m_workers.insert(std::make_pair(t.get_id(), std::move(t)));
    }
}

ThreadPool::~ThreadPool() {
    m_stop.store(true);
    m_consume_cv.notify_all();
    for (auto& it : m_workers) {
        std::thread& t = it.second;
        if (t.joinable()) {
            std::cout << "析构 线程即将退出 : " << t.get_id() << std::endl;
            t.join();
        }
    }
    if (m_manager->joinable()) {
        std::cout << "manager即将退出" << std::endl;
        m_manager->join();
    }
    delete m_manager;
}

void ThreadPool::addTask(std::function<void(void)> task) {
    if (!m_stop) {
        {
            std::lock_guard<std::mutex> locker(m_queue_mutex);
            m_tasks.emplace(task);
        }
        m_consume_cv.notify_one();
    } else {
        throw std::runtime_error("threadPool stoped.");
    }
}

void ThreadPool::managerTask(void) {
    while (!m_stop) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        int idle_size = m_idle_size;
        int curr_size = m_curr_size;

        if (idle_size * 2 > curr_size && curr_size > m_min_size) {
            m_exit_size.store(CHANGE_NUM);
            m_consume_cv.notify_all();
            std::lock_guard<std::mutex> locker(m_ids_mutex);
            if (m_ids.empty()) continue;
            for (const auto& id : m_ids) {
                auto it = m_workers.find(id);
                if (it != m_workers.end()) {
                    std::cout << "管理者销毁线程 : " << (*it).first << std::endl;
                    if ((*it).second.joinable()) (*it).second.join();
                    m_workers.erase(it);
                }
            }
            m_ids.clear();
        } else if (idle_size == 0 && curr_size < m_max_size) {
            std::thread t(&ThreadPool::workerTask, this);
            m_workers.insert(std::make_pair(t.get_id(), std::move(t)));
            ++m_curr_size;
            ++m_idle_size;
        }
    }
}

void ThreadPool::workerTask(void) {
    while (!m_stop) {
        std::function<void()> task = nullptr;
        {
            std::unique_lock<std::mutex> locker(m_queue_mutex);
            while (m_tasks.empty() && !m_stop) {
                m_consume_cv.wait(locker);
                if (m_exit_size > 0) {
                    --m_exit_size;
                    if (m_curr_size > m_min_size) {
                        --m_curr_size;
                        --m_idle_size;
                        std::cout << "工作线程函数退出 : " << std::this_thread::get_id() << std::endl;
                        std::unique_lock<std::mutex> locker(m_ids_mutex);
                        m_ids.emplace_back(std::this_thread::get_id());
                        return;
                    }
                }
            }
            if (!m_tasks.empty()) {
                // std::cout << "取出一个任务" << std::endl;
                task = std::move(m_tasks.front());
                m_tasks.pop();
            }
        }
        if (task) {
            --m_idle_size;
            task();
            ++m_idle_size;
        }
    }
}
