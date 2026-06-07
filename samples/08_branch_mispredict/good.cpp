/*
 * 08_branch_mispredict — good (sorted data, predictable branch)
 *
 * Same data and same branch, but sorted: all negatives first, then positives.
 * Branch predictor learns the pattern quickly; almost no mispredictions.
 * Expected pprof: noticeably lower cycle count vs bad variant.
 */

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <vector>

static volatile uint64_t sink = 0;

static std::vector<int> make_random(size_t n) {
    std::vector<int> v(n);
    uint32_t x = 0xdeadbeef;
    for (auto& e : v) {
        x = x * 1664525u + 1013904223u;
        e = static_cast<int>(x) - (1 << 30);
    }
    return v;
}

int main() {
    constexpr size_t N = 1 << 16;
    auto data = make_random(N);
    std::sort(data.begin(), data.end());  // sort once; branch becomes predictable
    uint64_t sum = 0;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    uint64_t rounds = 0;
    while (std::chrono::steady_clock::now() < deadline) {
        for (int v : data) {
            if (v > 0) sum += static_cast<uint64_t>(v);
        }
        ++rounds;
    }
    sink = sum;
    std::printf("rounds: %llu\n", static_cast<unsigned long long>(rounds));
}
