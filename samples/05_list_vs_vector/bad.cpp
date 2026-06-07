/*
 * 05_list_vs_vector — bad (std::list, pointer chasing)
 *
 * std::list allocates each node separately. Traversal follows pointers
 * scattered across the heap — every step is likely a cache miss.
 * Expected pprof: traverse() dominates; memory stalls visible in cycle count.
 */

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <list>

static volatile uint64_t sink = 0;

__attribute__((noinline))
static uint64_t traverse(const std::list<uint64_t>& data) {
    uint64_t sum = 0;
    for (uint64_t v : data) sum += v;
    return sum;
}

int main() {
    constexpr size_t N = 1 << 20;  // 1M elements, well beyond L3 cache
    std::list<uint64_t> data;
    for (size_t i = 0; i < N; ++i) data.push_back(i);

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    uint64_t rounds = 0;
    while (std::chrono::steady_clock::now() < deadline) {
        sink = traverse(data);
        ++rounds;
    }
    std::printf("rounds: %llu\n", static_cast<unsigned long long>(rounds));
}
