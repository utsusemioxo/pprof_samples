/*
 * 04_false_sharing — good (each counter on its own cache line)
 *
 * Each counter is padded to 64 bytes so threads never share a cache line.
 * fetch_add is a pure local operation with no coherence traffic.
 * Expected pprof: fetch_add nearly disappears; rounds/sec much higher.
 */

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>

static volatile uint64_t sink = 0;

struct alignas(64) PaddedCounter {
    std::atomic<uint64_t> value;
    char padding[64 - sizeof(std::atomic<uint64_t>)];
};

struct GoodCounters {
    PaddedCounter values[4];
};

int main() {
    int threads = static_cast<int>(
        std::min(4u, std::max(2u, std::thread::hardware_concurrency())));
    GoodCounters counters;
    for (int i = 0; i < threads; ++i)
        counters.values[i].value.store(0, std::memory_order_relaxed);

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    uint64_t rounds = 0;
    while (std::chrono::steady_clock::now() < deadline) {
        std::vector<std::thread> workers;
        for (int t = 0; t < threads; ++t) {
            workers.emplace_back([&, t] {
                for (uint64_t i = 0; i < 100000; ++i)
                    counters.values[t].value.fetch_add(1, std::memory_order_relaxed);
            });
        }
        for (auto& w : workers) w.join();
        ++rounds;
    }

    uint64_t sum = 0;
    for (int i = 0; i < threads; ++i)
        sum += counters.values[i].value.load(std::memory_order_relaxed);
    sink = sum;
    std::printf("rounds: %llu\n", static_cast<unsigned long long>(rounds));
}
