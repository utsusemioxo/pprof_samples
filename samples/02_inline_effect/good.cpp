/*
 * 02_inline_effect — good (noinline callee)
 *
 * good_noinline_work() keeps its own stack frame.
 * Expected pprof: good_noinline_work has high flat%; caller is visible in cum.
 */

#include <chrono>
#include <cstdint>
#include <cstdio>

static volatile uint64_t sink = 0;

__attribute__((noinline)) static uint64_t good_noinline_work(uint64_t n) {
    uint64_t x = 3;
    for (uint64_t i = 0; i < n; ++i)
        x = (x ^ i) * 11400714819323198485ull;
    return x;
}

__attribute__((noinline)) static uint64_t good_noinline_caller() {
    uint64_t r = good_noinline_work(1400000);
    __asm__ volatile("" : : "r"(&r) : "memory");  // prevent tail call
    return r;
}

int main() {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    uint64_t rounds = 0;
    while (std::chrono::steady_clock::now() < deadline) {
        sink ^= good_noinline_caller();
        ++rounds;
    }
    std::printf("rounds: %llu\n", static_cast<unsigned long long>(rounds));
}
