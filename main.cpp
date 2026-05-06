#include "mSkipList.hpp"
#include <iostream>
#include <string>
#include <cassert>
#include <stdexcept>
#include <limits>

using namespace msl;

#define TEST(name) std::cout << "[TEST] " << name << std::endl
#define PASS()     std::cout << "  -> PASSED" << std::endl

int main() {
    try {
        // ========== Test 1: 基本插入、查询、更新 ==========
        TEST("Basic insert, get, update");
        {
            auto list = make_mSkipList<0, int, std::string, double>();

            list.insert(1, std::string("one"), 1.1);
            list.insert(2, std::string("two"), 2.2);
            list.insert(3, std::string("three"), 3.3);

            assert(list.contain(1));
            assert(list.contain(2));
            assert(list.contain(3));
            assert(!list.contain(999));

            assert(list.get<std::string>(1, 1) == "one");
            assert(list.get<double>(2, 2) == 2.2);
            assert(list.get<int>(3, 0) == 3);

            // 重复键触发更新
            list.insert(2, std::string("TWO"), 22.22);
            assert(list.get<std::string>(2, 1) == "TWO");
            assert(list.get<double>(2, 2) == 22.22);

            PASS();
        }

        // ========== Test 2: 删除操作 ==========
        TEST("Erase and existence check");
        {
            auto list = make_mSkipList<0, int, std::string>();

            list.insert(10, std::string("ten"));
            list.insert(20, std::string("twenty"));
            list.insert(30, std::string("thirty"));

            assert(list.contain(20));
            list.erase(20);
            assert(!list.contain(20));

            // 删除不存在的键（比所有元素大）应抛异常
            bool thrown = false;
            try {
                list.erase(999);
            } catch (const std::runtime_error&) {
                thrown = true;
            }
            assert(thrown);

            // 删除比所有元素小的键（内部 find 返回 first）应安全返回，不崩溃
            list.erase(5);

            // 删除后重新插入相同键
            list.insert(20, std::string("twenty_new"));
            assert(list.get<std::string>(20, 1) == "twenty_new");

            PASS();
        }

        // ========== Test 3: 大量数据与跳表结构 ==========
        TEST("Bulk insert and skip list structure");
        {
            auto list = make_mSkipList<0, int, long long>();

            const int N = 5000;
            for (int i = 0; i < N; ++i) {
                list.insert(i, static_cast<long long>(i) * 1000LL);
            }

            // 验证所有数据
            for (int i = 0; i < N; ++i) {
                assert(list.get<long long>(i, 1) == static_cast<long long>(i) * 1000LL);
            }

            int depth = list.get_deep();
            std::cout << "  -> Skip list depth after " << N
                      << " inserts: " << depth << std::endl;
            assert(depth > 1);                 // 确保索引层已建立

            list.set_max_deep(5);
            assert(list.max_deep() == 5);

            PASS();
        }

        // ========== Test 4: 字符串主键 ==========
        TEST("String key support");
        {
            auto list = make_mSkipList<0, std::string, int, double>();

            list.insert(std::string("apple"),  1, 1.1);
            list.insert(std::string("banana"), 2, 2.2);
            list.insert(std::string("cherry"), 3, 3.3);

            assert(list.get<int>(std::string("banana"), 1) == 2);
            assert(list.get<double>(std::string("cherry"), 2) == 3.3);

            list.erase(std::string("banana"));
            assert(!list.contain(std::string("banana")));

            PASS();
        }

        // ========== Test 5: 自定义构造参数 ==========
        TEST("Custom allocator / gap / max_deep");
        {
            // allocate_size=1024, gap=2, max_deep=4, 哨兵初始值 0, 0.0
            mSkipList<0, int, double> list(1024, 2, 4, 0, 0.0);

            assert(list.gap() == 2);
            assert(list.max_deep() == 4);

            for (int i = 0; i < 100; ++i) {
                list.insert(i, static_cast<double>(i));
            }

            assert(list.get<double>(50, 1) == 50.0);

            // 修改 gap 并重建索引
            list.set_gap(4);
            list.anew_build();
            assert(list.gap() == 4);

            PASS();
        }

        // ========== Test 6: 异常与边界 ==========
        TEST("Exceptions and boundary conditions");
        {
            auto list = make_mSkipList<0, int, std::string>();

            // 空表查询应抛异常
            bool thrown = false;
            try {
                list.get<std::string>(42, 1);
            } catch (const std::runtime_error&) {
                thrown = true;
            }
            assert(thrown);

            // 极限 int 值
            list.insert(std::numeric_limits<int>::min(), std::string("min"));
            list.insert(std::numeric_limits<int>::max(), std::string("max"));
            assert(list.get<std::string>(std::numeric_limits<int>::min(), 1) == "min");
            assert(list.get<std::string>(std::numeric_limits<int>::max(), 1) == "max");

            PASS();
        }

        // ========== Test 7: 顺序删除与索引重建 ==========
        TEST("Sequential erase and index rebuild");
        {
            auto list = make_mSkipList<0, int, int>();

            for (int i = 0; i < 100; ++i) {
                list.insert(i, i * 2);
            }

            // 删除所有偶数键
            for (int i = 0; i < 100; i += 2) {
                list.erase(i);
            }

            // 验证奇数键仍然可查询
            for (int i = 1; i < 100; i += 2) {
                assert(list.contain(i));
                assert(list.get<int>(i, 1) == i * 2);
            }

            // 手动重建索引后再次验证
            list.anew_build();
            for (int i = 1; i < 100; i += 2) {
                assert(list.get<int>(i, 1) == i * 2);
            }

            PASS();
        }

        std::cout << "\n========================================" << std::endl;
        std::cout << "All tests passed successfully!" << std::endl;
        std::cout << "========================================" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}