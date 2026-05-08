#include "mSkipList.hpp"
#include <iostream>
#include <string>
#include <cassert>
#include <vector>
#include <set>
#include <algorithm>
#include <random>
#include <stdexcept>

using namespace msl;

// 测试统计与输出
int total_tests = 0;
int passed_tests = 0;

void check(bool condition, const std::string& test_name) {
    ++total_tests;
    if (condition) {
        ++passed_tests;
        std::cout << "  [通过] " << test_name << std::endl;
    } else {
        std::cout << "  [失败] " << test_name << std::endl;
    }
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "    mSkipList 功能完整性测试套件" << std::endl;
    std::cout << "========================================" << std::endl;

    // ============================================================
    // 测试组 1：构造与工厂函数
    // ============================================================
    std::cout << "\n【测试组 1】构造与工厂函数" << std::endl;
    {
        // 工厂函数：使用默认初始化构造哨兵节点
        auto list1 = make_mSkipList<0, int, std::string, double>();
        check(list1.size() == 0, "工厂函数构造空表，size 为 0");

        // 显式构造：提供哨兵初始值（0, "", 0.0）
        mSkipList<0, int, std::string, double> list2(0, "", 0.0);
        check(list2.size() == 0, "显式构造空表，size 为 0");

        // 自定义内存池与跳表参数构造
        mSkipList<0, int, std::string, double> list3(1024, 2, 5, 0, "", 0.0);
        check(list3.gap() == 2, "自定义 gap 参数生效");
        check(list3.max_deep() == 5, "自定义 max_deep 参数生效");
    }

    // ============================================================
    // 测试组 2：基础增删改查（CRUD）
    // ============================================================
    std::cout << "\n【测试组 2】基础增删改查" << std::endl;
    {
        auto list = make_mSkipList<0, int, std::string, double>();

        // 插入三条记录：(id, name, score)
        list.insert(1, "Alice", 95.5);
        list.insert(2, "Bob", 87.0);
        list.insert(3, "Charlie", 92.3);

        check(list.size() == 3, "插入 3 条记录后 size 为 3");
        check(list.contain(1), "包含 key=1");
        check(list.contain(2), "包含 key=2");
        check(list.contain(3), "包含 key=3");
        check(!list.contain(999), "不包含未插入的 key=999");

        // 按字段索引查询：0-id, 1-name, 2-score
        const std::string& name1 = list.get<std::string>(1, 1);
        check(name1 == "Alice", "查询 key=1 的 name 字段正确");

        const double& score2 = list.get<double>(2, 2);
        check(score2 == 87.0, "查询 key=2 的 score 字段正确");

        const int& id3 = list.get<int>(3, 0);
        check(id3 == 3, "查询 key=3 的 id 字段正确");

        // 删除中间节点
        list.erase(2);
        check(list.size() == 2, "删除 key=2 后 size 为 2");
        check(!list.contain(2), "删除后不再包含 key=2");
        check(list.contain(1) && list.contain(3), "删除后其余节点仍存在");

        // 删除头节点（逻辑最小 key）
        list.erase(1);
        check(list.size() == 1, "删除头节点后 size 为 1");

        // 删除尾节点（仅剩节点）
        list.erase(3);
        check(list.size() == 0, "删除尾节点后 size 为 0");
        check(!list.contain(3), "空表不再包含任何 key");
    }

    // ============================================================
    // 测试组 3：重复键更新机制
    // ============================================================
    std::cout << "\n【测试组 3】重复键更新机制" << std::endl;
    {
        auto list = make_mSkipList<0, int, std::string, double>();

        list.insert(1, "Alice", 95.5);
        list.insert(1, "AliceUpdated", 98.0);  // 同一 key，应覆盖而非新增

        check(list.size() == 1, "重复 key 插入后 size 仍为 1");

        const std::string& name = list.get<std::string>(1, 1);
        check(name == "AliceUpdated", "name 字段已更新");

        const double& score = list.get<double>(1, 2);
        check(score == 98.0, "score 字段已更新");

        // 再次更新验证稳定性
        list.insert(1, "AliceV3", 100.0);
        check(list.get<std::string>(1, 1) == "AliceV3", "多次更新后数据正确");
        check(list.get<double>(1, 2) == 100.0, "多次更新后 score 正确");
    }

    // ============================================================
    // 测试组 4：边界条件与异常行为
    // ============================================================
    std::cout << "\n【测试组 4】边界条件与异常行为" << std::endl;
    {
        auto list = make_mSkipList<0, int, std::string, double>();

        // 空表查询不存在的 key，应抛出 runtime_error
        bool thrown = false;
        try {
            list.get<std::string>(42, 1);
        } catch (const std::runtime_error&) {
            thrown = true;
        }
        check(thrown, "空表查询不存在 key 抛出 runtime_error");

        // 删除不存在的 key，应静默处理不崩溃
        list.erase(42);
        list.erase(0);
        check(list.size() == 0, "删除不存在 key 后 size 保持 0");

        // 单节点表：插入后立即删除
        list.insert(100, "Only", 60.0);
        check(list.size() == 1, "单节点表 size=1");
        list.erase(100);
        check(list.size() == 0, "删除唯一节点后 size=0");

        // 连续删除已删除的 key
        list.erase(100);
        check(list.size() == 0, "重复删除已删节点无异常");
    }

    // ============================================================
    // 测试组 5：跳表索引结构参数验证
    // ============================================================
    std::cout << "\n【测试组 5】跳表索引结构参数验证" << std::endl;
    {
        // 使用受限层数与较大 gap，便于观察索引行为
        mSkipList<0, int, std::string, double> list(1024, 4, 4, 0, "", 0.0);

        // 空表时仅有第 0 层（first <-> last）
        check(list.get_deep() == 1, "空表初始层数为 1");

        // 插入少量数据，索引应至少保持第 0 层
        for (int i = 1; i <= 10; ++i) {
            list.insert(i, "User", static_cast<double>(i));
        }
        check(list.get_deep() >= 1, "插入数据后层数 >= 1");
        check(list.size() == 10, "插入 10 条数据成功");

        // 验证层数上限约束
        list.set_max_deep(2);
        check(list.max_deep() == 2, "set_max_deep(2) 生效");

        list.set_gap(6);
        check(list.gap() == 6, "set_gap(6) 生效");

        // 重建索引后数据应完整保留
        list.anew_build();
        check(list.size() == 10, "anew_build 后数据未丢失");
        check(list.contain(5), "anew_build 后仍可查询中间节点");
        check(list.get<std::string>(5, 1) == "User", "anew_build 后数据值正确");

        // 大量数据 + 严格层数上限
        mSkipList<0, int, double> big_list(4096, 2, 3, 0, 0.0);
        for (int i = 0; i < 2000; ++i) {
            big_list.insert(i, static_cast<double>(i));
        }
        check(big_list.size() == 2000, "批量插入 2000 条成功");
        check(big_list.get_deep() <= 3, "层数受 max_deep=3 限制");
    }

    // ============================================================
    // 测试组 6：大量数据随机操作与正确性
    // ============================================================
    std::cout << "\n【测试组 6】大量数据随机操作与正确性" << std::endl;
    {
        mSkipList<0, int, double> list(0, 0.0);

        const int N = 3000;
        std::set<int> key_set;
        std::mt19937 rng(12345);
        std::uniform_int_distribution<int> dist(1, 50000);

        // 生成 N 个不重复的随机 key
        while (key_set.size() < static_cast<size_t>(N)) {
            key_set.insert(dist(rng));
        }
        std::vector<int> keys(key_set.begin(), key_set.end());
        std::shuffle(keys.begin(), keys.end(), rng);

        // 插入阶段
        for (int k : keys) {
            list.insert(k, static_cast<double>(k) * 1.5);
        }

        check(list.size() == N, "插入 N 个唯一 key 后 size 为 N");

        // 验证所有 key 存在且数据正确
        bool all_found = true;
        bool all_correct = true;
        for (int k : keys) {
            if (!list.contain(k)) {
                all_found = false;
                break;
            }
            if (list.get<double>(k, 1) != static_cast<double>(k) * 1.5) {
                all_correct = false;
                break;
            }
        }
        check(all_found, "所有插入的 key 均可查询");
        check(all_correct, "所有 key 对应的数据值正确");

        // 随机删除前半部分（每个 key 只删一次，绝无重复）
        std::shuffle(keys.begin(), keys.end(), rng);  // 再次打乱，模拟随机删除
        for (int i = 0; i < N / 2; ++i) {
            list.erase(keys[i]);
        }

        // 验证后半部分 key 仍然全部存在
        bool remaining_ok = true;
        for (int i = N / 2; i < N; ++i) {
            if (!list.contain(keys[i])) {
                remaining_ok = false;
                break;
            }
        }
        check(remaining_ok, "批量删除后剩余节点查询正常");
        check(list.size() == N / 2, "删除一半后 size 为 N/2");
    }

    // ============================================================
    // 测试组 7：非首字段作为主键
    // ============================================================
    std::cout << "\n【测试组 7】非首字段作为主键" << std::endl;
    {
        // 以第 1 个字段（std::string）作为主键
        mSkipList<1, int, std::string, double> list(0, "", 0.0);

        list.insert(101, "Alice", 95.0);
        list.insert(102, "Bob", 88.5);
        list.insert(103, "Charlie", 92.0);

        check(list.contain(std::string("Alice")), "字符串主键查询 Alice");
        check(list.contain(std::string("Bob")), "字符串主键查询 Bob");
        check(!list.contain(std::string("David")), "不包含 David");

        const int& id_alice = list.get<int>(std::string("Alice"), 0);
        check(id_alice == 101, "通过字符串主键获取 int 字段");

        // 覆盖相同字符串 key
        list.insert(201, "Alice", 96.5);
        check(list.size() == 3, "覆盖字符串 key 后 size 不变");
        check(list.get<int>(std::string("Alice"), 0) == 201, "字符串 key 对应数据已更新");
        check(list.get<double>(std::string("Alice"), 2) == 96.5, "字符串 key 对应 score 已更新");
    }

    // ============================================================
    // 测试组 8：高强度交替操作稳定性
    // ============================================================
    std::cout << "\n【测试组 8】高强度交替操作稳定性" << std::endl;
    {
        mSkipList<0, int, std::string> list(0, "");

        // 多轮次全量插入后全量删除
        for (int round = 0; round < 50; ++round) {
            for (int i = 0; i < 100; ++i) {
                list.insert(i, "R" + std::to_string(round) + "_U" + std::to_string(i));
            }
            for (int i = 0; i < 100; ++i) {
                list.erase(i);
            }
        }
        check(list.size() == 0, "50 轮全量插入删除后为空");

        // 验证表结构未损坏，可正常重新使用
        list.insert(42, "Reuse");
        check(list.contain(42), "高强度操作后表结构可复用");
        check(list.get<std::string>(42, 1) == "Reuse", "复用后数据正确");

        list.erase(42);
        check(list.size() == 0, "清理后恢复空表");
    }

    // ============================================================
    // 测试组 9：跨类型字段组合
    // ============================================================
    std::cout << "\n【测试组 9】跨类型字段组合" << std::endl;
    {
        // 使用 int, float, std::string, long long 四字段，以第 2 个字段（std::string）为主键
        mSkipList<2, int, float, std::string, long long> list(0, 0.0f, "", 0LL);

        list.insert(10, 3.14f, "Pi", 31415926LL);
        list.insert(20, 2.71f, "E", 27182818LL);

        check(list.contain(std::string("Pi")), "四字段表包含 Pi");
        check(list.contain(std::string("E")), "四字段表包含 E");

        const int& i_val = list.get<int>(std::string("Pi"), 0);
        const float& f_val = list.get<float>(std::string("Pi"), 1);
        const long long& ll_val = list.get<long long>(std::string("Pi"), 3);

        check(i_val == 10, "跨类型字段 int 正确");
        check(f_val == 3.14f, "跨类型字段 float 正确");
        check(ll_val == 31415926LL, "跨类型字段 long long 正确");
    }

    // ============================================================
    // 测试总结
    // ============================================================
    std::cout << "\n========================================" << std::endl;
    std::cout << "  测试完成: " << passed_tests << " / " << total_tests << " 通过" << std::endl;
    if (passed_tests == total_tests) {
        std::cout << "  全部通过，功能完整性良好！" << std::endl;
    } else {
        std::cout <<  "  存在失败用例，请检查实现。" << std::endl;
    }
    std::cout << "========================================" << std::endl;

    return (passed_tests == total_tests) ? 0 : 1;
}