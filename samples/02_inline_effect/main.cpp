/*
 * 02_inline_effect
 *
 * Concept:
 *   RelWithDebInfo still optimizes. Inlined functions may disappear from pprof
 *   call chains because their instructions are charged to the caller.
 *
 * Predict before running:
 *   bad_inlined_work() is always_inline, so pprof should usually show time in
 *   bad_inline_caller(), not in bad_inlined_work(). good_noinline_work() keeps a
 *   visible frame.
 *
 * Run:
 *   ./build/bin/sample_02_inline_effect
 */

#include "rdtsc.h"

#include <cstdint>
#include <cstdio>

static volatile uint64_t sink = 0;

__attribute__((always_inline)) static inline uint64_t bad_inlined_work(uint64_t n) {
    uint64_t x = 3;
    for (uint64_t i = 0; i < n; ++i) {
        x = (x ^ i) * 11400714819323198485ull;
    }
    return x;
}

__attribute__((noinline)) static uint64_t good_noinline_work(uint64_t n) {
    uint64_t x = 3;
    for (uint64_t i = 0; i < n; ++i) {
        x = (x ^ i) * 11400714819323198485ull;
    }
    return x;
}

__attribute__((noinline)) static uint64_t bad_inline_caller() {
    return bad_inlined_work(1400000);
}

__attribute__((noinline)) static uint64_t good_noinline_caller() {
    return good_noinline_work(1400000);
}

int main() {
    init_rdtsc();
    print_separator("inlining effect");

    auto bad_cycles = measure_median([] { sink ^= bad_inline_caller(); });
    auto good_cycles = measure_median([] { sink ^= good_noinline_caller(); });

    print_result("bad always_inline callee", bad_cycles);
    print_result("good noinline callee", good_cycles);

    std::printf("\npprof expectation:\n");
    std::printf("  bad_inlined_work can vanish; the caller receives the samples.\n");
    std::printf("  good_noinline_work remains visible as its own stack frame.\n");
    std::printf("sink=%llu\n", static_cast<unsigned long long>(sink));
    return 0;
}
