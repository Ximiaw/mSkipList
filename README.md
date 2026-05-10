# mSkipList
[English](README.md) | [中文](README_zh.md)

A generic Skip List data structure implemented in C++20, supporting multi-field data storage, any field as the primary key, and custom memory allocation strategies.

## Features

- **Header-only**: Single header file `mSkipList.hpp`, include and use
- **Modern C++20 syntax**: Uses Concepts, Requires constraints, fold expressions, etc.
- **Multi-field data support**: Stores any type and any number of fields via variadic templates
- **Flexible primary key selection**: Any field can be specified as the sorting and lookup key via template parameters
- **Duplicate key update mechanism**: Inserting an existing key automatically overwrites the data instead of creating a new node
- **Custom allocator**: Built-in memory pool + free list to reduce frequent allocation overhead
- **Tunable index parameters**: Supports custom `gap` (index interval) and `max_deep` (maximum level)
- **Sentinel node design**: Simplifies boundary condition handling via head and tail sentinels
- **mArray index storage optimization**: Uses a hybrid structure of stack array + fallback heap vector to reduce heap allocation overhead in small-index scenarios
- **STL-style iterators**: Provides `begin()` / `end()` bidirectional iterators supporting range traversal
- **Range query (Range)**: Supports returning sub-range iterators by primary key range for interval traversal

## Notes
- If using iterators, after deleting the node pointed to by the current iterator via `erase(key)` using the key, dereferencing the iterator is undefined behavior
- Iteration returns an internal `view` variable; if needed, hold it by value copy so that it still points to the old node data after the iterator moves

## Compilation Requirements

- C++20 compatible compiler (only tested with GCC 13+)
- No third-party dependencies

## Quick Start

```cpp
#include "mSkipList.hpp"
#include <iostream>
#include <string>

using namespace msl;

int main() {
    // Use field 0 (int) as the primary key, storing <id, name, score>
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

    // Range traversal (via iterator)
    for (auto it = list.begin(); it != list.end(); ++it) {
        auto data = *it;
        std::cout << std::get<0>(data.data()) << std::endl;
    }

    // Range query: traverse all nodes with primary key in the closed interval [1, 3]
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

## Custom Struct as Primary Key + Range Query

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
| `max_deep` | Maximum skip list level (-1 means unlimited) | -1 |

## Non-First Field as Primary Key

Any field can be specified as the primary key via the `keyIndex` template parameter:

```cpp
// Use field 1 (std::string) as the primary key
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
| `get<Type>(key, field_index)` | Query reference of specified field by primary key |
| `erase(key)` / `erase(it)` | Delete node with specified primary key |
| `contain(key)` | Check if specified primary key exists |
| `begin()` / `end()` | Return head and tail iterators, supporting range traversal |
| `range(left, right)` | Return sub-range iterators by primary key range (closed interval `[left, right]`) |
| `size()` | Return current number of nodes |
| `get_deep()` | Return current skip list level |
| `max_deep()` / `set_max_deep(n)` | Get / set maximum level limit |
| `gap()` / `set_gap(n)` | Get / set index interval |
| `anew_build()` | Rebuild index (call after modifying parameters) |

## Tests

The project includes three sets of tests (O3 optimized):

### Functional Test (test0.cpp)
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

### Iterator Test (test1.cpp)
Covers 11 iterator-specific test cases:

- Forward traversal (`operator*`, `operator++`)
- Backward traversal (`operator--`)
- Range `for` loop (C++11 syntax, value passing)
- Pre-increment `++` and post-increment `++` semantic difference
- Pre-decrement `--` and post-decrement `--` semantic difference
- Iterator comparison operators (`==` and `!=`)
- Out-of-bounds access exception safety (`++end()`, `--begin()`, `(end)++` all throw exceptions)
- Range query iteration (`range(left, right)` closed interval)
- Empty list iterator behavior (`begin == end`)
- Iterator-based deletion (`erase`)
- Iterator tag verification (`bidirectional_iterator_tag`)
```
# 11/11 passed
```

### Performance Test (test2.cpp)

> Environment: GCC 13+, `-O3`, 1 million data entries, key range 1~500,000, averaged over 5 runs.  
> Test code see `test2.cpp`, `std::map` uses `find()` for pure lookup and `erase()` for pure deletion, avoiding the insertion side effect of `operator[]`.

| Scenario | Operation | std::map | mSkipList | Ratio (Skip List / map) |
|----------|-----------|----------|-----------|------------------------|
| **Random Data** | Insert | ~155 ms | ~400 ms | **~2.6x** |
| | Lookup | ~270 ms | ~395 ms | **~1.5x** |
| | Delete | ~55 ms | ~102 ms | **~1.9x** |
| **Sequential Data** | Insert | ~183 ms | ~177 ms | **~0.97x** ✅ |
| | Lookup | ~100 ms | ~76 ms | **~0.76x** ✅ |
| | Delete | ~57 ms | ~115 ms | **~2.0x** |

### Micro Benchmark (test3.cpp)

Using rigorous testing methodology with independent instance method + cache warm-up + anti-compiler-optimization-elimination (test3.cpp), data scale 50000, default parameters (gap=3, max_deep=-1):

| Operation | Min | Max | Avg | Description |
|-----------|-----|-----|-----|-------------|
| Lookup - Existing Key | 80 ns | 3215 ns | **356 ns** | Random key hit, 3000 samples |
| Lookup - Non-existent Key | 92 ns | 3472 ns | **116 ns** | Random key miss, longer search path |
| Lookup - First Node | 317 ns | - | 317 ns | Minimum key, single test |
| Lookup - Last Node | 458 ns | - | 458 ns | Maximum key, single test |
| Delete - Random Key | 492 ns | 2497 ns | **826 ns** | Independent instance method, 60 samples |

Degraded scenario (gap=50000, max_deep=1, forced degradation to singly linked list):

| Operation | Min | Max | Avg | Description |
|-----------|-----|-----|-----|-------------|
| Lookup - Degraded List | 146 ns | 114359 ns | **53194 ns** | O(n) linear scan |
| Delete - Degraded List | 423636 ns | 725688 ns | **501384 ns** | O(n) linear deletion |

## Test Verification

| Check Item | Status |
|------------|--------|
| Functional completeness test | 59/59 passed |
| AddressSanitizer (memory leak detection) | Passed |
| Code coverage | Function coverage 95% (114/120) |

## Design Highlights

- **Index construction strategy**: Unlike traditional skip list random promotion, adopts a deterministic interval strategy (promote one level every `gap` nodes), ensuring index structure stability
- **Dynamic index maintenance**: Bottom-up bubble index construction during insertion, intelligent maintenance of adjacent node connections during deletion
- **Data storage**: Uses `std::tuple` to store multi-field data, with pointer arrays enabling fast access to each field by index
- **Index storage optimization (mArray)**: Skip list node index levels are usually small, using on-stack fixed array (default 5) to store indexes, falling back to heap vector when exceeded, significantly reducing memory footprint and heap allocation overhead for small-level nodes
- **Memory management**: Pre-allocated memory pool reduces system calls, free list recycles deleted nodes for reuse

## License

MIT License  
Copyright (c) 2026 Ximiaw
