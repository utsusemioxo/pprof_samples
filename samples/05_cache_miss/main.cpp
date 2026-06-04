/*
 * 05_cache_miss
 *
 * Concept:
 *   Sequential and random memory access are both O(n), but hardware behavior is
 *   very different. pprof may show the same loop while timing shows cache cost.
 *
 * Predict before running:
 *   bad_random_access() is much slower because each load is harder to prefetch.
 *   good_sequential_access() streams through memory.
 *
 * Run:
 *   ./build/bin/sample_05_cache_miss
 */

#include "rdtsc.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <numeric>
#include <vector>

static void escape(void* p) { __asm__ volatile("" : : "r"(p) : "memory"); }
static volatile uint64_t sink = 0;

__attribute__((noinline)) static uint64_t good_sequential_access(const std::vector<uint64_t>& data) {
    uint64_t sum = 0;
    for (uint64_t v : data) {
        sum += v;
    }
    sink ^= sum;
    return sum;
}

__attribute__((noinline)) static uint64_t bad_random_access(const std::vector<uint64_t>& data,
                                                            const std::vector<size_t>& order) {
    uint64_t sum = 0;
    for (size_t idx : order) {
        sum += data[idx];
    }
    sink ^= sum;
    return sum;
}

int main() {
    init_rdtsc();
    print_separator("cache miss");

    const size_t n = 8 * 1024 * 1024;
    std::vector<uint64_t> data(n);
    std::iota(data.begin(), data.end(), 1);
    std::vector<size_t> order(n);
    std::iota(order.begin(), order.end(), 0);

    uint32_t x = 1;
    for (size_t i = order.size() - 1; i > 0; --i) {
        x = x * 1664525u + 1013904223u;
        std::swap(order[i], order[x % (i + 1)]);
    }
    escape(data.data());
    escape(order.data());

    auto seq_cycles = measure_median([&] { good_sequential_access(data); }, 5);
    auto random_cycles = measure_median([&] { bad_random_access(data, order); }, 5);

    print_result("good sequential access", seq_cycles);
    print_result("bad random access", random_cycles);
    std::printf("slowdown: %.2fx\n", static_cast<double>(random_cycles) / seq_cycles);

    std::printf("\npprof expectation:\n");
    std::printf("  The loops look equally simple in code; the timing exposes memory locality.\n");
    std::printf("sink=%llu\n", static_cast<unsigned long long>(sink));
    return 0;
}
