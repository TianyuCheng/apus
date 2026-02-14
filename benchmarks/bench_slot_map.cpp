#include <benchmark/benchmark.h>
#include <apus/slot_map.hpp>
#include <unordered_map>
#include <vector>
#include <random>

struct TestObject {
    uint64_t data[8]; // 64 bytes
};

static void BM_SlotMap_AddRemove(benchmark::State& state) {
    apus::slot_map<TestObject> sm;
    std::vector<apus::slot_map<TestObject>::handle> handles;
    handles.reserve(state.range(0));

    for (auto _ : state) {
        for (int i = 0; i < state.range(0); ++i) {
            handles.push_back(sm.add(TestObject{}));
        }
        for (auto h : handles) {
            sm.remove(h);
        }
        handles.clear();
    }
}
BENCHMARK(BM_SlotMap_AddRemove)->Range(8, 1024);

static void BM_UnorderedMap_AddRemove(benchmark::State& state) {
    std::unordered_map<uint32_t, TestObject> um;
    for (auto _ : state) {
        for (uint32_t i = 0; i < (uint32_t)state.range(0); ++i) {
            um[i] = TestObject{};
        }
        for (uint32_t i = 0; i < (uint32_t)state.range(0); ++i) {
            um.erase(i);
        }
        um.clear();
    }
}
BENCHMARK(BM_UnorderedMap_AddRemove)->Range(8, 1024);

static void BM_SlotMap_Iteration(benchmark::State& state) {
    apus::slot_map<TestObject> sm;
    for (int i = 0; i < state.range(0); ++i) {
        sm.add(TestObject{});
    }

    for (auto _ : state) {
        uint64_t sum = 0;
        for (const auto& obj : sm) {
            sum += obj.data[0];
        }
        benchmark::DoNotOptimize(sum);
    }
}
BENCHMARK(BM_SlotMap_Iteration)->Range(8, 4096);

static void BM_UnorderedMap_Iteration(benchmark::State& state) {
    std::unordered_map<uint32_t, TestObject> um;
    for (uint32_t i = 0; i < (uint32_t)state.range(0); ++i) {
        um[i] = TestObject{};
    }

    for (auto _ : state) {
        uint64_t sum = 0;
        for (const auto& pair : um) {
            sum += pair.second.data[0];
        }
        benchmark::DoNotOptimize(sum);
    }
}
BENCHMARK(BM_UnorderedMap_Iteration)->Range(8, 4096);
