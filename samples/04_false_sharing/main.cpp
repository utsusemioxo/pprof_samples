/*
 * 04_false_sharing
 *
 * Concept:
 *   Logically independent atomics can still fight if they share one cache line.
 *   The cache-coherence traffic appears as expensive on-CPU time.
 *
 * Predict before running:
 *   bad_shared_line() is slower because different threads invalidate the same
 *   cache line. good_padded_line() separates counters onto different lines.
 *
 * Run:
 *   ./build/bin/sample_04_false_sharing
 */

#include "rdtsc.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>

static volatile uint64_t sink = 0;

struct BadCounters {
    std::atomic<uint64_t> values[4];
};

struct alignas(64) PaddedCounter {
    std::atomic<uint64_t> value;
    char padding[64 - sizeof(std::atomic<uint64_t>)];
};

struct GoodCounters {
    PaddedCounter values[4];
};

__attribute__((noinline)) static uint64_t bad_shared_line(int threads, uint64_t iters) {
    BadCounters counters;
    for (int i = 0; i < threads; ++i) {
        counters.values[i].store(0, std::memory_order_relaxed);
    }

    std::vector<std::thread> workers;
    for (int t = 0; t < threads; ++t) {
        workers.emplace_back([&, t] {
            for (uint64_t i = 0; i < iters; ++i) {
                counters.values[t].fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    for (auto& worker : workers) {
        worker.join();
    }

    uint64_t sum = 0;
    for (int i = 0; i < threads; ++i) {
        sum += counters.values[i].load(std::memory_order_relaxed);
    }
    sink ^= sum;
    return sum;
}

__attribute__((noinline)) static uint64_t good_padded_line(int threads, uint64_t iters) {
    GoodCounters counters;
    for (int i = 0; i < threads; ++i) {
        counters.values[i].value.store(0, std::memory_order_relaxed);
    }

    std::vector<std::thread> workers;
    for (int t = 0; t < threads; ++t) {
        workers.emplace_back([&, t] {
            for (uint64_t i = 0; i < iters; ++i) {
                counters.values[t].value.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    for (auto& worker : workers) {
        worker.join();
    }

    uint64_t sum = 0;
    for (int i = 0; i < threads; ++i) {
        sum += counters.values[i].value.load(std::memory_order_relaxed);
    }
    sink ^= sum;
    return sum;
}

int main() {
    init_rdtsc();
    print_separator("false sharing");

    int threads = std::min(4u, std::max(2u, std::thread::hardware_concurrency()));
    uint64_t iters = 1500000;

    auto bad_cycles = measure_median([&] { bad_shared_line(threads, iters); }, 5);
    auto good_cycles = measure_median([&] { good_padded_line(threads, iters); }, 5);

    print_result("bad same cache line", bad_cycles);
    print_result("good padded counters", good_cycles);
    std::printf("speedup: %.2fx\n", static_cast<double>(bad_cycles) / good_cycles);

    std::printf("\npprof expectation:\n");
    std::printf("  Both versions do the same logical work, but the bad one burns CPU in coherence traffic.\n");
    std::printf("sink=%llu threads=%d\n", static_cast<unsigned long long>(sink), threads);
    return 0;
}
