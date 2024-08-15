# CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(c_thread_pool)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")

add_executable(main main.c threadpool.c)

target_link_libraries(main pthread)