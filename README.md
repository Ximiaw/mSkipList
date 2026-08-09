# mSkipList
[English](README.md) | [中文](README_zh.md)

A generic skip list data structure implemented in C++20, supporting multi-field data storage, arbitrary field as primary key, and custom memory allocation strategies.

## Features

- **Header-only**: Single header file `mSkipList.hpp` — include and use
- **Modern C++20 syntax**: Uses Concepts, Requires clauses, fold expressions, and more
- **Multi-field data support**: Stores any type and any number of fields via variadic templates
- **Flexible primary key selection**: Any field can be specified as the sorting and lookup key via template parameters
- **Duplicate key update mechanism**: Inserting an existing key automatically overwrites the data instead of creating a new node
- **Custom memory allocator**: Built-in memory pool + free list to reduce frequent allocation overhead
- **Tunable index parameters**: Supports custom `gap` (index interval) and `max_deep` (maximum level)
- **Sentinel node design**: Simplifies boundary condition handling via head and tail sentinels
- **mArray index storage optimization**: Uses a hybrid structure of stack array + fallback heap vector to reduce heap allocation overhead in low-index scenarios
- **STL-style iterators**: Provides `begin()` / `end()` bidirectional iterators for range traversal
- **Range queries (Range)**: Supports returning sub-range iterators by primary key range for interval traversal

## Notes
- When using iterators, erasing the node pointed to by the current iterator via `erase(key)` and then dereferencing the iterator is undefined behavior
- Iterators return their internal `view` variable; if needed, hold it by value copy so it still points to the old node data after the iterator moves

## Build Requirements

- A C++20-compatible compiler (only tested on GCC 13+)
- No third-party dependencies

## Quick Start

```cpp
#include "mSkipList.hpp"
#include <iostream>
#include <string>

using namespace msl;

int main() {
    // Use the 0th field (int) as the primary key, storing <id, name, score>
    auto list = make_mSkipList<0, int, std::string, double>();

    // Insert data
    list.insert(1, "Alice", 95.5);
    list.insert(2, "Bob", 87.0);
    list.insert(3, "Charlie", 92.3);

    // Query: get<field_type>(key, field_index)
    const std::string& name = list.get<std::string>(1, 1);
    std::cout << "Name for ID=1: " << name << std::endl;  // Alice

    // Duplicate key will be overwritten
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

    // Range query: traverse all nodes with primary key in the closed interval [1, 3]
    for (auto data : list.range(1, 3)) {
        std::cout << std::get<1>(data.data()) << std::endl;
    }

    // Erase
    list.erase(2);
    std::cout << "Current size: " << list.size() << std::endl;  // 2

    return 0;
}
```

Compile:
```bash
g++ -std=c++20 main.cpp -O3 -o main
```

## Custom Struct Key + Range Query

```cpp
#include "mSkipList.hpp"
#include <iostream>

using namespace msl;
using namespace std;

struct task {
    size_t priority = 0;
    size_t number = 0;
    task(size_t p = 0, size_t n = 0) : priority(p), number(n) {};

    bool operator==(const task& other) const {
        return priority == other.priority && number == other.number;
    }
    auto operator<=>(const task& other) const = default;
};

int main() {
    auto sl = make_mSkipList<0, task, int>();

    for (size_t i = 0; i < 5; i++) {
        for (size_t j = 0; j < 10; j++) {
            sl.insert(task{i, j}, i + j);
        }
    }

    for (auto it : sl.range(task{3, 3}, task{4, 2})) {
        cout << "Priority: " << get<0>(it.data()).priority
             << "\tNumber: " << get<0>(it.data()).number
             << "\tTask: " << it.ref<int>(1) << endl;
    }
}
```

Output:
```
Priority: 3	Number: 3	Task: 6
Priority: 3	Number: 4	Task: 7
Priority: 3	Number: 5	Task: 8
Priority: 3	Number: 6	Task: 9
Priority: 3	Number: 7	Task: 10
Priority: 3	Number: 8	Task: 11
Priority: 3	Number: 9	Task: 12
Priority: 4	Number: 0	Task: 4
Priority: 4	Number: 1	Task: 5
Priority: 4	Number: 2	Task: 6
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
| `max_deep` | Maximum skip list levels (-1 for unlimited) | -1 |

## Non-first-field Primary Key

Any field can be designated as the primary key via the `keyIndex` template parameter:

```cpp
// Use the 1st field (std::string) as the primary key
mSkipList<1, int, std::string, double> list(0, "", 0.0);

list.insert(101, "Alice", 95.0);
list.insert(102, "Bob", 88.5);

// Query via string primary key
const int& id = list.get<int>(std::string("Alice"), 0);
```

## API Reference

| Method | Description |
|--------|-------------|
| `insert(T_D... args)` | Insert data; overwrites if the primary key already exists |
| `get<Type>(key, field_index)` | Query the reference of a specified field by primary key |
| `erase(key)` / `erase(it)` | Erase the node with the specified key |
| `contain(key)` | Check if the specified primary key exists |
| `begin()` / `end()` | Return head and tail iterators, supporting range traversal |
| `range(left, right)` | Return sub-range iterators by primary key range (closed interval `[left, right]`) |
| `prefix_to(key)` | Return prefix range from the first node to the specified key |
| `suffix_from(key)` | Return suffix range from the specified key to the tail node |
| `front<Type>(field_index)` | Get the specified field of the first node |
| `back<Type>(field_index)` | Get the specified field of the last node |
| `pop_front()` / `pop_back()` | Erase the first / last node |
| `size()` | Return the current number of nodes |
| `get_deep()` | Return the current skip list level count |
| `max_deep()` / `set_max_deep(n)` | Get / set the maximum level limit |
| `gap()` / `set_gap(n)` | Get / set the index interval |
| `anew_build()` | Rebuild the index (call after modifying parameters) |

## Tests

The project includes four test suites (O3 optimized):

### Functional Test (test_functional.cpp)
Covers 9 major test groups, 59 test cases:

- Construction and factory functions
- Basic CRUD operations
- Duplicate key update mechanism
- Boundary conditions and exceptional behavior
- Skip list index structure parameter validation
- Large-scale random operations and correctness (3000 nodes)
- Non-first field as primary key
- High-intensity alternating operation stability (50 rounds x 100 nodes)
- Cross-type field combinations
```
# 59/59 passed
```

### Iterator Test (test_iterator.cpp)

Covers **20** iterator-specific test groups:

| # | Test Item | Description |
|---|-----------|-------------|
| 1 | Forward traversal | `operator*` dereferencing and prefix `operator++` |
| 2 | Reverse traversal | `operator--` and `operator->` |
| 3 | Range-for | C++11 syntax sugar, pass-by-value View |
| 4 | Prefix++ vs Postfix++ | Semantic difference validation (return value vs iterator position) |
| 5 | Prefix-- vs Postfix-- | Semantic difference validation (return value vs iterator position) |
| 6 | Comparison operators | `==` and `!=` correctness |
| 7 | Out-of-bounds exception safety | `++end()`, `--begin()`, `(end)++`, `(begin)--` all throw exceptions |
| 8 | Range query iteration | `range(left, right)` closed interval traversal |
| 9 | Empty list iteration | `begin == end`, zero traversals on empty list |
| 10 | Iterator erase | `erase(it)` returns the next valid iterator |
| 11 | Iterator tags | `bidirectional_iterator_tag` and `iterator_traits` validation |
| 12 | `->` and `*` consistency | `it->data()` and `(*it).data()` return the same object |
| 13 | `View::ref()` | Modify non-key fields by reference and persist |
| 14 | `--end()` reverse traversal | Complete reverse traversal starting from the predecessor of the tail sentinel |
| 15 | Iterator copy | Independent advancement after copy construction/assignment |
| 16 | `front` / `back` | First and last node field access |
| 17 | `prefix_to(key)` | Prefix range query (from first node to specified key) |
| 18 | `suffix_from(key)` | Suffix range query (from specified key to tail node) |
| 18b | Range query boundary conditions | Out-of-range keys return empty/partial ranges instead of crashing |
| 19 | `pop_front` / `pop_back` | First/last node erasure, node recycling, and size update |

```
# 20/20 passed
```

### Performance Test (test_performance.cpp)

> Environment: GCC 13+, `-O3`, 1 million data entries, key range 1~500,000, average of 5 runs.  
> See `test_performance.cpp` for test code; `std::map` uses `find()` for pure lookup and `erase()` for pure deletion, avoiding the insertion side effect of `operator[]`.

| Scenario | Operation | std::map | mSkipList | Multiple (Skip List / Map) |
|----------|-----------|----------|-----------|---------------------------|
| **Random data** | Insert | ~155 ms | ~400 ms | **~2.6×** |
| | Lookup | ~270 ms | ~395 ms | **~1.5×** |
| | Erase | ~55 ms | ~102 ms | **~1.9×** |
| **Sequential data** | Insert | ~183 ms | ~177 ms | **~0.97×** ✅ |
| | Lookup | ~100 ms | ~76 ms | **~0.76×** ✅ |
| | Erase | ~57 ms | ~115 ms | **~2.0×** |

### Micro-Benchmark (test_benchmark.cpp)

Uses rigorous testing methodology with isolated instances + cache warm-up + compiler optimization elimination prevention (test_benchmark.cpp), data scale 50,000, default parameters (gap=3, max_deep=-1):

| Operation | Min | Max | Average | Description |
|-----------|-----|-----|---------|-------------|
| Lookup - Existing key | 80 ns | 3215 ns | **356 ns** | Random key hit, 3000 samples |
| Lookup - Non-existing key | 92 ns | 3472 ns | **116 ns** | Random key miss, longer search path |
| Lookup - First node | 317 ns | - | 317 ns | Minimum key, single test |
| Lookup - Last node | 458 ns | - | 458 ns | Maximum key, single test |
| Erase - Random key | 492 ns | 2497 ns | **826 ns** | Isolated instance method, 60 samples |

Degradation scenario (gap=50000, max_deep=1, forced degradation to singly linked list):

| Operation | Min | Max | Average | Description |
|-----------|-----|-----|---------|-------------|
| Lookup - Degraded list | 146 ns | 114359 ns | **53194 ns** | O(n) linear scan |
| Erase - Degraded list | 423636 ns | 725688 ns | **501384 ns** | O(n) linear deletion |

## Test Verification

| Check Item | Status |
|------------|--------|
| Functional completeness tests | 59/59 passed |
| Iterator-specific tests | 20/20 passed |
| AddressSanitizer (memory leak detection) | Passed |
| Code coverage | Function coverage 95% (114/120) |

## Design Highlights

- **Index construction strategy**: Unlike traditional skip list random promotion, uses a deterministic interval strategy (promote one level every `gap` nodes), ensuring index structure stability
- **Dynamic index maintenance**: Bottom-up bubble-up index construction during insertion, intelligent maintenance of adjacent node connections during deletion
- **Data storage**: Uses `std::tuple` to store multi-field data, with pointer arrays for fast field access by index
- **Index storage optimization (mArray)**: Skip list nodes typically have few levels; uses a fixed-size on-stack array (default 5) for index storage, falling back to heap vector when exceeded, significantly reducing memory footprint and heap allocation overhead for low-level nodes
- **Memory management**: Pre-allocated memory pool reduces system calls, free list recycles deleted nodes for reuse

## License

MIT License  
Copyright (c) 2026 Ximiaw
