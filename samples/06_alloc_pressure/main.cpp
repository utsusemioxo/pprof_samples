/*
 * 06_alloc_pressure
 *
 * Concept:
 *   Allocation pressure can dominate profiles. A tiny free-list pool removes
 *   most malloc/free overhead for fixed-size objects.
 *
 * Predict before running:
 *   bad_malloc_free() repeatedly calls the allocator. good_object_pool() mostly
 *   pops and pushes pointers from a local free list.
 *
 * Run:
 *   ./build/bin/sample_06_alloc_pressure
 */

#include "rdtsc.h"

#include <cstdint>
#include <cstdio>
#include <vector>

static void escape(void* p) { __asm__ volatile("" : : "r"(p) : "memory"); }
static volatile uint64_t sink = 0;

struct Node {
    Node* next;
    uint64_t value[7];
};

__attribute__((noinline)) static uint64_t bad_malloc_free(size_t n) {
    uint64_t sum = 0;
    for (size_t i = 0; i < n; ++i) {
        Node* node = new Node{};
        escape(node);
        node->value[0] = i;
        sum += node->value[0];
        delete node;
    }
    sink ^= sum;
    return sum;
}

__attribute__((noinline)) static uint64_t good_object_pool(size_t n) {
    std::vector<Node> storage(n);
    Node* free_list = nullptr;
    for (size_t i = 0; i < n; ++i) {
        storage[i].next = free_list;
        free_list = &storage[i];
    }
    escape(storage.data());

    uint64_t sum = 0;
    for (size_t i = 0; i < n; ++i) {
        Node* node = free_list;
        free_list = free_list->next;
        node->value[0] = i;
        sum += node->value[0];
        node->next = free_list;
        free_list = node;
    }
    sink ^= sum;
    return sum;
}

int main() {
    init_rdtsc();
    print_separator("allocation pressure");

    const size_t n = 1000000;
    auto malloc_cycles = measure_median([&] { bad_malloc_free(n); }, 5);
    auto pool_cycles = measure_median([&] { good_object_pool(n); }, 5);

    print_result("bad malloc/free total", malloc_cycles);
    print_result("good object pool total", pool_cycles);
    std::printf("bad per op:  %.1f ns\n", cycles_to_ns(malloc_cycles) / n);
    std::printf("good per op: %.1f ns\n", cycles_to_ns(pool_cycles) / n);

    std::printf("\npprof expectation:\n");
    std::printf("  malloc/free frames can become hot even when application logic is tiny.\n");
    std::printf("sink=%llu\n", static_cast<unsigned long long>(sink));
    return 0;
}
