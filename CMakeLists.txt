cmake_minimum_required(VERSION 3.23)
project(freq)

set(CMAKE_CXX_STANDARD 23)

#add_compile_options(-fsanitize=address)
#add_link_options(-fsanitize=address)

add_subdirectory(freq_benchmarks)
add_executable(freq
        src/threadpool.h
        src/unordered_dense.h
        src/freq.h
        src/freq.cpp
        src/main.cpp)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(freq PRIVATE Threads::Threads)
