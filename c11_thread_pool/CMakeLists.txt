cmake_minimum_required(VERSION 3.10)
project(c11_thread_pool)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(main main.cpp threadpool.cpp)

target_link_libraries(main pthread)