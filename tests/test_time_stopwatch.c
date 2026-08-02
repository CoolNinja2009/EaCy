/*
 * test_time_stopwatch.c — stress-test time, colors, stopwatch, assertions, memory
 *
 * Exercises: sleep_ms, current_time_ms, ec_init_colors, print_red/green/blue,
 *            reset_color, assert_msg, ec_malloc, ec_calloc, ec_free,
 *            ec_debug_alloc_count (if EC_DEBUG_MEMORY),
 *            timer_start, timer_restart, timer_elapsed_ms, timer_elapsed_seconds
 */

#define EC_DEBUG_MEMORY
#include "../eacy.h"
#include <stdio.h>

static int failures = 0;
#define CHECK(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL %s:%d — %s\n", __FILE__, __LINE__, msg); failures++; } \
} while (0)

void test_sleep(void) {
    println("=== sleep_ms ===");
    long long before = current_time_ms();
    sleep_ms(100);
    long long after = current_time_ms();
    long long elapsed = after - before;
    CHECK(elapsed >= 90 && elapsed <= 300, "sleep_ms ~100");
    println("  sleep: OK");
}

void test_monotonic(void) {
    println("=== current_time_ms monotonic ===");
    long long a = current_time_ms();
    long long b = current_time_ms();
    CHECK(b >= a, "monotonic increases");
    /* several calls should be ordered */
    long long prev = a;
    for_range(i, 0, 100) {
        long long now = current_time_ms();
        CHECK(now >= prev, "monotonic sequence");
        prev = now;
    }
    println("  monotonic: OK");
}

void test_colors(void) {
    println("=== Colors ===");
    ec_init_colors();
    print_red();     print("RED");
    print_green();   print("GREEN");
    print_blue();    print("BLUE");
    reset_color();   println(" NORMAL");
    /* output should appear colored */
    println("  colors: OK (verify visually)");
}

void test_assert_msg(void) {
    println("=== assert_msg (pass cases) ===");
    /* These should NOT abort */
    assert_msg(1 + 1 == 2, "basic math works");
    assert_msg(true, "true passes");
    assert_msg(42 > 0, "positive passes");
    println("  assert_msg: OK (all pass cases survived)");
}

void test_memory_debug(void) {
    println("=== Memory debugging ===");
    long before = ec_debug_alloc_count();
    CHECK(before >= 0, "alloc count valid");

    void *a = ec_malloc(100);
    CHECK(ec_debug_alloc_count() == before + 1, "ec_malloc increments");

    void *b = ec_calloc(10, 20);
    CHECK(ec_debug_alloc_count() == before + 2, "ec_calloc increments");

    ec_free(a);
    CHECK(ec_debug_alloc_count() == before + 1, "ec_free decrements");

    ec_free(b);
    CHECK(ec_debug_alloc_count() == before, "back to baseline");

    /* ec_free(NULL) safe */
    ec_free(NULL);
    CHECK(ec_debug_alloc_count() == before, "free NULL no-op");

    println("  memory debug: OK");
}

void test_stopwatch(void) {
    println("=== Stopwatch ===");
    ec_timer t = timer_start();
    sleep_ms(50);
    long long ms = timer_elapsed_ms(&t);
    CHECK(ms >= 40 && ms <= 200, "elapsed_ms ~50");

    double sec = timer_elapsed_seconds(&t);
    CHECK(sec >= 0.04 && sec <= 0.2, "elapsed_seconds ~0.05");
    CHECK(sec * 1000.0 >= (double)(ms - 1), "ms vs seconds consistent");

    timer_restart(&t);
    CHECK(timer_elapsed_ms(&t) < 5, "elapsed ~0 after restart");

    /* multiple restarts */
    for_range(i, 0, 5) {
        timer_restart(&t);
        /* tiny delay */
        volatile int x = 0;
        for_range(j, 0, 1000) x += j;
        CHECK(timer_elapsed_ms(&t) >= 0, "restart cycle");
    }

    println("  stopwatch: OK");
}

int main(void) {
    test_sleep();
    test_monotonic();
    test_colors();
    test_assert_msg();
    test_memory_debug();
    test_stopwatch();

    if (failures) {
        fprintf(stderr, "\n%d FAILURES\n", failures);
        return 1;
    }
    println("\n=== test_time_stopwatch: ALL PASSED ===");
    return 0;
}
