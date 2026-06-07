/*
 * 04_false_sharing — bad (counters share one cache line)
 *
 * All thread counters sit on the same 64-byte cache line. Every fetch_add by
 * one thread invalidates the line for all others, causing coherence traffic.
 * Expected pprof: high cycle count in fetch_add / cache-coherence stalls.
 */

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>

static volatile uint64_t sink = 0;

struct BadCounters {
    std::atomic<uint64_t> values[4];
};

int main() {
    int threads = static_cast<int>(
        std::min(4u, std::max(2u, std::thread::hardware_concurrency())));
    BadCounters counters;
    for (int i = 0; i < threads; ++i)
        counters.values[i].store(0, std::memory_order_relaxed);

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    uint64_t rounds = 0;
    while (std::chrono::steady_clock::now() < deadline) {
        std::vector<std::thread> workers;
        for (int t = 0; t < threads; ++t) {
            workers.emplace_back([&, t] {
                for (uint64_t i = 0; i < 100000; ++i)
                    counters.values[t].fetch_add(1, std::memory_order_relaxed);
            });
        }
        for (auto& w : workers) w.join();
        ++rounds;
    }

    uint64_t sum = 0;
    for (int i = 0; i < threads; ++i)
        sum += counters.values[i].load(std::memory_order_relaxed);
    sink = sum;
    std::printf("rounds: %llu\n", static_cast<unsigned long long>(rounds));
}
