/*
 * 06_vector_reserve — bad (no reserve, repeated reallocation)
 *
 * push_back without reserve triggers ~log2(N) reallocations as the vector
 * doubles its capacity. Each reallocation is: Scudo alloc + memcpy + free.
 * Expected pprof: Scudo alloc/dealloc visible as hot frames.
 */

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <vector>

static volatile uint64_t sink = 0;

int main() {
    constexpr size_t N = 1 << 17;  // 128K elements → ~17 reallocations
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    uint64_t rounds = 0;
    while (std::chrono::steady_clock::now() < deadline) {
        std::vector<uint64_t> v;
        for (size_t i = 0; i < N; ++i) v.push_back(i);
        sink ^= v.back();
        ++rounds;
    }
    std::printf("rounds: %llu\n", static_cast<unsigned long long>(rounds));
}
