#include <iostream>
#include "threadpool.h"

// void calc(int x, int y) {
//     int z = x + y;
//     std::cout << "z = " << z << std::endl;
//     std::this_thread::sleep_for(std::chrono::seconds(1));
// }

int calc1(int x, int y) {
    int z = x + y;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return z;
}

int main() {
    ThreadPool pool;
    std::vector<std::future<int>> res;
    for (int i = 0; i < 100; ++i) {
        // auto obj = std::bind(calc, i, i);
        res.emplace_back(pool.addTask(calc1, i, i));
    }
    for (auto& item: res) {
        std::cout << "线程执行结果: " << item.get() << std::endl;
    }
    return 0;
}