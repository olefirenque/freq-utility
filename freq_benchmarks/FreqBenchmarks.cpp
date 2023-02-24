#include <iostream>
#include <filesystem>
#include <fstream>
#include <benchmark/benchmark.h>

#include "../src/freq.h"
#include "../src/dummy/freq_dummy.h"

#define BASE_FREQ_BENCHMARK(TARGET) \
    BENCHMARK(TARGET) \
    ->MeasureProcessCPUTime() \
    ->UseRealTime() \
    ->Iterations(ITERATIONS) \
    ->DenseRange(0, 4, 1) \
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

static void BM_CountDummyFreqDictWords(benchmark::State &state) {
    run(state, process_file_dummy, "../test_cases/dict_words/");
}

static void BM_CountDummyFreqSingleWord(benchmark::State &state) {
    run(state, process_file_dummy, "../test_cases/dict_words/");
}

static void BM_CountFreqDictWords(benchmark::State &state) {
    run(state, process_file, "../test_cases/dict_words/");
}

static void BM_CountFreqSingleWord(benchmark::State &state) {
    run(state, process_file, "../test_cases/single_word/");
}

BASE_FREQ_BENCHMARK(BM_CountDummyFreqDictWords);
BASE_FREQ_BENCHMARK(BM_CountDummyFreqSingleWord);
BASE_FREQ_BENCHMARK(BM_CountFreqDictWords);
BASE_FREQ_BENCHMARK(BM_CountFreqSingleWord);

BENCHMARK_MAIN();