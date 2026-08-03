/*
 * test_eacyp.c - dynamic EaCyP helpers
 */

#include "../EaCyP.h"
#include <stdio.h>

static int failures = 0;
#define CHECK(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL %s:%d - %s\n", __FILE__, __LINE__, msg); failures++; } \
} while (0)

module(math);

export int module_add(int a, int b) {
    return a + b;
}

int main(void) {
    println("=== EaCyP dynamic layer ===");

    var a, b, c;
    a = b = c = V(12);

    CHECK(a.type == ECP_INT, "chained assignment stores int");
    CHECK(integer(a) == 12 && integer(b) == 12 && integer(c) == 12, "a = b = c = V(12)");

    let(name, "EaCyP");
    let(pi, 3.5);
    set(a, 99);
    set_all(V(true), &b, &c);

    CHECK(equals(type_of(name), "string"), "string type");
    CHECK(equals(type_of(pi), "double"), "double type");
    CHECK(integer(a) == 99, "set macro");
    CHECK(truthy(b) && truthy(c), "set_all truthy values");
    CHECK((int)num(add(V(2), V(3))) == 5, "dynamic add");
    CHECK(equals(module_name(math), "math"), "module name helper");
    CHECK(module_add(10, 2) == 12, "exported module function");

    say("name:", name, "a:", a, "pi:", pi);

    if (failures) {
        fprintf(stderr, "\n%d FAILURES\n", failures);
        return 1;
    }

    println("=== test_eacyp: ALL PASSED ===");
    return 0;
}
