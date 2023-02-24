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
    ->DenseRange(0, files.size() - 1, 1) \
    ->Unit(benchmark::kSecond);

constexpr size_t ITERATIONS = 3;

constexpr std::array<std::string_view, 5> files{
    "test-1000.txt",
    "test-10000.txt",
    "test-100000.txt",
    "test-1000000.txt",
    "test-10000000.txt",
};

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

BASE_FREQ_BENCHMARK(BM_BaseCountFreq, process_file, dict_words);
BASE_FREQ_BENCHMARK(BM_BaseCountFreq, process_file, single_word);
BASE_FREQ_BENCHMARK(BM_BaseCountFreq, process_file, one_word_dict);
BASE_FREQ_BENCHMARK(BM_BaseCountFreq, process_file, unique_words);
BASE_FREQ_BENCHMARK(BM_BaseCountFreq, process_file_dummy, dict_words);
BASE_FREQ_BENCHMARK(BM_BaseCountFreq, process_file_dummy, single_word);
BASE_FREQ_BENCHMARK(BM_BaseCountFreq, process_file_dummy, one_word_dict);
BASE_FREQ_BENCHMARK(BM_BaseCountFreq, process_file_dummy, unique_words);

BENCHMARK_MAIN();