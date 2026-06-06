/*
 * 01_flat_vs_cum — good (direct path)
 *
 * good_leaf() does all work directly without delegation.
 * Expected pprof: good_leaf high flat%.
 */

#include <chrono>
#include <cstdint>
#include <cstdio>

static volatile uint64_t sink = 0;

__attribute__((noinline)) static uint64_t burn(uint64_t n, uint64_t seed) {
    uint64_t x = seed;
    for (uint64_t i = 0; i < n; ++i)
        x = x * 1664525u + 1013904223u + (i & 7u);
    sink ^= x;
    return x;
}

__attribute__((noinline)) static uint64_t good_leaf() { return burn(2400000, 47); }

int main() {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    uint64_t rounds = 0;
    while (std::chrono::steady_clock::now() < deadline) {
        sink ^= good_leaf();
        ++rounds;
    }
    std::printf("rounds: %llu\n", static_cast<unsigned long long>(rounds));
}
