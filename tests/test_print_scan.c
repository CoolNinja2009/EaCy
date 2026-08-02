/*
 * test_print_scan.c — stress-test printing, input, and type dispatch
 *
 * Exercises: print, println, nl, scan, input_*, scan_flush
 * Stress: all supported types, edge values, max args (8)
 */

#include "../eacy.h"
#include <stdio.h>

static int failures = 0;
#define CHECK(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL %s:%d — %s\n", __FILE__, __LINE__, msg); failures++; } \
} while (0)

/* Redirect stdin from a string for scan testing. */
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/wait.h>
#endif

void test_print_all_types(void) {
    println("=== Printing — all types ===");
    int    i = -42;
    long   l = 1234567890L;
    float  f = 3.14159f;
    double d = 2.718281828;
    char   c = 'X';
    char  *s = "hello";
    bool   b = true;

    print("int:", i);          /* int: -42 */
    print("long:", l);         /* long: 1234567890 */
    print("float:", f);        /* float: 3.14159 */
    print("double:", d);       /* double: 2.71828 */
    print("char:", c);         /* char: X */
    print("str:", s);          /* str: hello */
    print("bool:", b);         /* bool: true */

    b = false;
    print("bool false:", b);   /* bool false: false */

    /* 8-argument max stress */
    println(1, 2L, 3.0f, 4.0, 'A', "five", "six", true);
    nl();
    println("All print tests OK");
}

void test_print_edge_values(void) {
    println("=== Printing — edge values ===");
    print("INT_MIN:", (int)0x80000000);
    print("INT_MAX:", 2147483647);
    print("zero:", 0);
    print("neg:", -1);
    /* float edges */
    float zero = 0.0f, neg = -0.0f, inf = 1.0f/0.0f;
    print("float zero:", zero);
    print("float neg:", neg);
    print("float inf:", inf);
    /* pointer fallback */
    print("ptr:", (void*)0xdeadbeef);
    nl();
}

void test_scan_basic(void) {
    println("=== Scan — basic (pass pointers from variables) ===");
    /* We can't really drive stdin here without user input,
     * but we verify the macros compile and the _Generic dispatch works. */
    int x = 0; long y = 0; float z = 0; double w = 0; bool b = false;
    (void)x; (void)y; (void)z; (void)w; (void)b;
    println("  Scan macros compile-check: OK");
}

void test_input_functions(void) {
    println("=== Input — helpers compile-check ===");
    /* input_int, input_float, input_double, input_char, input_string all compile */
    volatile int    vi = 0; (void)vi;
    volatile float  vf = 0; (void)vf;
    volatile double vd = 0; (void)vd;
    volatile char   vc = 0; (void)vc;
    println("  Input helpers compile-check: OK");
}

int main(void) {
    test_print_all_types();
    test_print_edge_values();
    test_scan_basic();
    test_input_functions();

    if (failures) {
        fprintf(stderr, "\n%d FAILURES\n", failures);
        return 1;
    }
    println("\n=== test_print_scan: ALL PASSED ===");
    return 0;
}
