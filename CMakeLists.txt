cmake_minimum_required(VERSION 3.23)
project(freq)

set(CMAKE_CXX_STANDARD 20)

#add_compile_options(-fsanitize=address)
#add_link_options(-fsanitize=address)

include(${CMAKE_SOURCE_DIR}/cmake/FindLibaio.cmake)
if (LIBAIO_FOUND)
    message(STATUS "Find libaio include:${LIBAIO_INCLUDE_DIR} libs:${LIBAIO_LIBRARIES}")
    add_definitions("-DHAS_LIBAIO")
    include_directories(${LIBAIO_INCLUDE_DIR})
    link_libraries(${LIBAIO_LIBRARIES})
else (LIBAIO_FOUND)
    message(WARNING "Cannot find libaio, posix aio is used instead")
endif (LIBAIO_FOUND)

include(freq_benchmarks/CMakeLists.txt)
include(freq_tests/CMakeLists.txt)

add_executable(freq
        src/threadpool.h
        src/unordered_dense.h
        src/freq.h
        src/freq.cpp
        src/utils.h
        src/main.cpp)

add_custom_target(run
        COMMAND freq
        DEPENDS freq
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(freq PRIVATE Threads::Threads)
