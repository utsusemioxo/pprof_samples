/*
 * 07_lock_contention
 *
 * Concept:
 *   Default pprof-style CPU profiling samples only running threads. A program
 *   blocked on a mutex can look almost empty even though wall time is bad.
 *
 * Predict before running:
 *   bad_lock_contention() has long wall time because worker threads wait for a
 *   mutex. CPU samples mostly land in the lock holder or scheduler/runtime code.
 *   simpleperf --trace-offcpu can reveal the waiting stacks.
 *
 * Run:
 *   ./build/bin/sample_07_lock_contention
 */

#include "rdtsc.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <thread>
#include <vector>

static volatile uint64_t sink = 0;

__attribute__((noinline)) static void hold_lock_work(uint64_t spins) {
    uint64_t x = 1;
    for (uint64_t i = 0; i < spins; ++i) {
        x = x * 2862933555777941757ull + 3037000493ull;
    }
    sink ^= x;
}

__attribute__((noinline)) static uint64_t bad_lock_contention(int threads, int rounds) {
    std::mutex mu;
    uint64_t shared = 0;
    std::vector<std::thread> workers;

    for (int t = 0; t < threads; ++t) {
        workers.emplace_back([&, t] {
            for (int i = 0; i < rounds; ++i) {
                std::lock_guard<std::mutex> lock(mu);
                hold_lock_work(45000);
                shared += static_cast<uint64_t>(t + i);
            }
        });
    }
    for (auto& worker : workers) {
        worker.join();
    }
    sink ^= shared;
    return shared;
}

__attribute__((noinline)) static uint64_t good_less_contention(int threads, int rounds) {
    std::mutex mu;
    uint64_t shared = 0;
    std::vector<std::thread> workers;

    for (int t = 0; t < threads; ++t) {
        workers.emplace_back([&, t] {
            uint64_t local = 0;
            for (int i = 0; i < rounds; ++i) {
                hold_lock_work(45000);
                local += static_cast<uint64_t>(t + i);
            }
            std::lock_guard<std::mutex> lock(mu);
            shared += local;
        });
    }
    for (auto& worker : workers) {
        worker.join();
    }
    sink ^= shared;
    return shared;
}

int main() {
    init_rdtsc();
    print_separator("lock contention");

    int threads = std::min(4u, std::max(2u, std::thread::hardware_concurrency()));
    int rounds = 80;

    auto bad_start = std::chrono::steady_clock::now();
    auto bad_cycles = measure_median([&] { bad_lock_contention(threads, rounds); }, 3);
    auto bad_end = std::chrono::steady_clock::now();

    auto good_start = std::chrono::steady_clock::now();
    auto good_cycles = measure_median([&] { good_less_contention(threads, rounds); }, 3);
    auto good_end = std::chrono::steady_clock::now();

    print_result("bad lock contention", bad_cycles);
    print_result("good shorter critical section", good_cycles);

    auto bad_ms = std::chrono::duration<double, std::milli>(bad_end - bad_start).count();
    auto good_ms = std::chrono::duration<double, std::milli>(good_end - good_start).count();
    std::printf("wall time including repeats: bad %.1f ms, good %.1f ms\n", bad_ms, good_ms);

    std::printf("\npprof expectation:\n");
    std::printf("  On-CPU profiling under-represents threads sleeping on the mutex.\n");
    std::printf("  Use an off-CPU profiler such as simpleperf --trace-offcpu to see wait stacks.\n");
    std::printf("sink=%llu threads=%d\n", static_cast<unsigned long long>(sink), threads);
    return 0;
}
