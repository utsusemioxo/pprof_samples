/*
 * 01_flat_vs_cum — bad (indirect path)
 *
 * bad_parent() delegates to bad_child_a() and bad_child_b(), which call burn().
 * Expected pprof: bad_parent low flat%, high cum%; burn high flat%.
 */

#include <chrono>
#include <cstdint>
#include <cstdio>

static volatile uint64_t sink = 0;
static void escape(void* p) { __asm__ volatile("" : : "r"(p) : "memory"); }

__attribute__((noinline)) static uint64_t burn(uint64_t n, uint64_t seed) {
    uint64_t x = seed;
    for (uint64_t i = 0; i < n; ++i)
        x = x * 1664525u + 1013904223u + (i & 7u);
    sink ^= x;
    return x;
}

__attribute__((noinline)) static uint64_t bad_child_a() {
    uint64_t r = burn(1200000, 11);
    escape(&r);  // prevent tail-call: forces a live stack frame past the burn() call
    return r;
}
__attribute__((noinline)) static uint64_t bad_child_b() {
    uint64_t r = burn(1200000, 29);
    escape(&r);
    return r;
}

__attribute__((noinline)) static uint64_t bad_parent() {
    uint64_t local = 7;
    escape(&local);
    return bad_child_a() + bad_child_b() + local;
}

int main() {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    uint64_t rounds = 0;
    while (std::chrono::steady_clock::now() < deadline) {
        sink ^= bad_parent();
        ++rounds;
    }
    std::printf("rounds: %llu\n", static_cast<unsigned long long>(rounds));
}
