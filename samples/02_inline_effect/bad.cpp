/*
 * 02_inline_effect — bad (always_inline callee)
 *
 * bad_inlined_work() is always_inline, so it disappears from the pprof call chain.
 * Expected pprof: bad_inline_caller has high flat%; bad_inlined_work is invisible.
 */

#include <chrono>
#include <cstdint>
#include <cstdio>

static volatile uint64_t sink = 0;

__attribute__((always_inline)) static inline uint64_t bad_inlined_work(uint64_t n) {
    uint64_t x = 3;
    for (uint64_t i = 0; i < n; ++i)
        x = (x ^ i) * 11400714819323198485ull;
    return x;
}

__attribute__((noinline)) static uint64_t bad_inline_caller() {
    uint64_t r = bad_inlined_work(1400000);
    __asm__ volatile("" : : "r"(&r) : "memory");  // prevent tail call
    return r;
}

int main() {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    uint64_t rounds = 0;
    while (std::chrono::steady_clock::now() < deadline) {
        sink ^= bad_inline_caller();
        ++rounds;
    }
    std::printf("rounds: %llu\n", static_cast<unsigned long long>(rounds));
}
