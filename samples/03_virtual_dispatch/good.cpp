/*
 * 03_virtual_dispatch — good (CRTP, compile-time polymorphism)
 *
 * CRTP resolves the derived type at compile time: no vtable, no indirect call.
 * The compiler can inline do_compute() into the hot loop entirely.
 * Expected pprof: cycles land directly in main or do_compute; no dispatch layer.
 */

#include <chrono>
#include <cstdint>
#include <cstdio>

static volatile uint64_t sink = 0;

template <typename Derived>
struct ProcessorBase {
    uint64_t compute(uint64_t x) const {
        return static_cast<const Derived*>(this)->do_compute(x);
    }
};

struct Processor : ProcessorBase<Processor> {
    __attribute__((noinline)) uint64_t do_compute(uint64_t x) const {
        return x * 6364136223846793005ULL + 1442695040888963407ULL;
    }
};

int main() {
    Processor p;
    uint64_t x = 1;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    uint64_t rounds = 0;
    while (std::chrono::steady_clock::now() < deadline) {
        x = p.compute(x);
        ++rounds;
    }
    sink = x;
    std::printf("rounds: %llu\n", static_cast<unsigned long long>(rounds));
}
