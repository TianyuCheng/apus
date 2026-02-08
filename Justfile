config:
    cmake -S . -B build

build:
    cmake --build build

test:
    ./build/apus_tests
