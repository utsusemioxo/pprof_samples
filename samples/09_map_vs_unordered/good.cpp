/*
 * 09_map_vs_unordered — good (std::unordered_map, hash table)
 *
 * std::unordered_map finds keys in O(1) via hash lookup. Cache behavior is
 * better than a tree for large tables with random access.
 * Expected pprof: hash machinery replaces tree traversal; rounds/sec higher.
 */

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <unordered_map>
#include <vector>

static volatile uint64_t sink = 0;

int main() {
    constexpr size_t N = 1 << 17;
    std::unordered_map<uint64_t, uint64_t> m;
    m.reserve(N);
    for (size_t i = 0; i < N; ++i) m[i] = i * 2;

    std::vector<uint64_t> keys(N);
    uint64_t x = 0xdeadbeef;
    for (auto& k : keys) {
        x = x * 6364136223846793005ULL + 1442695040888963407ULL;
        k = x % N;
    }

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    uint64_t sum = 0;
    uint64_t rounds = 0;
    while (std::chrono::steady_clock::now() < deadline) {
        for (uint64_t k : keys) sum += m.find(k)->second;
        ++rounds;
    }
    sink = sum;
    std::printf("rounds: %llu\n", static_cast<unsigned long long>(rounds));
}
