# apus

A C++17 library for commonly used data structures, optimized for performance and stable memory management.
This library is largely vibe coded with the assistance of GEMINI.

## Data Structures

### memory_arena
A PMR-based fixed-size memory arena using `std::pmr::monotonic_buffer_resource`.
- **Usage Scenario**: Ultra-fast, temporary allocations where all objects share the same lifetime and are reclaimed together.
- **Benefits**: Near-zero allocation overhead; uses a fixed stack or heap buffer defined at compile-time.

### paged_memory_arena
A growing monotonic allocator that allocates memory in fixed-size pages.
- **Usage Scenario**: Similar to `memory_arena`, but for cases where the total required memory is not known upfront and must grow dynamically.
- **Benefits**: Avoids massive reallocations by adding new pages; maintains efficiency of monotonic buffers.

### typed_memory_arena
A paged arena specialized for a single type `T`, supporting indexed access and slot reuse.
- **Usage Scenario**: Managing large collections of homogeneous objects where you need stable indices and the ability to "deallocate" and reuse individual slots.
- **Benefits**: $O(1)$ allocation and deallocation; stable pointers to elements; avoids memory fragmentation through an internal freelist.

### free_list
A template-based LIFO (Last-In, First-Out) container for managing free objects or indices.
- **Usage Scenario**: Implementing custom resource pools or memory managers where you need to track and retrieve available slots.
- **Benefits**: Simple, efficient, and generic.

### slot_map
A basic non-compacting slot map providing stable handles to objects.
- **Usage Scenario**: Ideal for Entity-Component Systems (ECS) or any system where you need "weak" pointers (handles) that can detect if the underlying object has been removed or replaced.
- **Benefits**: Protects against the ABA problem using versioning and a high-bit "dead" flag; provides stable handles that remain valid regardless of other additions or removals.

## Dependencies

- **doctest**: Fast, header-only testing framework (fetched via CMake).

## Building

```bash
mkdir -p build
cmake -S . -B build
cmake --build build
```

## Running Tests

```bash
./build/apus_tests
```

## Author(s)

[Tianyu Cheng](tianyu.cheng@utexas.edu)
