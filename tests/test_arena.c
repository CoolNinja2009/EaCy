/*
 * test_arena.c — stress-test arena (bump) allocator
 *
 * Exercises: ec_arena_new, ec_arena_alloc, ec_arena_alloc_zero,
 *            ec_arena_reset, ec_arena_free, ec_arena_remaining
 * Stress: many small allocs, exact exhaustion, reset cycles
 */

#include "../eacy.h"
#include <stdio.h>
#include <string.h>

static int failures = 0;
#define CHECK(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL %s:%d — %s\n", __FILE__, __LINE__, msg); failures++; } \
} while (0)

void test_basic(void) {
    println("=== Arena — basic ===");
    ec_arena a = ec_arena_new(1024);
    CHECK(a.buf != NULL, "allocation ok");
    CHECK(a.cap == 1024, "capacity");
    CHECK(a.offset == 0, "offset 0");
    CHECK(ec_arena_remaining(&a) == 1024, "remaining 1024");

    int *nums = ec_arena_alloc(&a, 100 * sizeof(int));
    CHECK(nums != NULL, "alloc nums");
    for_range(i, 0, 100) nums[i] = (int)i;
    CHECK(ec_arena_remaining(&a) < 1024, "remaining decreased");

    char *str = ec_arena_alloc(&a, 200);
    CHECK(str != NULL, "alloc str");
    strcpy(str, "hello arena");

    double *vals = ec_arena_alloc_zero(&a, 50 * sizeof(double));
    CHECK(vals != NULL, "alloc_zero vals");
    for_range(i, 0, 50) CHECK(vals[i] == 0.0, "zero-filled");

    /* verify our data is still intact after more allocs */
    CHECK(nums[0] == 0 && nums[99] == 99, "nums intact");
    CHECK(equals(str, "hello arena"), "str intact");

    ec_arena_free(&a);
    CHECK(a.buf == NULL, "freed buf NULL");
    CHECK(a.cap == 0, "freed cap 0");
    CHECK(a.offset == 0, "freed offset 0");

    /* double-free safe */
    ec_arena_free(&a);

    println("  basic: OK");
}

void test_exhaustion(void) {
    println("=== Arena — exhaustion ===");
    ec_arena a = ec_arena_new(256);
    CHECK(a.buf != NULL, "alloc ok");

    /* allocate exactly to fill */
    void *p1 = ec_arena_alloc(&a, 100);
    CHECK(p1 != NULL, "p1 ok");
    void *p2 = ec_arena_alloc(&a, 100);
    CHECK(p2 != NULL, "p2 ok");

    /* remaining should be small */
    size_t rem = ec_arena_remaining(&a);
    CHECK(rem < 100, "nearly exhausted");

    /* alloc more than remaining → NULL */
    void *p3 = ec_arena_alloc(&a, 200);
    CHECK(p3 == NULL, "exhausted returns NULL");

    ec_arena_free(&a);
    println("  exhaustion: OK");
}

void test_reset_cycle(void) {
    println("=== Arena — reset cycle ===");
    ec_arena a = ec_arena_new(1024 * 1024);  /* 1 MiB */
    CHECK(a.buf != NULL, "alloc ok");

    for (int cycle = 0; cycle < 10; cycle++) {
        int *data = ec_arena_alloc(&a, 10000 * sizeof(int));
        CHECK(data != NULL, "cycle alloc");
        for_range(i, 0, 10000) data[i] = cycle * 10000 + (int)i;

        /* use data... */
        CHECK(data[0] == cycle * 10000, "data intact");

        ec_arena_reset(&a);
        CHECK(a.offset == 0, "reset offset 0");
        CHECK(ec_arena_remaining(&a) == 1024 * 1024, "reset full remaining");
    }

    ec_arena_free(&a);
    println("  reset cycle: OK");
}

void test_zero_capacity(void) {
    println("=== Arena — zero capacity ===");
    ec_arena a = ec_arena_new(0);
    /* may or may not allocate — but must not crash */
    (void)ec_arena_alloc(&a, 16);
    /* p may be NULL, that's fine */
    CHECK(ec_arena_remaining(&a) == 0, "zero cap remaining");
    ec_arena_free(&a);
    println("  zero capacity: OK");
}

int main(void) {
    test_basic();
    test_exhaustion();
    test_reset_cycle();
    test_zero_capacity();

    if (failures) {
        fprintf(stderr, "\n%d FAILURES\n", failures);
        return 1;
    }
    println("\n=== test_arena: ALL PASSED ===");
    return 0;
}
