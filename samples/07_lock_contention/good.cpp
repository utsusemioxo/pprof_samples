/*
 * 07_lock_contention — good (work outside critical section)
 *
 * Each thread does heavy work locally, then acquires the mutex only to
 * accumulate the result. Threads run in parallel for most of their lifetime.
 * Expected pprof: do_work dominates; lock overhead nearly disappears.
 */

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <thread>
#include <vector>

static volatile uint64_t sink = 0;

__attribute__((noinline)) static void do_work(uint64_t spins) {
    uint64_t x = 1;
    for (uint64_t i = 0; i < spins; ++i)
        x = x * 2862933555777941757ULL + 3037000493ULL;
    sink ^= x;
}

int main() {
    int threads = static_cast<int>(
        std::min(4u, std::max(2u, std::thread::hardware_concurrency())));
    constexpr int rounds = 200;

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    uint64_t iters = 0;
    while (std::chrono::steady_clock::now() < deadline) {
        std::mutex mu;
        uint64_t shared = 0;
        std::vector<std::thread> workers;
        for (int t = 0; t < threads; ++t) {
            workers.emplace_back([&, t] {
                uint64_t local = 0;
                for (int i = 0; i < rounds; ++i) {
                    do_work(10000);
                    local += static_cast<uint64_t>(t + i);
                }
                std::lock_guard<std::mutex> lock(mu);
                shared += local;
            });
        }
        for (auto& w : workers) w.join();
        sink ^= shared;
        ++iters;
    }
    std::printf("iters: %llu\n", static_cast<unsigned long long>(iters));
}
