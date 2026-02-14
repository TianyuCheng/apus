#include <benchmark/benchmark.h>
#include <apus/memory_arena.hpp>
#include <cstdlib>
#include <memory>

// Benchmarking raw allocation speed of memory_arena vs malloc
static void BM_MemoryArena_Allocate(benchmark::State& state) {
    // 1MB Arena
    apus::memory_arena<1024 * 1024> arena;
    size_t alloc_size = state.range(0);

    for (auto _ : state) {
        void* p = arena.allocate(alloc_size);
        benchmark::DoNotOptimize(p);
        
        // Reset every iteration for this micro-benchmark to ensure consistent state
        // and avoid overflow. monotonic_buffer_resource reset is very fast.
        arena.reset();
    }
}
BENCHMARK(BM_MemoryArena_Allocate)->Arg(8)->Arg(64)->Arg(512);

static void BM_Malloc_Allocate(benchmark::State& state) {
    size_t alloc_size = state.range(0);
    for (auto _ : state) {
        void* p = std::malloc(alloc_size);
        benchmark::DoNotOptimize(p);
        std::free(p);
    }
}
BENCHMARK(BM_Malloc_Allocate)->Arg(8)->Arg(64)->Arg(512);

// Benchmarking the cost of freeing many objects vs arena reset
static void BM_MemoryArena_Reset(benchmark::State& state) {
    apus::memory_arena<1024 * 1024> arena;
    const size_t num_allocs = state.range(0);
    
    for (auto _ : state) {
        state.PauseTiming();
        for(size_t i = 0; i < num_allocs; ++i) {
            arena.allocate(16);
        }
        state.ResumeTiming();
        
        arena.reset();
    }
}
BENCHMARK(BM_MemoryArena_Reset)->Range(8, 8192);

static void BM_Vector_Clear(benchmark::State& state) {
    const size_t num_allocs = state.range(0);
    std::vector<void*> ptrs;
    ptrs.reserve(num_allocs);

    for (auto _ : state) {
        state.PauseTiming();
        for(size_t i = 0; i < num_allocs; ++i) {
            ptrs.push_back(std::malloc(16));
        }
        state.ResumeTiming();

        for(void* p : ptrs) {
            std::free(p);
        }
        ptrs.clear();
    }
}
BENCHMARK(BM_Vector_Clear)->Range(8, 8192);
