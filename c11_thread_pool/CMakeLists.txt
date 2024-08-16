# CMakeLists.txt

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 定义make_lib则为生成动态库, 否则生成测试程序
set(MAKE_LIB 1)

if(DEFINED MAKE_LIB)
    add_library(c11_thread_pool SHARED threadpool.cpp)
else()
    add_executable(c11_thread_pool main.cpp threadpool.cpp)
endif()

target_link_libraries(c11_thread_pool pthread)