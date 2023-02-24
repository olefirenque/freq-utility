#include <iostream>
#include <filesystem>
#include <fstream>
#include <benchmark/benchmark.h>

#include "../src/freq.h"

constexpr std::array<std::string_view, 5> files{
    "test-1000.txt",
    "test-10000.txt",
    "test-100000.txt",
    "test-1000000.txt",
    "test-10000000.txt",
};

static void BM_CountFreqOrdinaryFile(benchmark::State &state) {
    for (auto _ : state) {
        auto file = std::string(files[state.range(0)]);
        state.SetLabel(file);
        const auto &data = process_file("../freq_benchmarks/test_cases/" + file);
    }
}

BENCHMARK(BM_CountFreqOrdinaryFile)
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Iterations(10)
    ->DenseRange(0, 4, 1)
    ->Unit(benchmark::kSecond);
BENCHMARK_MAIN();