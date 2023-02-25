# Disable the Google Benchmark requirement on Google Test
set(BENCHMARK_ENABLE_TESTING NO)

include(FetchContent)

FetchContent_Declare(
        googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG origin/main
        GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(googlebenchmark)
