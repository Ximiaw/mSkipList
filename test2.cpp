#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <chrono>
#include <random>
#include "mSkipList.hpp"

using namespace std;
using namespace msl;
using namespace std::chrono;

// 防优化累加器
volatile size_t g_dummy = 0;

template<typename Func>
auto bench(Func&& f) -> long long {
    auto s = high_resolution_clock::now();
    f();
    auto e = high_resolution_clock::now();
    return duration_cast<milliseconds>(e - s).count();
}

int main() {
    auto rd = random_device();
    auto m = mt19937(rd());
    size_t random_max = 500000;
    auto r = uniform_int_distribution<>(1, random_max);
    size_t max = 1000000;

    vector<size_t> v;
    set<size_t> s;
    for (size_t i = 0; i < max; i++) {
        size_t si = r(m);
        v.push_back(si);
        s.insert(si);
    }

    // ========== 随机测试 ==========
    cout << "随机插查" << max << "条1~" << random_max << endl;

    // 1. map 插入
    auto m_map = map<size_t, size_t>{};
    cout << "map插入:" << bench([&]() {
        for (size_t i = 0; i < v.size(); i++) {
            m_map.insert({v[i], v[i]});
        }
    }) << "ms" << endl;

    // 2. map 查找（在已填充的 map 上，用 find + 防优化）
    cout << "map查找:" << bench([&]() {
        size_t acc = 0;
        for (size_t i = 0; i < v.size(); i++) {
            auto it = m_map.find(v[i]);
            if (it != m_map.end()) acc += it->second;
        }
        g_dummy = acc;  // 强制写出，防止优化
    }) << "ms" << endl;

    // 3. map 删除（基于 set 的 key 序列，返回值累加防优化）
    size_t map_erase_cnt = 0;
    cout << "map删除:" << bench([&]() {
        size_t cnt = 0;
        for (auto it = s.begin(); it != s.end(); ++it) {
            cnt += m_map.erase(*it);
        }
        map_erase_cnt = cnt;
        g_dummy = cnt;
    }) << "ms" << endl;

    // 4. 跳表插入
    auto m_sl = mSkipList<0, size_t, size_t>(4096, 3, -1, 0, 0);
    cout << "mSkipList插入:" << bench([&]() {
        for (size_t i = 0; i < v.size(); i++) {
            m_sl.insert(v[i], v[i]);
        }
    }) << "ms" << endl;

    // 5. 跳表查找
    cout << "mSkipList查找:" << bench([&]() {
        size_t acc = 0;
        for (size_t i = 0; i < v.size(); i++) {
            acc += m_sl.get<size_t>(v[i], 0);
        }
        g_dummy = acc;
    }) << "ms" << endl;

    // 6. 跳表删除
    cout << "mSkipList删除:" << bench([&]() {
        for (auto it = s.begin(); it != s.end(); ++it) {
            m_sl.erase(*it);
        }
    }) << "ms" << endl;

    // ========== 顺序测试 ==========
    cout << "\n顺序插查" << max << "条" << endl;
    {
        // 1. map 顺序插入
        m_map = map<size_t, size_t>{};
        cout << "map插入:" << bench([&]() {
            for (size_t i = 0; i < max; i++) {
                m_map.insert({i, i});
            }
        }) << "ms" << endl;

        // 2. map 顺序查找（在已填充的 map 上）
        cout << "map查找:" << bench([&]() {
            size_t acc = 0;
            for (size_t i = 0; i < max; i++) {
                auto it = m_map.find(i);
                if (it != m_map.end()) acc += it->second;
            }
            g_dummy = acc;
        }) << "ms" << endl;

        // 3. map 顺序删除
        cout << "map删除:" << bench([&]() {
            size_t cnt = 0;
            for (size_t i = 0; i < max; i++) {
                cnt += m_map.erase(i);
            }
            g_dummy = cnt;
        }) << "ms" << endl;
    }
    {
        // 4. 跳表顺序插入
        auto m_sl = mSkipList<0, size_t, size_t>(4096, 3, -1, 0, 0);
        cout << "mSkipList插入:" << bench([&]() {
            for (size_t i = 0; i < max; i++) {
                m_sl.insert(i, i);
            }
        }) << "ms" << endl;

        // 5. 跳表顺序查找
        cout << "mSkipList查找:" << bench([&]() {
            size_t acc = 0;
            for (size_t i = 0; i < max; i++) {
                acc += m_sl.get<size_t>(i, 0);
            }
            g_dummy = acc;
        }) << "ms" << endl;

        // 6. 跳表顺序删除
        cout << "mSkipList删除:" << bench([&]() {
            for (size_t i = 0; i < max; i++) {
                m_sl.erase(i);
            }
        }) << "ms" << endl;
    }
    return 0;
}