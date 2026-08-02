/*
 * test_logging_benchmark.c — stress-test logging and benchmark macros
 *
 * Exercises: log_info, log_warn, log_error, log_debug,
 *            benchmark, benchmark_avg
 */

#include "../eacy.h"
#include <stdio.h>

static int failures = 0;
#define CHECK(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL %s:%d — %s\n", __FILE__, __LINE__, msg); failures++; } \
} while (0)

void test_logging(void) {
    println("=== Logging ===");
    ec_init_colors();

    log_info("Test info:", 1);
    log_warn("Test warning:", 2);
    log_error("Test error:", 3);
    log_debug("Test debug:", 4);

    /* all 8 args max */
    log_info("one", "two", 3, 4L, 5.0f, 6.0, '7', true);

    /* single arg */
    log_info("standalone message");

    /* empty-looking (compiles, prints just timestamp + label) */
    /* Note: at least one arg required */

    println("  logging: OK (verify stderr output above)");
}

/* A dummy workload for benchmarking */
static int do_work(int n) {
    volatile int sum = 0;
    for_range(i, 0, n) sum += (int)i;
    return sum;
}

void test_benchmark(void) {
    println("=== Benchmark ===");

    benchmark("dummy workload 10k") {
        do_work(10000);
    }

    benchmark("dummy workload 100k") {
        do_work(100000);
    }

    println("  benchmark: OK (verify timing output above)");
}

void test_benchmark_avg(void) {
    println("=== benchmark_avg ===");

    benchmark_avg("dummy avg 5k x 20", 20) {
        do_work(5000);
    }

    benchmark_avg("dummy avg 1k x 100", 100) {
        do_work(1000);
    }

    /* single iteration */
    benchmark_avg("single iteration avg", 1) {
        do_work(100);
    }

    println("  benchmark_avg: OK (verify timing output above)");
}

int main(void) {
    test_logging();
    test_benchmark();
    test_benchmark_avg();

    if (failures) {
        fprintf(stderr, "\n%d FAILURES\n", failures);
        return 1;
    }
    println("\n=== test_logging_benchmark: ALL PASSED ===");
    return 0;
}
