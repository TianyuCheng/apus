#include <benchmark/benchmark.h>
#include <apus/ring_buffer.hpp>
#include <boost/circular_buffer.hpp>

static void BM_BoostCircularBuffer_PushBack(benchmark::State& state)
{
    boost::circular_buffer<int> rb(state.range(0));
    for (auto _ : state) {
        for (int i = 0; i < state.range(0); ++i) {
            rb.push_back(i);
        }
        benchmark::DoNotOptimize(rb.array_one().first);
    }
}
BENCHMARK(BM_BoostCircularBuffer_PushBack)->Range(8, 1024);

static void BM_ApusRingBuffer_PushBack(benchmark::State& state)
{
    apus::ring_buffer<int> rb(state.range(0));
    for (auto _ : state) {
        for (int i = 0; i < state.range(0); ++i) {
            rb.push_back(i);
        }
        benchmark::DoNotOptimize(rb.begin());
    }
}
BENCHMARK(BM_ApusRingBuffer_PushBack)->Range(8, 1024);

static void BM_BoostCircularBuffer_Iterate(benchmark::State& state)
{
    boost::circular_buffer<int> rb(state.range(0));
    for (int i = 0; i < state.range(0); ++i) rb.push_back(i);
    for (auto _ : state) {
        int sum = 0;
        for (int x : rb) {
            sum += x;
        }
        benchmark::DoNotOptimize(sum);
    }
}
BENCHMARK(BM_BoostCircularBuffer_Iterate)->Range(8, 1024);

static void BM_ApusRingBuffer_Iterate(benchmark::State& state)
{
    apus::ring_buffer<int> rb(state.range(0));
    for (int i = 0; i < state.range(0); ++i) rb.push_back(i);
    for (auto _ : state) {
        int sum = 0;
        for (int x : rb) {
            sum += x;
        }
        benchmark::DoNotOptimize(sum);
    }
}
BENCHMARK(BM_ApusRingBuffer_Iterate)->Range(8, 1024);
