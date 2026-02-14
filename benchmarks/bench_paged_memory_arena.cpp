#include <cstdlib>
#include <benchmark/benchmark.h>
#include <apus/paged_memory_arena.hpp>

static void BM_PagedArena_Allocate(benchmark::State& state)
{
    apus::paged_memory_arena<1024 * 64> arena;
    size_t                              alloc_size = state.range(0);

    for (auto _ : state) {
        void* p = arena.allocate(alloc_size);
        benchmark::DoNotOptimize(p);

        // We can't easily reset paged_arena without losing all pages,
        // but for a micro-benchmark we want to see allocation speed.
        // We'll reset every N iterations to keep it somewhat sane.
        if (state.iterations() % 100 == 0) {
            arena.reset();
        }
    }
}
BENCHMARK(BM_PagedArena_Allocate)->Arg(8)->Arg(64)->Arg(512);

static void BM_Malloc_Allocate_PagedComparison(benchmark::State& state)
{
    size_t alloc_size = state.range(0);
    for (auto _ : state) {
        void* p = std::malloc(alloc_size);
        benchmark::DoNotOptimize(p);
        std::free(p);
    }
}
BENCHMARK(BM_Malloc_Allocate_PagedComparison)->Arg(8)->Arg(64)->Arg(512);
