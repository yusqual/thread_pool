# CMakeLists.txt

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")

# 定义make_lib则为生成动态库, 否则生成测试程序
set(MAKE_LIB 1)

if(DEFINED MAKE_LIB)
    add_library(c_thread_pool SHARED threadpool.c)
else()
    add_executable(c_thread_pool main.c threadpool.c)
endif()

target_link_libraries(c_thread_pool pthread)
