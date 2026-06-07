/*
 * 06_vector_reserve — good (reserve upfront, zero reallocation)
 *
 * reserve(N) allocates the full capacity once. push_back never reallocates.
 * Scudo is called exactly once per outer loop iteration.
 * Expected pprof: Scudo overhead gone; rounds/sec much higher.
 */

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <vector>

static volatile uint64_t sink = 0;

int main() {
    constexpr size_t N = 1 << 17;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    uint64_t rounds = 0;
    while (std::chrono::steady_clock::now() < deadline) {
        std::vector<uint64_t> v;
        v.reserve(N);
        for (size_t i = 0; i < N; ++i) v.push_back(i);
        sink ^= v.back();
        ++rounds;
    }
    std::printf("rounds: %llu\n", static_cast<unsigned long long>(rounds));
}
