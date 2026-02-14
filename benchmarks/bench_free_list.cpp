#include <benchmark/benchmark.h>
#include <apus/free_list.hpp>
#include <vector>

static void BM_FreeList_PushPop(benchmark::State& state) {
    apus::free_list<int> fl;
    fl.reserve(state.range(0));
    for (auto _ : state) {
        for (int i = 0; i < state.range(0); ++i) {
            fl.push_back(i);
        }
        for (int i = 0; i < state.range(0); ++i) {
            benchmark::DoNotOptimize(fl.pop_back());
        }
    }
}
BENCHMARK(BM_FreeList_PushPop)->Range(8, 4096);

static void BM_Vector_PushPop(benchmark::State& state) {
    std::vector<int> v;
    v.reserve(state.range(0));
    for (auto _ : state) {
        for (int i = 0; i < state.range(0); ++i) {
            v.push_back(i);
        }
        for (int i = 0; i < state.range(0); ++i) {
            int val = v.back();
            v.pop_back();
            benchmark::DoNotOptimize(val);
        }
    }
}
BENCHMARK(BM_Vector_PushPop)->Range(8, 4096);
