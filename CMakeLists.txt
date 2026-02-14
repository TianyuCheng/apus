cmake_minimum_required(VERSION 3.14)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(apus VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# Fetch GTest for testing
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)

FetchContent_Declare(
  google_benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG        v1.8.3
)

set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest google_benchmark)

# Library
add_library(apus INTERFACE)
target_include_directories(apus INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Testing
add_executable(apus_tests
    tests/main.cpp
    tests/test_memory_arena.cpp
    tests/test_paged_memory_arena.cpp
    tests/test_typed_memory_arena.cpp
    tests/test_free_list.cpp
    tests/test_slot_map.cpp
)
target_link_libraries(apus_tests PRIVATE apus GTest::gtest GTest::gtest_main)
# Benchmarking
add_executable(apus_benchmarks
    benchmarks/main.cpp
    benchmarks/bench_memory_arena.cpp
    benchmarks/bench_paged_memory_arena.cpp
    benchmarks/bench_free_list.cpp
    benchmarks/bench_slot_map.cpp
)
target_link_libraries(apus_benchmarks PRIVATE apus benchmark::benchmark)
