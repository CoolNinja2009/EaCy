/*
 * test_loops_math.c — stress-test loops, math macros, and swap
 *
 * Exercises: repeat, foreach, for_range, min, max, clamp, lerp, swap
 * Stress: nested loops, edge values, type combinations
 */

#include "../eacy.h"
#include <stdio.h>
#include <math.h>

static int failures = 0;
#define CHECK(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL %s:%d — %s\n", __FILE__, __LINE__, msg); failures++; } \
} while (0)

void test_repeat(void) {
    println("=== repeat() ===");
    int count = 0;
    repeat(100) count++;
    CHECK(count == 100, "repeat 100");

    count = 0;
    repeat(0) count++;
    CHECK(count == 0, "repeat 0");

    /* nested repeat */
    int outer = 0, inner = 0;
    repeat(10) {
        outer++;
        repeat(10) inner++;
    }
    CHECK(outer == 10, "nested repeat outer");
    CHECK(inner == 100, "nested repeat inner");

    println("  repeat: OK");
}

void test_foreach(void) {
    println("=== foreach() ===");
    int arr[] = {10, 20, 30, 40, 50};
    int sum = 0;
    foreach(i, arr) sum += arr[i];
    CHECK(sum == 150, "foreach sum");

    /* single-element array */
    int one[] = {99};
    sum = 0;
    foreach(i, one) sum += one[i];
    CHECK(sum == 99, "foreach single element");

    /* empty array not possible in C, but zero-length VLA? skip */
    /* large array */
    int big[1000];
    for_range(i, 0, 1000) big[i] = i;
    long long bigsum = 0;
    foreach(i, big) bigsum += big[i];
    CHECK(bigsum == 499500LL, "foreach big array");

    println("  foreach: OK");
}

void test_for_range(void) {
    println("=== for_range() ===");
    int sum = 0;
    for_range(i, 0, 100) sum += (int)i;
    CHECK(sum == 4950, "for_range 0..99");

    sum = 0;
    for_range(i, -50, 50) sum += (int)i;
    CHECK(sum == -50, "for_range -50..49");

    /* empty range */
    sum = 0;
    for_range(i, 5, 5) sum += (int)i;
    CHECK(sum == 0, "for_range empty");

    /* nested */
    int n = 0;
    for_range(i, 0, 5) for_range(j, 0, 5) n++;
    CHECK(n == 25, "nested for_range");

    println("  for_range: OK");
}

void test_math_macros(void) {
    println("=== Math macros ===");

    CHECK(min(3, 7) == 3, "min(3,7)");
    CHECK(min(-5, 0) == -5, "min(-5,0)");
    CHECK(min(42, 42) == 42, "min equal");
    CHECK(min(0.5f, 1.0f) == 0.5f, "min float");

    CHECK(max(3, 7) == 7, "max(3,7)");
    CHECK(max(-5, 0) == 0, "max(-5,0)");
    CHECK(max(0.1, 0.2) == 0.2, "max double");

    CHECK(clamp(5, 0, 10) == 5, "clamp inside");
    CHECK(clamp(-5, 0, 10) == 0, "clamp below");
    CHECK(clamp(15, 0, 10) == 10, "clamp above");
    CHECK(clamp(0, 0, 0) == 0, "clamp zero range");

    double t = lerp(0.0, 100.0, 0.5);
    CHECK(fabs(t - 50.0) < 0.001, "lerp 0.5");
    t = lerp(0.0, 100.0, 0.0);
    CHECK(fabs(t) < 0.001, "lerp 0.0");
    t = lerp(0.0, 100.0, 1.0);
    CHECK(fabs(t - 100.0) < 0.001, "lerp 1.0");

    println("  math: OK");
}

void test_swap(void) {
    println("=== swap() ===");
    int a = 1, b = 2;
    swap(a, b, int);
    CHECK(a == 2 && b == 1, "swap int");

    float fa = 1.5f, fb = 2.5f;
    swap(fa, fb, float);
    CHECK(fa == 2.5f && fb == 1.5f, "swap float");

    double da = 0.1, db = 0.9;
    swap(da, db, double);
    CHECK(da == 0.9 && db == 0.1, "swap double");

    char ca = 'X', cb = 'Y';
    swap(ca, cb, char);
    CHECK(ca == 'Y' && cb == 'X', "swap char");

    /* swap same variable (no-op) */
    int x = 42;
    swap(x, x, int);
    CHECK(x == 42, "swap self");

    println("  swap: OK");
}

int main(void) {
    test_repeat();
    test_foreach();
    test_for_range();
    test_math_macros();
    test_swap();

    if (failures) {
        fprintf(stderr, "\n%d FAILURES\n", failures);
        return 1;
    }
    println("\n=== test_loops_math: ALL PASSED ===");
    return 0;
}
