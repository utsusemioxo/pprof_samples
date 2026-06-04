/*
 * 03_complexity
 *
 * Concept:
 *   pprof shows where time went, but it does not know input size N. Timing
 *   ratios across several N values reveal algorithmic complexity.
 *
 * Predict before running:
 *   Doubling N makes bad_o_n2() about 4x slower, while good_o_nlogn() grows by
 *   roughly 2x plus a small log factor.
 *
 * Run:
 *   ./build/bin/sample_03_complexity
 */

#include "rdtsc.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <vector>

static void escape(void* p) { __asm__ volatile("" : : "r"(p) : "memory"); }
static volatile uint64_t sink = 0;

__attribute__((noinline)) static uint64_t bad_o_n2(const std::vector<int>& data) {
    uint64_t count = 0;
    for (size_t i = 0; i < data.size(); ++i) {
        for (size_t j = 0; j < data.size(); ++j) {
            count += data[i] < data[j];
        }
    }
    sink ^= count;
    return count;
}

__attribute__((noinline)) static uint64_t good_o_nlogn(const std::vector<int>& data) {
    std::vector<int> tmp(data.size());
    uint64_t checksum = 0;
    for (int repeat = 0; repeat < 16; ++repeat) {
        std::copy(data.begin(), data.end(), tmp.begin());
        escape(tmp.data());
        std::sort(tmp.begin(), tmp.end());
        for (int v : tmp) {
            checksum += static_cast<uint64_t>(v);
        }
    }
    sink ^= checksum;
    return checksum;
}

static std::vector<int> make_data(size_t n) {
    std::vector<int> data(n);
    uint32_t x = 12345;
    for (size_t i = 0; i < n; ++i) {
        x = x * 1103515245u + 12345u;
        data[i] = static_cast<int>(x);
    }
    return data;
}

int main() {
    init_rdtsc();
    print_separator("complexity ratios");

    const size_t sizes[] = {512, 1024, 2048, 4096};
    uint64_t prev_bad = 0;
    uint64_t prev_good = 0;

    std::printf("%8s %16s %10s %16s %10s\n", "N", "bad O(n^2)", "ratio", "good O(nlogn)", "ratio");
    for (size_t n : sizes) {
        auto data = make_data(n);
        auto bad_cycles = measure_median([&] { bad_o_n2(data); }, 5);
        auto good_cycles = measure_median([&] { good_o_nlogn(data); }, 5);
        double bad_ratio = prev_bad ? static_cast<double>(bad_cycles) / prev_bad : 0.0;
        double good_ratio = prev_good ? static_cast<double>(good_cycles) / prev_good : 0.0;

        std::printf("%8zu %16llu %10.2f %16llu %10.2f\n",
                    n,
                    static_cast<unsigned long long>(bad_cycles),
                    bad_ratio,
                    static_cast<unsigned long long>(good_cycles),
                    good_ratio);
        prev_bad = bad_cycles;
        prev_good = good_cycles;
    }

    std::printf("\npprof expectation:\n");
    std::printf("  A single profile identifies hot code, but these ratios explain why it scales badly.\n");
    std::printf("sink=%llu\n", static_cast<unsigned long long>(sink));
    return 0;
}
