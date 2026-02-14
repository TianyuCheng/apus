cmake_minimum_required(VERSION 3.14)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(apus VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(APUS_BUILD_TESTS "Build tests" ON)
option(APUS_BUILD_BENCHMARKS "Build benchmarks" ON)

include(FetchContent)

# fetch GTest for testing
if(APUS_BUILD_TESTS)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
  )
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif()

# fetch GoogleBenchmark for benchmarking
if(APUS_BUILD_BENCHMARKS)
  FetchContent_Declare(
    google_benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.8.3
  )
  set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
  set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(google_benchmark)

  # Fetch Boost for comparison in benchmarks (Header-only usage)
  FetchContent_Declare(
    boost
    URL https://github.com/boostorg/boost/releases/download/boost-1.84.0/boost-1.84.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  FetchContent_GetProperties(boost)
  if(NOT boost_POPULATED)
    FetchContent_Populate(boost)
  endif()
endif()

# library
add_library(apus INTERFACE)
add_library(apus::apus ALIAS apus)
target_include_directories(apus INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# testing
if(APUS_BUILD_TESTS)
  add_executable(apus_tests
    tests/main.cpp
    tests/test_memory_arena.cpp
    tests/test_paged_memory_arena.cpp
    tests/test_typed_memory_arena.cpp
    tests/test_free_list.cpp
    tests/test_slot_map.cpp
    tests/test_small_vector.cpp
    tests/test_ring_buffer.cpp
  )
  target_link_libraries(apus_tests PRIVATE apus::apus GTest::gtest GTest::gtest_main)
endif()

# benchmarking
if(APUS_BUILD_BENCHMARKS)
  add_executable(apus_benchmarks
    benchmarks/main.cpp
    benchmarks/bench_memory_arena.cpp
    benchmarks/bench_paged_memory_arena.cpp
    benchmarks/bench_free_list.cpp
    benchmarks/bench_slot_map.cpp
    benchmarks/bench_small_vector.cpp
    benchmarks/bench_ring_buffer.cpp
  )
  target_link_libraries(apus_benchmarks PRIVATE apus::apus benchmark::benchmark)
  target_include_directories(apus_benchmarks PRIVATE ${boost_SOURCE_DIR})
endif()

# installation
install(TARGETS apus
  EXPORT apusTargets
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/apus
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT apusTargets
  FILE apusTargets.cmake
  NAMESPACE apus::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/apus
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/apusConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/apusConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/apusConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/apus
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/apusConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/apusConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/apus
)
