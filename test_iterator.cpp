#include "mSkipList.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <iterator>
#include <type_traits>

using namespace msl;

// ============================================================
// 测试辅助工具
// ============================================================
static int g_passed = 0;
static int g_failed = 0;

void print_title(const std::string& s) {
    std::cout << "\n========== " << s << " ==========\n";
}

void check(const std::string& desc, bool condition) {
    std::cout << "  [" << (condition ? "PASS" : "FAIL") << "] " << desc << "\n";
    if (condition) ++g_passed; else ++g_failed;
}

void check_eq(const std::string& desc, int actual, int expected) {
    bool ok = (actual == expected);
    std::cout << "  [" << (ok ? "PASS" : "FAIL") << "] " << desc
              << " => " << actual << (ok ? "" : " (预期: " + std::to_string(expected) + ")") << "\n";
    if (ok) ++g_passed; else ++g_failed;
}

void check_str_eq(const std::string& desc, const std::string& actual, const std::string& expected) {
    bool ok = (actual == expected);
    std::cout << "  [" << (ok ? "PASS" : "FAIL") << "] " << desc
              << " => \"" << actual << "\"" << (ok ? "" : " (预期: \"" + expected + "\")") << "\n";
    if (ok) ++g_passed; else ++g_failed;
}

// 从 View 中提取主键（keyIndex = 0）
// 注意：View::data() 非 const，故此处使用非 const 引用
template<typename View>
int get_key(View& view) {
    return std::get<0>(view.data());
}

// 从 View 中提取 value 字段（field_index = 1）
template<typename View>
std::string& get_value(View& view) {
    return std::get<1>(view.data());
}

// 检查抛出异常的工具宏
#define CHECK_THROWS(desc, code, expected_msg_substr)                       \
    do {                                                                     \
        bool caught = false;                                                 \
        try {                                                                \
            code;                                                            \
            std::cout << "  [FAIL] " desc " => 未抛出异常\n";               \
            ++g_failed;                                                      \
        } catch (const std::runtime_error& e) {                              \
            std::string what = e.what();                                     \
            if (what.find(expected_msg_substr) != std::string::npos) {       \
                std::cout << "  [PASS] " desc " => 捕获预期异常\n";         \
                ++g_passed;                                                  \
            } else {                                                         \
                std::cout << "  [FAIL] " desc " => 异常信息不匹配: "        \
                          << e.what() << "\n";                             \
                ++g_failed;                                                  \
            }                                                                \
        }                                                                    \
    } while (0)


// ============================================================
// main
// ============================================================
int main() {
    // ============================================================
    print_title("构造跳表并插入测试数据");
    // ============================================================
    mSkipList<0, int, std::string> list(1024, 3, -1, 0, "");

    std::vector<std::pair<int, std::string>> raw_data = {
        {10, "十"}, {20, "二十"}, {5, "五"},
        {30, "三十"}, {15, "十五"}, {25, "二十五"}
    };
    for (auto& p : raw_data) {
        list.insert(p.first, p.second);
        std::cout << "  插入: key=" << p.first << ", value=\"" << p.second << "\"\n";
    }
    check_eq("跳表大小应为 6", (int)list.size(), 6);

    // 排序后的期望主键序列
    const std::vector<int> E = {5, 10, 15, 20, 25, 30};


    // ============================================================
    print_title("测试 1: 正向遍历（operator* 与前置 operator++）");
    // ============================================================
    {
        int idx = 0;
        for (auto it = list.begin(); it != list.end(); ++it) {
            int k = get_key(*it);
            check("第 " + std::to_string(idx) + " 个节点 key=" + std::to_string(E[idx]),
                  k == E[idx]);
            ++idx;
        }
        check_eq("正向遍历节点总数", idx, 6);
    }


    // ============================================================
    print_title("测试 2: 反向遍历（operator-- 与 operator->）");
    // ============================================================
    {
        auto it = list.end();
        int rev_idx = (int)E.size() - 1;
        bool order_ok = true;
        int count = 0;
        while (true) {
            --it;
            int k = std::get<0>(it->data());   // 使用 operator->
            if (k != E[rev_idx]) order_ok = false;
            --rev_idx;
            ++count;
            if (it == list.begin()) break;
        }
        check("反向遍历顺序正确", order_ok);
        check_eq("反向遍历节点总数", count, 6);
    }


    // ============================================================
    print_title("测试 3: 范围 for 循环（C++11 语法，值传递）");
    // ============================================================
    {
        int idx = 0;
        for (auto view : list) {
            int k = get_key(view);
            check("第 " + std::to_string(idx) + " 个元素 key=" + std::to_string(E[idx]),
                  k == E[idx]);
            ++idx;
        }
        check_eq("范围 for 遍历总数", idx, 6);
    }


    // ============================================================
    print_title("测试 4: 前置++ 与 后置++ 语义区别");
    // ============================================================
    {
        auto it_post = list.begin();   // 指向 key=5
        auto it_pre  = list.begin();   // 指向 key=5

        auto val_post = (*(it_post++)).data();   // 后置：先返回旧值，再自增
        auto val_pre  = (*++it_pre).data();       // 前置：先自增，再返回新值

        check_eq("后置++ 返回值应为 5 (key=5)", std::get<0>(val_post), 5);
        check_eq("前置++ 返回值应为 10 (key=10)", std::get<0>(val_pre), 10);
        check_eq("后置++ 后迭代器应指向 key=10", get_key(*it_post), 10);
        check_eq("前置++ 后迭代器应指向 key=10", get_key(*it_pre), 10);
        check("两迭代器现在应相等", it_post == it_pre);
    }


    // ============================================================
    print_title("测试 5: 前置-- 与 后置-- 语义区别");
    // ============================================================
    {
        auto it_rpost = list.end();   // 指向尾哨兵
        auto it_rpre  = list.end();
        --it_rpost;                    // 移到最后一个有效节点 (key=30)
        --it_rpre;

        auto val_rpost = (*(it_rpost--)).data();  // 后置：先返回 key=30，再自减
        auto val_rpre  = (*--it_rpre).data();      // 前置：先自减，再返回 key=25

        check_eq("后置-- 返回值应为 30 (key=30)", std::get<0>(val_rpost), 30);
        check_eq("前置-- 返回值应为 25 (key=25)", std::get<0>(val_rpre), 25);
        check_eq("后置-- 后迭代器应指向 key=25", get_key(*it_rpost), 25);
        check_eq("前置-- 后迭代器应指向 key=25", get_key(*it_rpre), 25);
        check("两迭代器现在应相等", it_rpost == it_rpre);
    }


    // ============================================================
    print_title("测试 6: 迭代器比较运算符（== 与 !=）");
    // ============================================================
    {
        auto a = list.begin();
        auto b = list.begin();
        auto c = list.end();
        check("begin == begin", a == b);
        check("begin != end",   a != c);
        check("!(begin == end)", !(a == c));
    }


    // ============================================================
    print_title("测试 7: 越界访问异常安全性");
    // ============================================================
    {
        // 前置++ 越过 end()
        CHECK_THROWS("++end() 应抛异常",
                     { auto it = list.end(); ++it; },
                     "overstep");

        // 前置-- 越过 begin()
        CHECK_THROWS("--begin() 应抛异常",
                     { auto it = list.begin(); --it; },
                     "overstep");

        // 后置++(end) 越过 end()
        CHECK_THROWS("(end)++ 应抛异常",
                     { auto it = list.end(); it++; },
                     "overstep");

        // 后置--(begin) 越过 begin()  【新增测试】
        CHECK_THROWS("(begin)-- 应抛异常",
                     { auto it = list.begin(); it--; },
                     "overstep");
    }


    // ============================================================
    print_title("测试 8: 范围查询迭代（range 闭区间）");
    // ============================================================
    {
        auto rng = list.range(10, 25);
        std::vector<int> range_keys;
        for (auto it = rng.begin(); it != rng.end(); ++it) {
            range_keys.push_back(get_key(*it));
        }
        check_eq("range[10,25] 元素个数应为 4", (int)range_keys.size(), 4);
        bool keys_ok = (range_keys.size() == 4 &&
                        range_keys[0] == 10 && range_keys[1] == 15 &&
                        range_keys[2] == 20 && range_keys[3] == 25);
        check("range[10,25] 元素为 10,15,20,25", keys_ok);
    }


    // ============================================================
    print_title("测试 9: 空表迭代行为");
    // ============================================================
    {
        mSkipList<0, int, std::string> empty_list(1024, 3, -1, 0, "");
        check("空表 begin == end", empty_list.begin() == empty_list.end());

        int empty_cnt = 0;
        for (auto it = empty_list.begin(); it != empty_list.end(); ++it)
            ++empty_cnt;
        check_eq("空表遍历节点数应为 0", empty_cnt, 0);

        // 空表越界也应安全
        CHECK_THROWS("空表 ++begin 应抛异常",
                     { auto it = empty_list.begin(); ++it; },
                     "overstep");
    }


    // ============================================================
    print_title("测试 10: 基于迭代器的删除（erase）");
    // ============================================================
    {
        check_eq("删除前大小应为 6", (int)list.size(), 6);

        auto it = list.begin();
        ++it;   // 现在指向 key=10
        check_eq("待删除节点 key 应为 10", get_key(*it), 10);

        auto nxt = list.erase(it);
        check_eq("删除后返回迭代器应指向 key=15", get_key(*nxt), 15);
        check_eq("删除后大小应为 5", (int)list.size(), 5);

        // 验证删除后剩余序列
        std::vector<int> remaining;
        for (auto i = list.begin(); i != list.end(); ++i)
            remaining.push_back(get_key(*i));
        bool seq_ok = (remaining.size() == 5 &&
                       remaining[0] == 5 && remaining[1] == 15 &&
                       remaining[2] == 20 && remaining[3] == 25 && remaining[4] == 30);
        check("删除后序列为 5,15,20,25,30", seq_ok);
    }


    // ============================================================
    print_title("测试 11: 迭代器标签验证（bidirectional_iterator_tag）");
    // ============================================================
    {
        using Iter = decltype(list.begin());
        static_assert(std::is_same_v<typename std::iterator_traits<Iter>::iterator_category,
                                     std::bidirectional_iterator_tag>,
                      "迭代器类别应为 bidirectional_iterator_tag");
        check("iterator_category 为 bidirectional_iterator_tag", true);

        // 同时验证与 std::iterator_traits 的其他类型定义
        static_assert(std::is_same_v<typename std::iterator_traits<Iter>::value_type,
                                     typename Iter::value_type>,
                      "iterator_traits value_type 一致");
        static_assert(std::is_same_v<typename std::iterator_traits<Iter>::difference_type,
                                     std::ptrdiff_t>,
                      "iterator_traits difference_type 为 ptrdiff_t");
        check("iterator_traits 类型定义正确", true);
    }


    // ============================================================
    print_title("测试 12: operator-> 与 operator* 一致性");
    // ============================================================
    {
        auto it = list.begin();   // 指向 key=5

        // operator-> 应返回 View*，解引用后与 operator* 等价
        const auto& tup_arrow = it->data();
        const auto& tup_star  = (*it).data();
        check("it->data() 与 (*it).data() 地址相同", &tup_arrow == &tup_star);
        check_eq("it-> 读取 key=5", std::get<0>(tup_arrow), 5);
        check_eq("*it  读取 key=5", std::get<0>(tup_star), 5);

        // operator-> 可用于连续访问（虽然 View 没有深层成员）
        check_str_eq("it->data() 读取 value", std::get<1>(it->data()), "五");
    }


    // ============================================================
    print_title("测试 13: View::ref() 访问非主键字段");
    // ============================================================
    {
        auto it = list.begin();   // key=5, value="五"

        // ref<string>(1) 应返回 value 字段的可变引用
        std::string& val_ref = it->ref<std::string>(1);
        check_str_eq("ref(1) 读取 value", val_ref, "五");

        // 通过引用修改
        val_ref = "伍";
        check_str_eq("通过 ref 修改后 get 验证", list.get<std::string>(5, 1), "伍");

        // 恢复原始值
        val_ref = "五";
        check_str_eq("恢复后 value 应为 五", list.get<std::string>(5, 1), "五");
    }


    // 经过测试 10 的删除操作后，当前跳表节点为：5, 15, 20, 25, 30
    const std::vector<int> E2 = {5, 15, 20, 25, 30};

    // ============================================================
    print_title("测试 14: --end() 反向遍历模式");
    // ============================================================
    {
        // 从 end() 开始，通过 --it 反向遍历全部节点
        int rev_idx = (int)E2.size() - 1;
        bool order_ok = true;
        int count = 0;

        // 注意：end() 指向尾哨兵，需先 -- 才能访问最后一个有效节点
        for (auto it = list.end(); it != list.begin(); ) {
            --it;
            int k = get_key(*it);
            if (k != E2[rev_idx]) order_ok = false;
            --rev_idx;
            ++count;
        }
        check("--end() 反向遍历顺序正确", order_ok);
        check_eq("--end() 遍历节点总数", count, 5);
    }


    // ============================================================
    print_title("测试 15: 迭代器拷贝构造与拷贝赋值");
    // ============================================================
    {
        auto it1 = list.begin();   // 指向 key=5
        auto it2 = it1;            // 拷贝构造
        auto it3 = list.end();
        it3 = it1;                 // 拷贝赋值

        check("拷贝构造后 it1 == it2", it1 == it2);
        check("拷贝赋值后 it1 == it3", it1 == it3);
        check_eq("it1 指向 key=5", get_key(*it1), 5);
        check_eq("it2 指向 key=5", get_key(*it2), 5);
        check_eq("it3 指向 key=5", get_key(*it3), 5);

        // 验证拷贝后的迭代器独立前进（当前第二个节点是 key=15，因为 10 已被删除）
        ++it2;
        check("++it2 后 it2 != it1", it2 != it1);
        check_eq("it2 现在指向 key=15", get_key(*it2), 15);
        check_eq("it1 仍指向 key=5",    get_key(*it1), 5);
    }


    // ============================================================
    print_title("测试 16: front<Type>() / back<Type>() 首尾访问");
    // ============================================================
    {
        // 当前节点: 5, 15, 20, 25, 30
        check_str_eq("front(1) 获取首节点 value", list.front<std::string>(1), "五");
        check_str_eq("back(1) 获取尾节点 value", list.back<std::string>(1), "三十");
        check_eq("front(0) 获取首节点 key", list.front<int>(0), 5);
        check_eq("back(0) 获取尾节点 key", list.back<int>(0), 30);
    }


    // ============================================================
    print_title("测试 17: prefix_to(key) 前缀范围查询");
    // ============================================================
    {
        // 当前节点: 5, 15, 20, 25, 30
        {
            // prefix_to(20) → [5, 15, 20]
            auto rng = list.prefix_to(20);
            std::vector<int> keys;
            for (auto it = rng.begin(); it != rng.end(); ++it)
                keys.push_back(get_key(*it));
            bool ok = (keys.size() == 3 && keys[0] == 5 && keys[1] == 15 && keys[2] == 20);
            check("prefix_to(20) 包含 5,15,20", ok);
        }
        {
            // prefix_to(100) → [5, 15, 20, 25, 30]（key 不存在，所有节点都小于 100）
            auto rng = list.prefix_to(100);
            std::vector<int> keys;
            for (auto it = rng.begin(); it != rng.end(); ++it)
                keys.push_back(get_key(*it));
            bool ok = (keys.size() == 5 && keys[0] == 5 && keys[4] == 30);
            check("prefix_to(100) 包含全部 5 个节点", ok);
        }
        {
            // prefix_to(5) → [5]（只包含首节点）
            auto rng = list.prefix_to(5);
            std::vector<int> keys;
            for (auto it = rng.begin(); it != rng.end(); ++it)
                keys.push_back(get_key(*it));
            bool ok = (keys.size() == 1 && keys[0] == 5);
            check("prefix_to(5) 只包含首节点", ok);
        }
    }


    // ============================================================
    print_title("测试 18: suffix_from(key) 后缀范围查询");
    // ============================================================
    {
        // 当前节点: 5, 15, 20, 25, 30
        {
            // suffix_from(15) → [15, 20, 25, 30]
            auto rng = list.suffix_from(15);
            std::vector<int> keys;
            for (auto it = rng.begin(); it != rng.end(); ++it)
                keys.push_back(get_key(*it));
            bool ok = (keys.size() == 4 && keys[0] == 15 && keys[3] == 30);
            check("suffix_from(15) 包含 15,20,25,30", ok);
        }
        {
            // suffix_from(25) → [25, 30]
            auto rng = list.suffix_from(25);
            std::vector<int> keys;
            for (auto it = rng.begin(); it != rng.end(); ++it)
                keys.push_back(get_key(*it));
            bool ok = (keys.size() == 2 && keys[0] == 25 && keys[1] == 30);
            check("suffix_from(25) 包含 25,30", ok);
        }
        {
            // suffix_from(30) → [30]（只包含尾节点）
            auto rng = list.suffix_from(30);
            std::vector<int> keys;
            for (auto it = rng.begin(); it != rng.end(); ++it)
                keys.push_back(get_key(*it));
            bool ok = (keys.size() == 1 && keys[0] == 30);
            check("suffix_from(30) 只包含尾节点", ok);
        }
    }


    // ============================================================
    print_title("测试 18b: 范围查询边界条件");
    // ============================================================
    {
        // 当前节点: 5, 15, 20, 25, 30
        // range 左边界小于最小 key → [min, right]
        {
            auto rng = list.range(0, 25);
            std::vector<int> ks;
            for (auto it = rng.begin(); it != rng.end(); ++it)
                ks.push_back(get_key(*it));
            bool ok = (ks.size() == 4 && ks[0] == 5 && ks[3] == 25);
            check("range(0,25) 返回 5,15,20,25", ok);
        }
        // range 左右边界都小于最小 key → 空
        {
            auto rng = list.range(1, 4);
            std::vector<int> ks;
            for (auto it = rng.begin(); it != rng.end(); ++it)
                ks.push_back(get_key(*it));
            check("range(1,4) 为空", ks.empty());
        }
        // range 左边界大于最大 key → 空
        {
            auto rng = list.range(100, 200);
            std::vector<int> ks;
            for (auto it = rng.begin(); it != rng.end(); ++it)
                ks.push_back(get_key(*it));
            check("range(100,200) 为空", ks.empty());
        }
        // suffix_from 最小 key → 整个表
        {
            auto rng = list.suffix_from(5);
            std::vector<int> ks;
            for (auto it = rng.begin(); it != rng.end(); ++it)
                ks.push_back(get_key(*it));
            bool ok = (ks.size() == 5 && ks[0] == 5 && ks[4] == 30);
            check("suffix_from(5) 返回全部节点", ok);
        }
        // suffix_from key 小于最小 key → 整个表
        {
            auto rng = list.suffix_from(1);
            std::vector<int> ks;
            for (auto it = rng.begin(); it != rng.end(); ++it)
                ks.push_back(get_key(*it));
            bool ok = (ks.size() == 5 && ks[0] == 5 && ks[4] == 30);
            check("suffix_from(1) 返回全部节点", ok);
        }
        // suffix_from key 大于最大 key → 空
        {
            auto rng = list.suffix_from(100);
            std::vector<int> ks;
            for (auto it = rng.begin(); it != rng.end(); ++it)
                ks.push_back(get_key(*it));
            check("suffix_from(100) 为空", ks.empty());
        }
        // suffix_from key 不存在且位于中间 → 从大于 key 的最小节点开始
        {
            auto rng = list.suffix_from(18);
            std::vector<int> ks;
            for (auto it = rng.begin(); it != rng.end(); ++it)
                ks.push_back(get_key(*it));
            bool ok = (ks.size() == 3 && ks[0] == 20 && ks[2] == 30);
            check("suffix_from(18) 返回 20,25,30", ok);
        }
        // prefix_to key 小于最小 key → 空
        {
            auto rng = list.prefix_to(3);
            std::vector<int> ks;
            for (auto it = rng.begin(); it != rng.end(); ++it)
                ks.push_back(get_key(*it));
            check("prefix_to(3) 为空", ks.empty());
        }
        // prefix_to key 大于最大 key → 整个表
        {
            auto rng = list.prefix_to(100);
            std::vector<int> ks;
            for (auto it = rng.begin(); it != rng.end(); ++it)
                ks.push_back(get_key(*it));
            bool ok = (ks.size() == 5 && ks[0] == 5 && ks[4] == 30);
            check("prefix_to(100) 返回全部节点", ok);
        }
    }


    // ============================================================
    print_title("测试 19: pop_front() / pop_back() 首尾删除");
    // ============================================================
    {
        // 当前节点: 5, 15, 20, 25, 30
        check_eq("删除前大小", (int)list.size(), 5);

        list.pop_front();   // 删除 5
        check_eq("pop_front 后大小", (int)list.size(), 4);
        check_eq("pop_front 后首节点 key", list.front<int>(0), 15);
        check_str_eq("pop_front 后首节点 value", list.front<std::string>(1), "十五");

        list.pop_back();    // 删除 30
        check_eq("pop_back 后大小", (int)list.size(), 3);
        check_eq("pop_back 后尾节点 key", list.back<int>(0), 25);
        check_str_eq("pop_back 后尾节点 value", list.back<std::string>(1), "二十五");

        // 验证剩余序列: 15, 20, 25
        std::vector<int> remaining;
        for (auto it = list.begin(); it != list.end(); ++it)
            remaining.push_back(get_key(*it));
        bool seq_ok = (remaining.size() == 3 &&
                       remaining[0] == 15 && remaining[1] == 20 && remaining[2] == 25);
        check("pop 后序列为 15,20,25", seq_ok);

        // 继续删光
        list.pop_front();   // 删除 15
        list.pop_back();    // 删除 25
        check_eq("只剩一个节点", (int)list.size(), 1);
        check_eq("唯一节点 key=20", list.front<int>(0), 20);

        list.pop_front();   // 删除最后一个
        check_eq("全部删除后大小", (int)list.size(), 0);
        check("全部删除后 begin == end", list.begin() == list.end());
    }


    // ============================================================
    print_title("测试结果汇总");
    // ============================================================
    std::cout << "\n  ================================\n";
    std::cout << "   通过: " << g_passed << "\n";
    std::cout << "   失败: " << g_failed << "\n";
    std::cout << "   总计: " << (g_passed + g_failed) << "\n";
    std::cout << "  ================================\n\n";

    return g_failed > 0 ? 1 : 0;
}