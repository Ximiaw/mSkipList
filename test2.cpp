#include "mSkipList.hpp"
#include <iostream>
#include <cassert>
#include <string>
#include <stdexcept>
#include <vector>

using namespace msl;

// 辅助：从 Data 引用中提取 key（第 0 个字段）
template<typename... T_D>
const auto& get_key(Data<T_D...>& d) {
    return std::get<0>(d.data());
}

// 辅助：从 Data 引用中提取 value（第 1 个字段）
template<typename... T_D>
const auto& get_val(Data<T_D...>& d) {
    return std::get<1>(d.data());
}

int main() {
    std::cout << "=== mSkipList 迭代器专项测试 ===" << std::endl;

    // -------------------------------------------------------
    // 准备数据：key 为 int，附加数据为 std::string
    // -------------------------------------------------------
    mSkipList<0, int, std::string> list = make_mSkipList<0, int, std::string>();
    list.insert(30, "thirty");
    list.insert(10, "ten");
    list.insert(50, "fifty");
    list.insert(20, "twenty");
    list.insert(40, "forty");
    assert(list.size() == 5);

    // -------------------------------------------------------
    // 测试 1: 正向遍历 (前缀 ++)
    // -------------------------------------------------------
    std::cout << "[测试 1] 正向遍历 (前缀 ++)" << std::endl;
    {
        std::vector<int> keys;
        for (auto it = list.begin(); it != list.end(); ++it) {
            keys.push_back(get_key(*it));
        }
        assert((keys == std::vector<int>{10, 20, 30, 40, 50}));
    }

    // -------------------------------------------------------
    // 测试 2: 后缀递增语义
    // -------------------------------------------------------
    std::cout << "[测试 2] 后缀 ++ 语义" << std::endl;
    {
        auto it = list.begin();
        auto old = it++;
        assert(get_key(*old) == 10);   // 返回旧迭代器
        assert(get_key(*it)  == 20);   // 当前迭代器已前进
    }

    // -------------------------------------------------------
    // 测试 3: 反向遍历 (前缀 --)
    // -------------------------------------------------------
    std::cout << "[测试 3] 反向遍历 (前缀 --)" << std::endl;
    {
        std::vector<int> keys;
        auto it = list.end();
        --it; // 指向 50
        while (true) {
            keys.push_back(get_key(*it));
            if (it == list.begin()) break;
            --it;
        }
        assert((keys == std::vector<int>{50, 40, 30, 20, 10}));
    }

    // -------------------------------------------------------
    // 测试 4: 后缀递减语义
    // -------------------------------------------------------
    std::cout << "[测试 4] 后缀 -- 语义" << std::endl;
    {
        auto it = list.end();
        --it; // 50
        auto old = it--;
        assert(get_key(*old) == 50);
        assert(get_key(*it)  == 40);
    }

    // -------------------------------------------------------
    // 测试 5: 解引用 (*) 与箭头 (->)
    // -------------------------------------------------------
    std::cout << "[测试 5] 解引用与箭头操作符" << std::endl;
    {
        auto it = list.begin();
        Data<int, std::string> d = *it; // 拷贝，验证返回真实引用
        assert(get_key(d) == 10);
        assert(get_val(d) == "ten");

        // 箭头操作：it-> 返回 Data*，可直接访问 Data 成员
        assert(std::get<0>(it->data()) == 10);       // 通过箭头直接访问 data()
        assert(std::get<1>(it->data()) == "ten");    // 通过箭头直接访问 data()
        
        // operator->() 返回 Data*，解引用后等价于 *it
        assert(get_key(*(it.operator->())) == 10);
        assert(get_val(*(it.operator->())) == "ten");
    }

    // -------------------------------------------------------
    // 测试 6: 相等与不等比较
    // -------------------------------------------------------
    std::cout << "[测试 6] 相等与不等比较" << std::endl;
    {
        auto a = list.begin();
        auto b = list.begin();
        assert(a == b);
        ++a;
        assert(a != b);
        ++b;
        assert(a == b);
    }

    // -------------------------------------------------------
    // 测试 7: 越界异常安全性
    // -------------------------------------------------------
    std::cout << "[测试 7] 边界异常安全性" << std::endl;
    {
        // ++end() 必须抛异常
        bool caught = false;
        try {
            auto it = list.end();
            ++it;
        } catch (const std::runtime_error&) {
            caught = true;
        }
        assert(caught);

        // 从 begin() 连续 -- 两次应触发越界
        caught = false;
        try {
            auto it = list.begin();
            --it; // 退到 first 哨兵
            --it; // 应抛异常
        } catch (const std::runtime_error&) {
            caught = true;
        }
        assert(caught);
    }

    // -------------------------------------------------------
    // 测试 8: 范围 for 循环
    // -------------------------------------------------------
    std::cout << "[测试 8] 范围 for 循环" << std::endl;
    {
        std::vector<int> keys;
        for (auto& data : list) {
            keys.push_back(get_key(data));
        }
        assert((keys == std::vector<int>{10, 20, 30, 40, 50}));
    }

    // -------------------------------------------------------
    // 测试 9: 子区间 range() 迭代
    // -------------------------------------------------------
    std::cout << "[测试 9] 子区间 range()" << std::endl;
    {
        // 取闭区间 [20, 40]
        auto sub = list.range(20, 40);
        std::vector<int> keys;
        for (auto it = sub.begin(); it != sub.end(); ++it) {
            keys.push_back(get_key(*it));
        }
        assert((keys == std::vector<int>{20, 30, 40}));
    }

    // -------------------------------------------------------
    // 测试 10: 单元素表
    // -------------------------------------------------------
    std::cout << "[测试 10] 单元素表" << std::endl;
    {
        mSkipList<0, int, std::string> single = make_mSkipList<0, int, std::string>();
        single.insert(100, "century");
        assert(single.size() == 1);

        auto it = single.begin();
        assert(get_key(*it) == 100);
        ++it;
        assert(it == single.end());

        --it;
        assert(get_key(*it) == 100);
    }

    // -------------------------------------------------------
    // 测试 11: 空表
    // -------------------------------------------------------
    std::cout << "[测试 11] 空表" << std::endl;
    {
        mSkipList<0, int, std::string> empty = make_mSkipList<0, int, std::string>();
        assert(empty.begin() == empty.end());
    }

    // -------------------------------------------------------
    // 测试 12: 更新已有 key 后迭代器可见性
    // -------------------------------------------------------
    std::cout << "[测试 12] 键值更新后的可见性" << std::endl;
    {
        list.insert(20, "TWENTY"); // 覆盖原值
        auto it = list.begin();
        ++it; // 指向 20
        assert(get_val(*it) == "TWENTY");
    }

    std::cout << "=== 所有迭代器测试通过！===" << std::endl;
    return 0;
}