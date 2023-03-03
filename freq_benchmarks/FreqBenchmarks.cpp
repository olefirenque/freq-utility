#include <iostream>
#include <filesystem>
#include <fstream>
#include <benchmark/benchmark.h>

#include "../src/freq.h"
#include "../src/dummy/freq_dummy.h"

#define BASE_FREQ_BENCHMARK(TARGET, FUNCTION, TEST_DIR) \
    BENCHMARK_CAPTURE(TARGET, TEST_DIR, FUNCTION, #TEST_DIR) \
    ->Name(#FUNCTION"/"#TEST_DIR) \
    ->MeasureProcessCPUTime() \
    ->UseRealTime() \
    ->Iterations(ITERATIONS) \
    ->DenseRange(FILES_RANGE_START, FILES_RANGE_END, 1) \
    ->Unit(benchmark::kSecond);

constexpr std::array<std::string_view, 5> files{
    "test-1000.txt",
    "test-10000.txt",
    "test-100000.txt",
    "test-1000000.txt",
    "test-10000000.txt",
};

constexpr size_t ITERATIONS = 3;
constexpr size_t FILES_RANGE_START = files.size() - 1; // default 0;
constexpr size_t FILES_RANGE_END = files.size() - 1;

template<typename F>
static void run(benchmark::State &state, F f, const std::string &test_dir) {
    for (auto _ : state) {
        auto file = std::string(files[state.range(0)]);
        state.SetLabel(file);
        const auto &data = f(test_dir + file);
    }
}

template <class ...Args>
static void BM_BaseCountFreq(benchmark::State &state, Args&&... args) {
    auto args_tuple = std::make_tuple(std::move(args)...);
    auto func = std::get<0>(args_tuple);
    const std::string test_dir(std::get<1>(args_tuple));

    run(state, func, "../test_cases/" + test_dir + "/");
}

BASE_FREQ_BENCHMARK(BM_BaseCountFreq, process_file_blocking_read, dict_words);
#ifdef ENABLE_PROCESS_MMAPED_FILE
BASE_FREQ_BENCHMARK(BM_BaseCountFreq, process_mmaped_file, dict_words);
#endif
#ifdef HAS_LIBAIO
BASE_FREQ_BENCHMARK(BM_BaseCountFreq, process_file_aio, dict_words);
#endif

BENCHMARK_MAIN();