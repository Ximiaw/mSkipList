# mSkipList

一个基于 C++20 实现的通用跳表（Skip List）数据结构，支持多字段数据存储、任意字段作为主键、以及自定义内存分配策略。

## 特性

- **Header-only**：单头文件 `mSkipList.hpp`，包含即可用
- **C++20 现代语法**：使用 Concepts、Requires 约束、折叠表达式等
- **多字段数据支持**：通过可变参数模板存储任意类型、任意数量的字段
- **灵活的主键选择**：可通过模板参数指定任意字段作为排序和查找主键
- **重复键更新机制**：插入已存在的 key 时自动覆盖数据，而非新增节点
- **自定义内存分配器**：内置内存池 + 自由列表，减少频繁分配开销
- **可调的索引参数**：支持自定义 gap（索引间隔）和 max_deep（最大层数）
- **哨兵节点设计**：通过首尾哨兵简化边界条件处理
- **mArray 索引存储优化**：采用栈数组 + 后备堆 vector 的混合结构，减少小索引场景下的堆分配开销（arrSize可调优）

## 编译要求

- 支持 C++20 的编译器（仅测试了GCC 13+）
- 无第三方依赖

## 快速开始

```cpp
#include "mSkipList.hpp"
#include <iostream>
#include <string>

using namespace msl;

int main() {
    // 以第 0 个字段（int）作为主键，存储 <id, name, score>
    auto list = make_mSkipList<0, int, std::string, double>();

    // 插入数据
    list.insert(1, "Alice", 95.5);
    list.insert(2, "Bob", 87.0);
    list.insert(3, "Charlie", 92.3);

    // 查询：get<字段类型>(key, 字段索引)
    const std::string& name = list.get<std::string>(1, 1);
    std::cout << "ID=1 的名字: " << name << std::endl;  // Alice

    // 重复 key 会覆盖
    list.insert(1, "AliceUpdated", 98.0);

    // 检查存在性
    if (list.contain(2)) {
        std::cout << "包含 ID=2" << std::endl;
    }

    // 删除
    list.erase(2);
    std::cout << "当前大小: " << list.size() << std::endl;  // 2

    return 0;
}
```

编译：
```bash
g++ -std=c++20 main.cpp -O3 -o main
```

## 构造方式

### 工厂函数（推荐）
```cpp
auto list = make_mSkipList<0, int, std::string, double>();
```

### 显式构造
```cpp
// 使用默认值构造哨兵节点
mSkipList<0, int, std::string, double> list(0, "", 0.0);
```

### 自定义参数
```cpp
// 参数：内存池大小, gap, max_deep, 哨兵初始值...
mSkipList<0, int, std::string> list(1024, 2, 5, 0, "");
```

| 参数 | 说明 | 默认值 |
|------|------|--------|
| `allocate_size` | 内存池块大小 | 4096 |
| `gap` | 相邻索引节点之间间隔的数据节点数 | 3 |
| `max_deep` | 跳表最大层数（-1 表示不限制） | -1 |

## 非首字段主键

可以通过模板参数 `keyIndex` 指定任意字段作为主键：

```cpp
// 以第 1 个字段（std::string）作为主键
mSkipList<1, int, std::string, double> list(0, "", 0.0);

list.insert(101, "Alice", 95.0);
list.insert(102, "Bob", 88.5);

// 通过字符串主键查询
const int& id = list.get<int>(std::string("Alice"), 0);
```

## API 参考

| 方法 | 说明 |
|------|------|
| `insert(T_D... args)` | 插入数据；若主键已存在则覆盖 |
| `get<Type>(key, field_index)` | 按主键查询指定字段的引用 |
| `erase(key)` | 删除指定主键的节点 |
| `contain(key)` | 判断是否包含指定主键 |
| `size()` | 返回当前节点数 |
| `get_deep()` | 返回当前跳表层数 |
| `max_deep()` / `set_max_deep(n)` | 获取/设置最大层数限制 |
| `gap()` / `set_gap(n)` | 获取/设置索引间隔 |
| `anew_build()` | 重建索引（修改参数后调用） |

## 测试

项目包含两组测试：

### 功能测试（test1.cpp）
覆盖 9 大测试组、59 个测试用例：

- 构造与工厂函数
- 基础增删改查（CRUD）
- 重复键更新机制
- 边界条件与异常行为
- 跳表索引结构参数验证
- 大量数据随机操作与正确性（3000 节点）
- 非首字段作为主键
- 高强度交替操作稳定性（50 轮 x 100 节点）
- 跨类型字段组合

```bash
g++ -std=c++20 mSkipList.hpp test1.cpp -O3 -o test && ./test
# 59/59 通过
```

### 性能测试（test.cpp）

> 环境：GCC 13+，`-O3`，100 万条数据，键值范围 1~50 万，取 5 次运行平均值。  
> 测试代码见 `test.cpp`，`std::map` 使用 `find()` 纯查找、`erase()` 纯删除，避免 `operator[]` 的插入副作用。

| 场景 | 操作 | std::map | mSkipList | 倍数 (跳表/map) |
|------|------|----------|-----------|-----------------|
| **随机数据** | 插入 | ~155 ms | ~400 ms | **~2.6×** |
| | 查找 | ~270 ms | ~395 ms | **~1.5×** |
| | 删除 | ~55 ms | ~102 ms | **~1.9×** |
| **顺序数据** | 插入 | ~183 ms | ~177 ms | **~0.97×** ✅ |
| | 查找 | ~100 ms | ~76 ms | **~0.76×** ✅ |
| | 删除 | ~57 ms | ~115 ms | **~2.0×** |

## 测试验证

| 检查项 | 状态 |
|--------|------|
| 功能完整性测试 | 59/59 通过 |
| AddressSanitizer（内存泄漏检测） | 通过 |
| 代码覆盖率 | 函数覆盖 95%（114/120）|

## 设计要点

- **索引构建策略**：不同于传统跳表的随机提升，采用确定性间隔策略（每 `gap` 个节点提升一层），保证索引结构的稳定性
- **动态索引维护**：插入时自底向上冒泡构建索引，删除时智能维护相邻节点连接关系
- **数据存储**：使用 `std::tuple` 存储多字段数据，通过指针数组实现按索引快速访问各字段
- **内存管理**：预分配内存池减少系统调用，自由列表回收已删除节点实现复用

## 许可证

MIT License  
Copyright (c) 2026 Ximiaw