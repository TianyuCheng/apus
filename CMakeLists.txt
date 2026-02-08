cmake_minimum_required(VERSION 3.14)
project(apus VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Fetch doctest for testing
FetchContent_Declare(
  doctest
  GIT_REPOSITORY https://github.com/doctest/doctest.git
  GIT_TAG        v2.4.11
)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

FetchContent_MakeAvailable(doctest)

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
target_link_libraries(apus_tests PRIVATE apus doctest::doctest)