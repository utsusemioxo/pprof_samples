/*
 * 08_branch_mispredict — bad (random data, unpredictable branch)
 *
 * The branch `if (data[i] > 0)` is unpredictable on random data.
 * CPU pipeline stalls on every misprediction.
 * Expected pprof: high cycle count in the loop body relative to good variant.
 */

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
        e = static_cast<int>(x) - (1 << 30);  // roughly half positive, half negative
    }
    return v;
}

int main() {
    constexpr size_t N = 1 << 16;
    auto data = make_random(N);
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
