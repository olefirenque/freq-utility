cmake_minimum_required(VERSION 3.23)

include(cmake/GoogleBenchmark.cmake)

add_executable(FreqBenchmarks
        src/freq.h
        src/freq.cpp
        src/dummy/freq_dummy.h
        src/dummy/freq_dummy.cpp
        freq_benchmarks/FreqBenchmarks.cpp)

add_custom_target(benchmarks
        COMMAND FreqBenchmarks
        DEPENDS FreqBenchmarks
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/freq_benchmarks
        )

target_link_libraries(FreqBenchmarks benchmark::benchmark)

# Link Shlwapi to the project
if ("${CMAKE_SYSTEM_NAME}" MATCHES "Windows")
    target_link_libraries(FreqBenchmarks Shlwapi)
endif ()
