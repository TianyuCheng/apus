config type="Debug":
    cmake -S . -B build -DCMAKE_BUILD_TYPE={{type}}

build:
    cmake --build build

test: build
    ./build/apus_tests

bench: build
    ./build/apus_benchmarks
