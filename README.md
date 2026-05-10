# mSkipList
[English](README.md) | [中文](README_zh.md)

A generic Skip List data structure implemented in C++20, supporting multi-field data storage, arbitrary field as primary key, and custom memory allocation strategies.

## Features

- **Header-only**: Single header file `mSkipList.hpp` — include and use
- **Modern C++20 syntax**: Utilizes Concepts, Requires clauses, fold expressions, etc.
- **Multi-field data support**: Store any number of fields of any type via variadic templates
- **Flexible primary key selection**: Designate any field as the sorting and lookup key through template parameters
- **Duplicate key update mechanism**: Automatically overwrites existing data when inserting a duplicate key, instead of creating a new node
- **Custom allocator**: Built-in memory pool + free list to reduce frequent allocation overhead
- **Tunable indexing parameters**: Supports custom `gap` (index interval) and `max_deep` (maximum level)
- **Sentinel node design**: Simplifies boundary condition handling via head and tail sentinels
- **mArray index storage optimization**: Uses a hybrid structure of stack array + fallback heap vector to reduce heap allocation overhead in low-index scenarios
- **STL-style iterators**: Provides `begin()` / `end()` bidirectional iterators supporting range traversal
- **Range query (Range)**: Supports returning sub-range iterators by primary key range for interval traversal

## Notes
- If using iterators, erasing the node pointed to by the current iterator via `erase(key)` and then dereferencing the iterator results in undefined behavior
- Iterators return an internal `view` variable; if needed, hold it by value copy so it still points to the old node data after the iterator moves

## Compilation Requirements

- A compiler supporting C++20 (only tested with GCC 13+)
- No third-party dependencies

## Quick Start

```cpp
#include "mSkipList.hpp"
#include <iostream>
#include <string>

using namespace msl;

int main() {
    // Use field 0 (int) as primary key, store <id, name, score>
    auto list = make_mSkipList<0, int, std::string, double>();

    // Insert data
    list.insert(1, "Alice", 95.5);
    list.insert(2, "Bob", 87.0);
    list.insert(3, "Charlie", 92.3);

    // Query: get<field_type>(key, field_index)
    const std::string& name = list.get<std::string>(1, 1);
    std::cout << "Name for ID=1: " << name << std::endl;  // Alice

    // Duplicate key will overwrite
    list.insert(1, "AliceUpdated", 98.0);

    // Check existence
    if (list.contain(2)) {
        std::cout << "Contains ID=2" << std::endl;
    }

    // Range traversal (via iterators)
    for (auto it = list.begin(); it != list.end(); ++it) {
        auto data = *it;
        std::cout << std::get<0>(data.data()) << std::endl;
    }

    // Range query: iterate all nodes with primary key in the [1, 3] closed interval
    for (auto data : list.range(1, 3)) {
        std::cout << std::get<1>(data.data()) << std::endl;
    }

    // Delete
    list.erase(2);
    std::cout << "Current size: " << list.size() << std::endl;  // 2

    return 0;
}
```

Compile:
```bash
g++ -std=c++20 main.cpp -O3 -o main
```

## Construction Methods

### Factory Function (Recommended)
```cpp
auto list = make_mSkipList<0, int, std::string, double>();
```

### Explicit Construction
```cpp
// Construct sentinel nodes with default values
mSkipList<0, int, std::string, double> list(0, "", 0.0);
```

### Custom Parameters
```cpp
// Parameters: memory pool size, gap, max_deep, sentinel initial values...
mSkipList<0, int, std::string> list(1024, 2, 5, 0, "");
```

| Parameter | Description | Default |
|-----------|-------------|---------|
| `allocate_size` | Memory pool block size | 4096 |
| `gap` | Number of data nodes between adjacent index nodes | 3 |
| `max_deep` | Maximum skip list level (-1 for unlimited) | -1 |

## Non-First-Field Primary Key

You can specify any field as the primary key through the `keyIndex` template parameter:

```cpp
// Use field 1 (std::string) as primary key
mSkipList<1, int, std::string, double> list(0, "", 0.0);

list.insert(101, "Alice", 95.0);
list.insert(102, "Bob", 88.5);

// Query via string primary key
const int& id = list.get<int>(std::string("Alice"), 0);
```

## API Reference

| Method | Description |
|--------|-------------|
| `insert(T_D... args)` | Insert data; overwrites if primary key already exists |
| `get<Type>(key, field_index)` | Query a reference to the specified field by primary key |
| `erase(key)` / `erase(it)` | Delete node with the specified primary key |
| `contain(key)` | Check whether the specified primary key exists |
| `begin()` / `end()` | Return head and tail iterators, supporting range traversal |
| `range(left, right)` | Return sub-range iterators by primary key range (`[left, right]` closed interval) |
| `size()` | Return current node count |
| `get_deep()` | Return current skip list level count |
| `max_deep()` / `set_max_deep(n)` | Get/set maximum level limit |
| `gap()` / `set_gap(n)` | Get/set index interval |
| `anew_build()` | Rebuild index (call after modifying parameters) |

## Tests

The project includes three test suites (O3 optimized):

### Functional Tests (test0.cpp)
Covers 9 test groups, 59 test cases:

- Construction and factory functions
- Basic CRUD operations
- Duplicate key update mechanism
- Boundary conditions and exceptional behavior
- Skip list index structure parameter verification
- Large-scale random operations and correctness (3000 nodes)
- Non-first-field as primary key
- High-intensity alternating operation stability (50 rounds x 100 nodes)
- Cross-type field combinations
```
# 59/59 passed
```

### Iterator Tests (test1.cpp)
Covers 11 iterator-specific test cases:

- Forward traversal (`operator*`, `operator++`)
- Backward traversal (`operator--`)
- Range `for` loop (C++11 syntax, pass-by-value)
- Prefix `++` vs postfix `++` semantic distinction
- Prefix `--` vs postfix `--` semantic distinction
- Iterator comparison operators (`==` and `!=`)
- Out-of-bounds access exception safety (`++end()`, `--begin()`, `(end)++` all throw exceptions)
- Range query iteration (`range(left, right)` closed interval)
- Empty list iteration behavior (`begin == end`)
- Iterator-based deletion (`erase`)
- Iterator tag verification (`bidirectional_iterator_tag`)
```
# 11/11 passed
```

### Performance Tests (test2.cpp)

> Environment: GCC 13+, `-O3`, 1 million records, key range 1~500,000, average of 5 runs.  
> See `test2.cpp` for test code; `std::map` uses `find()` for pure lookup and `erase()` for pure deletion, avoiding the insertion side effect of `operator[]`.

| Scenario | Operation | std::map | mSkipList | Ratio (SkipList/map) |
|----------|-----------|----------|-----------|---------------------|
| **Random data** | Insert | ~155 ms | ~400 ms | **~2.6x** |
| | Lookup | ~270 ms | ~395 ms | **~1.5x** |
| | Delete | ~55 ms | ~102 ms | **~1.9x** |
| **Sequential data** | Insert | ~183 ms | ~177 ms | **~0.97x** |
| | Lookup | ~100 ms | ~76 ms | **~0.76x** |
| | Delete | ~57 ms | ~115 ms | **~2.0x** |

### Micro-Benchmarks (test3.cpp)

Using rigorous test methodology with isolated instances + cache warm-up + compiler optimization elimination prevention (test3.cpp), data scale 50,000, default parameters (gap=3, max_deep=-1):

| Operation | Min | Max | Avg | Description |
|-----------|-----|-----|-----|-------------|
| Lookup - Existing key | 80 ns | 3215 ns | **356 ns** | Random key hit, 3000 samples |
| Lookup - Non-existing key | 92 ns | 3472 ns | **116 ns** | Random key miss, longer search path |
| Lookup - First node | 317 ns | - | 317 ns | Minimum key, single test |
| Lookup - Last node | 458 ns | - | 458 ns | Maximum key, single test |
| Delete - Random key | 492 ns | 2497 ns | **826 ns** | Isolated instance method, 60 samples |

Degraded scenario (gap=50000, max_deep=1, forced degradation to singly linked list):

| Operation | Min | Max | Avg | Description |
|-----------|-----|-----|-----|-------------|
| Lookup - Degraded list | 146 ns | 114359 ns | **53194 ns** | O(n) linear scan |
| Delete - Degraded list | 423636 ns | 725688 ns | **501384 ns** | O(n) linear deletion |

## Test Verification

| Check Item | Status |
|------------|--------|
| Functional completeness tests | 59/59 passed |
| AddressSanitizer (memory leak detection) | Passed |
| Code coverage | Function coverage 95% (114/120) |

## Design Highlights

- **Indexing strategy**: Unlike traditional skip lists' random promotion, uses a deterministic interval strategy (promote one level every `gap` nodes) to guarantee index structure stability
- **Dynamic index maintenance**: Builds indexes bottom-up via bubbling during insertion, intelligently maintains adjacent node connections during deletion
- **Data storage**: Uses `std::tuple` to store multi-field data, with pointer arrays enabling fast access to each field by index
- **Index storage optimization (mArray)**: Skip list nodes typically have few levels; uses a fixed-size on-stack array (default 5) to store indexes, falling back to heap vector when exceeded, significantly reducing memory footprint and heap allocation overhead for low-level nodes
- **Memory management**: Pre-allocated memory pool reduces system calls, free list recycles deleted nodes for reuse

## License

MIT License  
Copyright (c) 2026 Ximiaw
