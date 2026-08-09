#include "mSkipList.hpp"
#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <memory>

using namespace msl;
using Clock = std::chrono::high_resolution_clock;
using NS   = std::chrono::nanoseconds;

// 全局变量，用于消耗查询结果，防止编译器将查询优化消除
volatile int64_t g_sink = 0;

// 统计结果结构体
struct Stats {
    long long min_ns;
    long long max_ns;
    long long avg_ns;
};

// 执行一次传入的函数，返回耗时（纳秒）
template<typename Func>
static inline long long timed_ns(Func&& f) {
    auto t1 = Clock::now();
    f();
    auto t2 = Clock::now();
    return std::chrono::duration_cast<NS>(t2 - t1).count();
}

// 重复采样，返回 {最小, 最大, 平均}
template<typename Func>
Stats sample_stats(int samples, Func&& f) {
    std::vector<long long> times;
    times.reserve(samples);
    for (int i = 0; i < samples; ++i)
        times.push_back(timed_ns(f));

    auto [min_it, max_it] = std::minmax_element(times.begin(), times.end());
    long long sum = std::accumulate(times.begin(), times.end(), 0LL);
    return {*min_it, *max_it, sum / samples};
}

int main() {
    std::cout << std::fixed << std::setprecision(1);

    // 测试规模列表，可根据机器性能自行调整
    const std::vector<int> data_sizes = {50000};

    for (int N : data_sizes) {
        std::cout << "\n=================== 数据规模: " << N << " ===================\n";

        // ------------------------------------------------------------
        // 1. 查询测试 —— 正常索引（gap=3, max_deep=-1 表示不限制深度）
        // ------------------------------------------------------------
        {
            mSkipList<0, int, int> list(4096, 3, -1, 0, 0);
            for (int i = 0; i < N; ++i) list.insert(i, i * 2);

            // 预热：先执行数百次查询，让缓存和分支预测稳定
            for (int i = 0; i < 500; ++i) g_sink = list.contain(i);

            std::mt19937 rng(42);
            std::uniform_int_distribution<int> dist_exist(0, N - 1);

            // 1a) 查询存在的键（随机命中）
            auto st = sample_stats(3000, [&]() {
                int key = dist_exist(rng);
                g_sink = list.contain(key) ? 1 : 0;
            });
            std::cout << "[查询-存在键]      最小=" << st.min_ns << " 纳秒  最大=" << st.max_ns
                      << " 纳秒  平均=" << st.avg_ns << " 纳秒\n";

            // 1b) 查询不存在的键（搜索路径通常更长）
            std::uniform_int_distribution<int> dist_not_exist(N + 1, 2 * N);
            st = sample_stats(3000, [&]() {
                int key = dist_not_exist(rng);
                g_sink = list.contain(key) ? 1 : 0;
            });
            std::cout << "[查询-不存在键]    最小=" << st.min_ns << " 纳秒  最大=" << st.max_ns
                      << " 纳秒  平均=" << st.avg_ns << " 纳秒\n";

            // 1c) 边界查询：首节点与尾节点
            long long t_first = timed_ns([&]() { g_sink = list.contain(0); });
            long long t_last  = timed_ns([&]() { g_sink = list.contain(N - 1); });
            std::cout << "[查询-首节点]      " << t_first << " 纳秒\n";
            std::cout << "[查询-尾节点]      " << t_last  << " 纳秒\n";
        }

        // ------------------------------------------------------------
        // 2. 查询测试 —— 退化链表（gap=N, max_deep=1，几乎无索引层）
        // ------------------------------------------------------------
        {
            mSkipList<0, int, int> degraded_list(4096, N, 1, 0, 0);
            for (int i = 0; i < N; ++i) degraded_list.insert(i, i * 2);

            std::mt19937 rng(42);
            std::uniform_int_distribution<int> dist(0, N - 1);

            // 退化链表查询很慢，减少采样次数以免测试时间过长
            int samples = std::max(200, N / 500);
            auto st = sample_stats(samples, [&]() {
                int key = dist(rng);
                g_sink = degraded_list.contain(key) ? 1 : 0;
            });
            std::cout << "[查询-退化链表]    最小=" << st.min_ns << " 纳秒  最大=" << st.max_ns
                      << " 纳秒  平均=" << st.avg_ns << " 纳秒  (gap=" << N << ", 最大深度=1)\n";
        }

        // ------------------------------------------------------------
        // 3. 删除测试 —— 正常索引（独立实例法，每个实例只删除一次）
        // ------------------------------------------------------------
        {
            const int instances = 60;
            std::vector<long long> times;
            times.reserve(instances);

            std::mt19937 rng(123);
            std::uniform_int_distribution<int> dist(0, N - 1);

            for (int i = 0; i < instances; ++i) {
                auto list = std::make_unique<mSkipList<0, int, int>>(4096, 3, -1, 0, 0);
                for (int j = 0; j < N; ++j) list->insert(j, j * 2);

                int key = dist(rng);
                times.push_back(timed_ns([&]() { list->erase(key); }));
            }

            auto [min_it, max_it] = std::minmax_element(times.begin(), times.end());
            long long avg = std::accumulate(times.begin(), times.end(), 0LL) / times.size();
            std::cout << "[删除-随机键]      最小=" << *min_it << " 纳秒  最大=" << *max_it
                      << " 纳秒  平均=" << avg << " 纳秒\n";
        }

        // ------------------------------------------------------------
        // 4. 删除测试 —— 退化链表（独立实例法）
        // ------------------------------------------------------------
        {
            const int instances = 30;
            std::vector<long long> times;
            times.reserve(instances);

            std::mt19937 rng(456);
            std::uniform_int_distribution<int> dist(0, N - 1);

            for (int i = 0; i < instances; ++i) {
                auto degraded_list = std::make_unique<mSkipList<0, int, int>>(4096, N, 1, 0, 0);
                for (int j = 0; j < N; ++j) degraded_list->insert(j, j * 2);

                int key = dist(rng);
                times.push_back(timed_ns([&]() { degraded_list->erase(key); }));
            }

            auto [min_it, max_it] = std::minmax_element(times.begin(), times.end());
            long long avg = std::accumulate(times.begin(), times.end(), 0LL) / times.size();
            std::cout << "[删除-退化链表]    最小=" << *min_it << " 纳秒  最大=" << *max_it
                      << " 纳秒  平均=" << avg << " 纳秒  (gap=" << N << ", 最大深度=1)\n";
        }
    }

    return 0;
}