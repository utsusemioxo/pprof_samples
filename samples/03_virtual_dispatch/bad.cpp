/*
 * 03_virtual_dispatch — bad (virtual dispatch)
 *
 * Calls through a base pointer: every call goes through the vtable.
 * Expected pprof: hot samples spread across vtable dispatch + Derived::compute,
 * the indirection cost is visible as extra overhead vs the good variant.
 */

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <memory>

static volatile uint64_t sink = 0;

struct Processor {
    virtual uint64_t compute(uint64_t x) const = 0;
    virtual ~Processor() = default;
};

struct DerivedProcessor : Processor {
    __attribute__((noinline)) uint64_t compute(uint64_t x) const override {
        return x * 6364136223846793005ULL + 1442695040888963407ULL;
    }
};

int main() {
    std::unique_ptr<Processor> p = std::make_unique<DerivedProcessor>();
    uint64_t x = 1;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    uint64_t rounds = 0;
    while (std::chrono::steady_clock::now() < deadline) {
        x = p->compute(x);
        ++rounds;
    }
    sink = x;
    std::printf("rounds: %llu\n", static_cast<unsigned long long>(rounds));
}
