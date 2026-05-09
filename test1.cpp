#include "mSkipList.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <iterator>
#include <type_traits>

using namespace msl;

void print_title(const std::string& s) {
    std::cout << "\n========== " << s << " ==========\n";
}

int main() {
    // 主键 keyIndex = 0（int），附加数据为 std::string
    print_title("构造跳表并插入测试数据");
    mSkipList<0, int, std::string> list(1024, 3, -1, 0, "");

    std::vector<std::pair<int, std::string>> data = {
        {10, "十"}, {20, "二十"}, {5, "五"},
        {30, "三十"}, {15, "十五"}, {25, "二十五"}
    };
    for (auto& p : data) {
        list.insert(p.first, p.second);
        std::cout << "已插入: key=" << p.first << ", value=" << p.second << "\n";
    }
    std::cout << "跳表当前大小: " << list.size() << "（预期: 6）\n";

    // ============================================================
    print_title("测试1: 正向遍历（operator* 与 operator++）");
    int idx = 0;
    std::vector<int> expected = {5, 10, 15, 20, 25, 30};
    for (auto it = list.begin(); it != list.end(); ++it) {
        auto tup = (*it).data();          // 使用 operator*
        int k = std::get<0>(tup);
        std::cout << "  节点[" << idx << "]: key=" << k
                  << ", value=" << std::get<1>(tup)
                  << (k == expected[idx] ? "  ✓" : "  ✗") << "\n";
        ++idx;
    }
    std::cout << "遍历完成，节点总数: " << idx << "（预期: 6）\n";

    // ============================================================
    print_title("测试2: 反向遍历（operator--）");
    if (list.begin() != list.end()) {
        auto it = list.end();
        int rev_idx = (int)expected.size() - 1;
        while (true) {
            --it;
            auto tup = it->data();        // 使用 operator->
            int k = std::get<0>(tup);
            std::cout << "  反向节点: key=" << k
                      << (k == expected[rev_idx] ? "  ✓" : "  ✗") << "\n";
            if (it == list.begin()) break;
            --rev_idx;
        }
    }

    // ============================================================
    print_title("测试3: 范围 for 循环（C++11 语法，值传递）");
    idx = 0;
    // 注意：这里必须使用 auto view（值传递），因为原头文件中 View::data() 非 const
    for (auto view : list) {
        auto tup = view.data();
        std::cout << "  元素[" << idx++ << "]: "
                  << std::get<0>(tup) << " -> " << std::get<1>(tup) << "\n";
    }

    // ============================================================
    print_title("测试4: 前置++ 与 后置++ 语义区别");
    auto it_post = list.begin();
    auto it_pre  = list.begin();

    auto val_post = (*(it_post++)).data();   // 后置：先返回旧值，再移动
    auto val_pre  = (*++it_pre).data();      // 前置：先移动，再返回新值

    std::cout << "  后置++ 返回值: key=" << std::get<0>(val_post)
              << "（预期: 5）\n";
    std::cout << "  前置++ 返回值: key=" << std::get<0>(val_pre)
              << "（预期: 10）\n";
    std::cout << "  后置++ 后迭代器位置: key="
              << std::get<0>((*(it_post)).data()) << "（预期: 10）\n";
    std::cout << "  前置++ 后迭代器位置: key="
              << std::get<0>((*(it_pre)).data()) << "（预期: 10）\n";
    std::cout << "  两迭代器现在是否相等: "
              << (it_post == it_pre ? "是  ✓" : "否  ✗") << "\n";

    // ============================================================
    print_title("测试5: 前置-- 与 后置-- 语义区别");
    auto it_rpost = list.end();
    auto it_rpre  = list.end();
    --it_rpost;  // 移到最后一个有效节点
    --it_rpre;

    auto val_rpost = (*(it_rpost--)).data(); // 后置
    auto val_rpre  = (*--it_rpre).data();    // 前置

    std::cout << "  后置-- 返回值: key=" << std::get<0>(val_rpost)
              << "（预期: 30）\n";
    std::cout << "  前置-- 返回值: key=" << std::get<0>(val_rpre)
              << "（预期: 25）\n";
    std::cout << "  后置-- 后迭代器位置: key="
              << std::get<0>((*(it_rpost)).data()) << "（预期: 25）\n";
    std::cout << "  前置-- 后迭代器位置: key="
              << std::get<0>((*(it_rpre)).data()) << "（预期: 25）\n";
    std::cout << "  两迭代器现在是否相等: "
              << (it_rpost == it_rpre ? "是  ✓" : "否  ✗") << "\n";

    // ============================================================
    print_title("测试6: 迭代器比较运算符（== 与 !=）");
    auto a = list.begin();
    auto b = list.begin();
    auto c = list.end();
    std::cout << "  begin == begin: " << (a == b ? "是  ✓" : "否  ✗")
              << "（预期: 是）\n";
    std::cout << "  begin != end:   " << (a != c ? "是  ✓" : "否  ✗")
              << "（预期: 是）\n";
    std::cout << "  begin == end:   " << (a == c ? "是" : "否")
              << "（预期: 否）\n";

    // ============================================================
    print_title("测试7: 越界访问异常安全性");
    try {
        auto it = list.end();
        ++it;
        std::cout << "  前置++ 越界: 未捕获异常  ✗\n";
    } catch (const std::runtime_error& e) {
        std::cout << "  前置++ 越界: 捕获异常  ✓ -> " << e.what() << "\n";
    }

    try {
        auto it = list.begin();
        --it;
        std::cout << "  前置-- 越界: 未捕获异常  ✗\n";
    } catch (const std::runtime_error& e) {
        std::cout << "  前置-- 越界: 捕获异常  ✓ -> " << e.what() << "\n";
    }

    try {
        auto it = list.end();
        it++;
        std::cout << "  后置++(end) 越界: 未捕获异常  ✗\n";
    } catch (const std::runtime_error& e) {
        std::cout << "  后置++(end) 越界: 捕获异常  ✓ -> " << e.what() << "\n";
    }

    // ============================================================
    print_title("测试8: 范围查询迭代（Range）");
    auto rng = list.range(10, 25);
    std::cout << "  查询闭区间 [10, 25] 内的元素:\n";
    int cnt = 0;
    for (auto it = rng.begin(); it != rng.end(); ++it) {
        auto tup = it->data();
        std::cout << "    key=" << std::get<0>(tup)
                  << ", value=" << std::get<1>(tup) << "\n";
        ++cnt;
    }
    std::cout << "  范围内元素数量: " << cnt
              << "（预期: 4，即 10, 15, 20, 25）\n";

    // ============================================================
    print_title("测试9: 空表迭代行为");
    mSkipList<0, int, std::string> empty_list(1024, 3, -1, 0, "");
    std::cout << "  空表 begin == end: "
              << (empty_list.begin() == empty_list.end() ? "是  ✓" : "否  ✗")
              << "（预期: 是）\n";
    int empty_cnt = 0;
    for (auto it = empty_list.begin(); it != empty_list.end(); ++it) ++empty_cnt;
    std::cout << "  空表遍历节点数: " << empty_cnt << "（预期: 0）\n";

    // ============================================================
    print_title("测试10: 基于迭代器的删除（erase）");
    std::cout << "  删除前大小: " << list.size() << "\n";
    {
        auto it = list.begin();
        ++it; // 现在指向 key=10
        std::cout << "  待删除迭代器指向: key="
                  << std::get<0>(it->data()) << "（预期: 10）\n";
        auto nxt = list.erase(it);
        std::cout << "  删除后返回迭代器指向: key="
                  << std::get<0>(nxt->data()) << "（预期: 15）\n";
    }
    std::cout << "  删除后大小: " << list.size() << "（预期: 5）\n";
    std::cout << "  删除后正向遍历结果: ";
    for (auto it = list.begin(); it != list.end(); ++it) {
        std::cout << std::get<0>(it->data()) << " ";
    }
    std::cout << "（预期: 5 15 20 25 30）\n";

    // ============================================================
    print_title("测试11: 迭代器标签与类型特性");
    using Iter = decltype(list.begin());
    static_assert(std::is_same_v<typename std::iterator_traits<Iter>::iterator_category,
                                 std::bidirectional_iterator_tag>,
                  "迭代器标签应为 bidirectional_iterator_tag");
    std::cout << "  迭代器标签验证: bidirectional_iterator_tag  ✓\n";

    // ============================================================
    print_title("测试结束");
    std::cout << "所有迭代器专项测试已执行完毕，请核对输出中的 ✓ 与 ✗。\n";
    return 0;
}