#include "mSkipList.hpp"
#include <iostream>
#include <string>
#include <random>
#include <chrono>
#include <map>
#include <cassert>
#include <vector>
#include <algorithm>

// ==================== 结构验证 ====================
// 逐层遍历跳表，验证每层索引严格升序
template<int keyIndex, typename... T_D>
bool validate_skiplist(mSkipList<keyIndex, T_D...>* msl) {
    auto* first = msl->fir();
    int max_deep = first->rightIndex.size();

    for (int d = 0; d < max_deep; ++d) {
        auto* cur = first->rightIndex[d];
        int prev = INT_MIN;
        while (cur && cur->rightIndex.size() > d) {
            // last 哨兵的 rightIndex 为空，循环会在 last 之前自然终止
            int val = cur->data().template ref<int>(keyIndex);
            if (val < prev) {
                std::cerr << "层 " << d << " 发现逆序: " << prev << " > " << val << std::endl;
                return false;
            }
            prev = val;
            cur = cur->rightIndex[d];
        }
    }
    return true;
}

// ==================== 全量逐元素对比 ====================
// 遍历跳表底层链表，与 std::map 逐 key-value 对比
template<int keyIndex, typename... T_D>
bool full_compare(mSkipList<keyIndex, T_D...>* msl, const std::map<int, std::string>& ref) {
    auto* node = msl->fir();
    auto it = ref.begin();

    while (node->rightIndex.size() > 0) {
        node = node->rightIndex[0];
        // 到达 last 哨兵时退出（last->rightIndex 为空）
        if (node->rightIndex.size() == 0) break;

        int key = node->data().template ref<int>(0);
        const std::string& val = node->data().template ref<std::string>(1);

        if (it == ref.end()) return false;
        if (it->first != key) {
            std::cerr << "key 不匹配: map=" << it->first << " skiplist=" << key << std::endl;
            return false;
        }
        if (it->second != val) {
            std::cerr << "value 不匹配: key=" << key << std::endl;
            return false;
        }
        ++it;
    }
    return it == ref.end();
}

int main() {
    std::random_device rd;
    auto num__=rd();
    //auto num__=209743480;
    std::cout<<"随机数种子："<<num__<<std::endl;
    std::mt19937 gen(num__);
    std::uniform_int_distribution<> dis(1, 100000000);

    // ==================== 阶段1：随机插入 100 万 + 同步 map ====================
    std::cout << "=== 阶段1：随机插入 100 万 (int, string) ===" << std::endl;
    std::map<int, std::string> ref;
    mSkipList<0, int, std::string> msl{0, ""};

    auto t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000000; ++i) {
        int key = dis(gen);
        std::string val = std::to_string(key);  // 用确定性值便于对照
        msl.insert(key, val);
        ref[key] = val;
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    auto insert_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
    std::cout << "插入耗时: " << insert_ns << " ns (" << insert_ns / 1e6 << " ms)" << std::endl;
    std::cout << "实际节点数(map): " << ref.size() << std::endl;

    // ==================== 阶段2：插入后验证 ====================
    std::cout << "\n=== 阶段2：插入后验证 ===" << std::endl;

    assert(validate_skiplist(&msl));
    std::cout << "结构验证(多层索引有序): 通过" << std::endl;

    assert(full_compare(&msl, ref));
    std::cout << "全量逐元素对比: 通过" << std::endl;

    // 随机抽查查找
    bool find_ok = true;
    std::uniform_int_distribution<> check_dis(1, 100000000);
    for (int i = 0; i < 100000; ++i) {
        int key = check_dis(gen);
        auto it = ref.find(key);
        try {
            const int& found = msl.get<int>(key);
            (void)found;
            if (it == ref.end()) find_ok = false;  // 跳表有，map 没有（不应发生）
        } catch (const std::runtime_error&) {
            if (it != ref.end()) find_ok = false;  // map 有，跳表找不到
        }
    }
    std::cout << "随机查找对照(10万次): " << (find_ok ? "通过" : "失败") << std::endl;

    // ==================== 阶段3：删除约一半节点 ====================
    std::cout << "\n=== 阶段3：删除约一半节点 ===" << std::endl;
    std::vector<int> to_delete;
    for (const auto& [k, v] : ref) {
        if (dis(gen) % 2 == 0) to_delete.push_back(k);
    }

    auto t3 = std::chrono::high_resolution_clock::now();
    for (int key : to_delete) {
        msl.erase(key);  // 这些 key 一定存在，不应抛异常
    }
    auto t4 = std::chrono::high_resolution_clock::now();
    auto erase_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t4 - t3).count();
    std::cout << "删除 " << to_delete.size() << " 个节点" << std::endl;
    std::cout << "删除耗时: " << erase_ns << " ns (" << erase_ns / 1e6 << " ms)" << std::endl;

    for (int key : to_delete) ref.erase(key);

    // ==================== 阶段4：删除后验证 ====================
    std::cout << "\n=== 阶段4：删除后验证 ===" << std::endl;

    assert(validate_skiplist(&msl));
    std::cout << "结构验证(多层索引有序): 通过" << std::endl;

    assert(full_compare(&msl, ref));
    std::cout << "剩余节点全量对比: 通过" << std::endl;

    // 剩余节点全部可查
    bool remain_ok = true;
    for (const auto& [k, v] : ref) {
        try {
            msl.get<int>(k);
        } catch (...) {
            remain_ok = false; break;
        }
    }
    std::cout << "剩余节点查找: " << (remain_ok ? "通过" : "失败") << std::endl;

    // 已删除节点应全部抛异常
    bool gone_ok = true;
    int del_check = std::min((int)to_delete.size(), 50000);
    for (int i = 0; i < del_check; ++i) {
        try {
            msl.get<int>(to_delete[i]);
            gone_ok = false; break;  // 找到了，说明没删干净
        } catch (const std::runtime_error&) {
            // 正确行为
        }
    }
    std::cout << "已删除节点隔离(" << del_check << "次抽查): " << (gone_ok ? "通过" : "失败") << std::endl;

    // ==================== 阶段5：纯 int 性能基线 ====================
    std::cout << "\n=== 阶段5：纯 int 性能基线 ===" << std::endl;

    // 5a. 顺序插入
    mSkipList<0, int> seq_msl{0};
    auto t5 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000000; ++i) seq_msl.insert(i);
    auto t6 = std::chrono::high_resolution_clock::now();
    auto seq_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t6 - t5).count();
    std::cout << "顺序插入 100 万: " << seq_ns << " ns (" << seq_ns / 1e6 << " ms), 层高: " << seq_msl.get_deep() << std::endl;

    // 5b. 随机插入
    mSkipList<0, int> rand_msl{0};
    auto t7 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000000; ++i) rand_msl.insert(dis(gen));
    auto t8 = std::chrono::high_resolution_clock::now();
    auto rand_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t8 - t7).count();
    std::cout << "随机插入 100 万: " << rand_ns << " ns (" << rand_ns / 1e6 << " ms), 层高: " << rand_msl.get_deep() << std::endl;

    // 5c. 随机删除（约 50 万次尝试）
    std::vector<int> del_keys;
    for (int i = 0; i < 1000000; ++i) del_keys.push_back(dis(gen));
    auto t9 = std::chrono::high_resolution_clock::now();
    int erase_success = 0;
    for (int i = 0; i < 500000; ++i) {
        try {
            rand_msl.erase(del_keys[i]);
            ++erase_success;
        } catch (...) {
            // key 不存在（重复或随机未命中）
        }
    }
    auto t10 = std::chrono::high_resolution_clock::now();
    auto del_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t10 - t9).count();
    std::cout << "随机删除 50 万次尝试: " << del_ns << " ns (" << del_ns / 1e6 << " ms), 成功: " << erase_success << std::endl;

    std::cout << "\n=== 全部测试完成 ===" << std::endl;

    return 0;
}