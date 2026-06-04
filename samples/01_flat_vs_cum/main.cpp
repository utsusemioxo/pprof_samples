/*
 * 01_flat_vs_cum
 *
 * Concept:
 *   pprof flat% counts only samples inside the function itself.
 *   cum% includes samples in the whole subtree below that function.
 *
 * Predict before running:
 *   bad_parent() should have low flat% but high cum%, because most time is in
 *   bad_child_a() and bad_child_b(). good_leaf() should show high flat% because
 *   it does its own work directly.
 *
 * Run:
 *   ./build/bin/sample_01_flat_vs_cum
 */

#include "rdtsc.h"

#include <chrono>
#include <cstdint>
#include <cstdio>

static void escape(void* p) { __asm__ volatile("" : : "r"(p) : "memory"); }

static volatile uint64_t sink = 0;

extern "C" __attribute__((noinline)) uint64_t burn(uint64_t n, uint64_t seed) {
    uint64_t x = seed;
    for (uint64_t i = 0; i < n; ++i) {
        x = x * 1664525u + 1013904223u + (i & 7u);
    }
    sink ^= x;
    return x;
}

extern "C" __attribute__((noinline)) uint64_t bad_child_a() {
    return burn(1200000, 11);
}

extern "C" __attribute__((noinline)) uint64_t bad_child_b() {
    return burn(1200000, 29);
}

extern "C" __attribute__((noinline)) uint64_t bad_parent() {
    uint64_t local = 7;
    escape(&local);
    return bad_child_a() + bad_child_b() + local;
}

extern "C" __attribute__((noinline)) uint64_t good_leaf() {
    return burn(2400000, 47);
}

extern "C" __attribute__((noinline)) void run_profile_workload() {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    uint64_t rounds = 0;
    while (std::chrono::steady_clock::now() < deadline) {
        sink ^= bad_parent();
        sink ^= good_leaf();
        ++rounds;
    }
    std::printf("profile workload rounds: %llu\n", static_cast<unsigned long long>(rounds));
}

int main() {
    init_rdtsc();
    print_separator("flat% vs cum%");

    auto bad_cycles = measure_median([] { sink ^= bad_parent(); });
    auto good_cycles = measure_median([] { sink ^= good_leaf(); });

    print_result("bad_parent via callees", bad_cycles);
    print_result("good_leaf direct work", good_cycles);

    print_separator("profiling workload");
    run_profile_workload();

    std::printf("\npprof expectation:\n");
    std::printf("  bad_parent: low flat%%, high cum%%; the flame graph box is wide because children are hot.\n");
    std::printf("  good_leaf:   high flat%%; samples land directly in the measured function.\n");
    std::printf("sink=%llu\n", static_cast<unsigned long long>(sink));
    return 0;
}
