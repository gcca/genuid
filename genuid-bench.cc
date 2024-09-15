#include <benchmark/benchmark.h>

#include "genuid.hpp"

static void BM_LockTier(benchmark::State &state) {
  genuid::Init();
  for (auto _ : state) { std::size_t uid = genuid::LockTierUID(); }
}

static void BM_LockFree(benchmark::State &state) {
  genuid::Init();
  for (auto _ : state) { std::size_t uid = genuid::LockFreeUID(); }
}

BENCHMARK(BM_LockTier);
BENCHMARK(BM_LockFree);

BENCHMARK_MAIN();
