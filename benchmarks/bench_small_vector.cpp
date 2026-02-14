#include <benchmark/benchmark.h>
#include <apus/small_vector.hpp>
#include <vector>

static void BM_StdVectorPushBack(benchmark::State& state)
{
    for (auto _ : state) {
        std::vector<int> v;
        for (int i = 0; i < state.range(0); ++i) {
            v.push_back(i);
        }
        benchmark::DoNotOptimize(v.data());
    }
}
BENCHMARK(BM_StdVectorPushBack)->Range(1, 128);

static void BM_SmallVectorPushBack(benchmark::State& state)
{
    for (auto _ : state) {
        apus::small_vector<int, 16> v;
        for (int i = 0; i < state.range(0); ++i) {
            v.push_back(i);
        }
        benchmark::DoNotOptimize(v.data());
    }
}
BENCHMARK(BM_SmallVectorPushBack)->Range(1, 128);

static void BM_StdVectorIterate(benchmark::State& state)
{
    std::vector<int> v;
    for (int i = 0; i < state.range(0); ++i) {
        v.push_back(i);
    }
    for (auto _ : state) {
        int sum = 0;
        for (int x : v) {
            sum += x;
        }
        benchmark::DoNotOptimize(sum);
    }
}
BENCHMARK(BM_StdVectorIterate)->Range(1, 1024);

static void BM_SmallVectorIterate(benchmark::State& state)
{
    apus::small_vector<int, 16> v;
    for (int i = 0; i < state.range(0); ++i) {
        v.push_back(i);
    }
    for (auto _ : state) {
        int sum = 0;
        for (int x : v) {
            sum += x;
        }
        benchmark::DoNotOptimize(sum);
    }
}
BENCHMARK(BM_SmallVectorIterate)->Range(1, 1024);
