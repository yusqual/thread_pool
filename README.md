# C/C++线程池
## 参考链接

[【线程池工作原理和实现 - 【C语言版 】C/C++】](https://www.bilibili.com/video/BV1jV411J795/?share_source=copy_web&vd_source=d82b4d9c97f08207c1489029425f087f)

[【基于C++11实现的异步线程池【C/C++】】](https://www.bilibili.com/video/BV1fw4m1r7cT/?share_source=copy_web&vd_source=d82b4d9c97f08207c1489029425f087f)

## 环境和工具
    # WSL2 (Ubuntu22.04)
    # CMake

## C11版线程池用到的头文件

### myheads/cxxthread.h
```c++
    #include <myheads/base.h>
    #include <thread>
    #include <future>
    #include <functional>
```

### myheads/base.h
```c++
    #ifndef _MYHEADS_BASE_
    #define _MYHEADS_BASE_

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <math.h>
    #include <errno.h>
    #include <unistd.h>

    // 打印错误信息msg, 默认quit = true退出程序
    static inline void err_exit(const char* msg, bool quit = true) {
        perror(msg);
        if (quit) exit(EXIT_FAILURE);
    }
    // 在err_exit基础上, 传入condition判断是否出现错误
    static inline void errif_exit(bool condition, const char* msg, bool quit = true) {
        if (condition) {
        perror(msg);
            if (quit) exit(EXIT_FAILURE);
        }
    }

    #endif  // _MYHEADS_BASE_
```