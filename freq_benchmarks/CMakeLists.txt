cmake_minimum_required(VERSION 3.23)
project(freqBenchmark)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")

# Include Google Benchmark
include(cmake/GoogleBenchmark.cmake)

# Add source to this project's executable.
add_executable(FreqBenchmarks
        "../src/freq.h"
        "../src/freq.cpp"
        "FreqBenchmarks.cpp" FreqBenchmarks.cpp)

# Link Google Benchmark to the project
target_link_libraries(FreqBenchmarks benchmark::benchmark)

# Link Shlwapi to the project
if ("${CMAKE_SYSTEM_NAME}" MATCHES "Windows")
    target_link_libraries(FreqBenchmarks Shlwapi)
endif ()
