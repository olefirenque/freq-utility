cmake_minimum_required(VERSION 3.23)
project(freq)

set(CMAKE_CXX_STANDARD 23)

#add_compile_options(-fsanitize=address)
#add_link_options(-fsanitize=address)

add_executable(freq threadpool.h main.cpp)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(freq PRIVATE Threads::Threads)
