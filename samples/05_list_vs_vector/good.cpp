/*
 * 05_list_vs_vector — good (std::vector, sequential memory)
 *
 * std::vector stores elements contiguously. Traversal is a linear scan —
 * the hardware prefetcher predicts every next address ahead of time.
 * Expected pprof: traverse() still shows up but much narrower; rounds/sec >> bad.
 */

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <vector>

static volatile uint64_t sink = 0;

__attribute__((noinline))
static uint64_t traverse(const std::vector<uint64_t>& data) {
    uint64_t sum = 0;
    for (uint64_t v : data) sum += v;
    return sum;
}

int main() {
    constexpr size_t N = 1 << 20;
    std::vector<uint64_t> data(N);
    for (size_t i = 0; i < N; ++i) data[i] = i;

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    uint64_t rounds = 0;
    while (std::chrono::steady_clock::now() < deadline) {
        sink = traverse(data);
        ++rounds;
    }
    std::printf("rounds: %llu\n", static_cast<unsigned long long>(rounds));
}
