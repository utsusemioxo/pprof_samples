#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>

static inline uint64_t rdtsc() {
#if defined(__aarch64__)
    uint64_t val;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(val));
    return val;
#elif defined(__x86_64__)
    uint32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return (static_cast<uint64_t>(hi) << 32) | lo;
#else
#error "unsupported arch"
#endif
}

#define RDTSC_START() ({ __asm__ volatile("" ::: "memory"); rdtsc(); })
#define RDTSC_END() ({ uint64_t t = rdtsc(); __asm__ volatile("" ::: "memory"); t; })

static double GHZ = 0.0;

static inline double calibrate_ghz() {
    auto t0 = rdtsc();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    auto t1 = rdtsc();
    return static_cast<double>(t1 - t0) / 2e8;
}

static inline void init_rdtsc() {
    GHZ = calibrate_ghz();
    std::printf("CPU freq: %.2f GHz\n\n", GHZ);
}

static inline double cycles_to_ns(uint64_t cycles) {
    return GHZ > 0.0 ? static_cast<double>(cycles) / GHZ : 0.0;
}

template <typename F>
static inline uint64_t measure_median(F&& fn, int runs = 11) {
    std::vector<uint64_t> samples;
    samples.reserve(static_cast<size_t>(runs));
    for (int i = 0; i < runs; ++i) {
        auto start = RDTSC_START();
        fn();
        auto end = RDTSC_END();
        samples.push_back(end - start);
    }
    std::sort(samples.begin(), samples.end());
    return samples[samples.size() / 2];
}

static inline void print_result(const char* label, uint64_t cycles) {
    std::printf("%-34s %12llu cycles  %12.1f ns\n",
                label,
                static_cast<unsigned long long>(cycles),
                cycles_to_ns(cycles));
}

static inline void print_separator(const char* title) {
    std::printf("\n== %s ==\n", title);
}
