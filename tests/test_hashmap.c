/*
 * test_hashmap.c — stress-test generic hash map
 *
 * Exercises: hm(int,int), hm(char*,float), hm(struct,struct),
 *            ec_hm_* backward compat, all hm_* macros
 * Stress: 50k entries, collisions, tombstones, resize chains
 */

#include "../eacy.h"
#include <stdio.h>
#include <string.h>

static int failures = 0;
#define CHECK(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL %s:%d — %s\n", __FILE__, __LINE__, msg); failures++; } \
} while (0)

void test_int_int(void) {
    println("=== hm(int,int) ===");
    hm(int, int) m;
    hm_init(m);
    CHECK(hm_empty(m), "empty");
    CHECK(hm_size(m) == 0, "size 0");

    /* insert 1000 entries */
    for_range(i, 0, 1000) hm_set(m, (int)i, (int)(i * 10));
    CHECK(hm_size(m) == 1000, "size 1000");
    CHECK(!hm_empty(m), "not empty");

    /* lookup all */
    for_range(i, 0, 1000) {
        int v; int k = (int)i;
        CHECK(hm_get(m, k, &v) && v == (int)(i * 10), "hm_get");
    }

    /* contains */
    { int k = 500; CHECK(hm_contains(m, k), "contains 500"); }
    { int k = 9999; CHECK(!hm_contains(m, k), "!contains 9999"); }

    /* update */
    { int k = 500; hm_set(m, k, 99999); }
    int v; { int k = 500;
    CHECK(hm_get(m, k, &v) && v == 99999, "update"); }

    /* remove some */
    for_range(i, 0, 500) { int k = (int)i; hm_remove(m, k); }
    CHECK(hm_size(m) == 500, "size after remove 500");

    /* removed keys not found */
    { int k = 0; CHECK(!hm_contains(m, k), "removed key gone"); }
    { int k = 499; CHECK(!hm_contains(m, k), "removed key gone"); }

    /* remaining keys still found */
    { int k = 500; CHECK(hm_contains(m, k), "remaining key found"); }
    { int k = 999; CHECK(hm_contains(m, k), "remaining key found"); }

    /* re-insert removed key */
    { int k = 0; hm_set(m, k, 42); }
    { int k = 0; CHECK(hm_get(m, k, &v) && v == 42, "re-insert"); }

    /* clear */
    hm_clear(m);
    CHECK(hm_empty(m), "empty after clear");
    CHECK(hm_size(m) == 0, "size after clear");

    /* reuse after clear */
    { int k = 1; hm_set(m, k, 100); }
    { int k = 1; CHECK(hm_get(m, k, &v) && v == 100, "reuse after clear"); }
}

void test_string_float(void) {
    println("=== hm(char*, float) ===");
    hm(char*, float) m;
    hm_init(m);

    const char *keys[] = {"alpha", "beta", "gamma", "delta", "epsilon",
                          "zeta", "eta", "theta", "iota", "kappa"};
    for_range(i, 0, 10) {
        hm_set(m, keys[i], (float)i * 1.5f);
    }
    CHECK(hm_size(m) == 10, "size 10");

    float price;
    CHECK(hm_get(m, keys[3], &price) && price == 4.5f, "string key get");
    CHECK(hm_get(m, keys[9], &price) && price == 13.5f, "string key get");

    /* remove and re-check */
    hm_remove(m, keys[0]);
    CHECK(!hm_contains(m, keys[0]), "removed string key");
    CHECK(hm_contains(m, keys[1]), "other keys intact");

    hm_free(m);
    println("  hm(char*, float): OK");
}

void test_struct_keys(void) {
    println("=== hm(struct, struct) ===");
    typedef struct { int id; double score; } Record;
    typedef struct { char label[32]; } Tag;

    hm(Record, Tag) m;
    hm_init(m);

    Record r1 = {1, 9.5}, r2 = {2, 7.3}, r3 = {3, 8.1};
    Tag t1 = {"first"}, t2 = {"second"}, t3 = {"third"};

    hm_set(m, r1, t1);
    hm_set(m, r2, t2);
    hm_set(m, r3, t3);
    CHECK(hm_size(m) == 3, "struct key size");

    Tag out;
    CHECK(hm_get(m, r2, &out) && strcmp(out.label, "second") == 0, "struct key get");
    CHECK(!hm_contains(m, ((Record){99, 0.0})), "struct key not found");

    hm_free(m);
    println("  hm(struct, struct): OK");
}

void test_backward_compat(void) {
    println("=== ec_hm_* backward compat ===");
    ec_hashmap m = ec_hm_new();
    CHECK(m.key_size == sizeof(char*), "key_size set");

    ec_hm_set(&m, "hello", "world");
    ec_hm_set(&m, "foo", "bar");
    ec_hm_set(&m, "number", "42");

    CHECK(ec_hm_len(&m) == 3, "len");
    CHECK(equals(ec_hm_get(&m, "hello"), "world"), "get");
    CHECK(ec_hm_has(&m, "foo"), "has");
    CHECK(!ec_hm_has(&m, "nope"), "!has");

    /* update */
    ec_hm_set(&m, "number", "99");
    CHECK(equals(ec_hm_get(&m, "number"), "99"), "update");

    /* delete */
    CHECK(ec_hm_del(&m, "foo"), "del true");
    CHECK(!ec_hm_del(&m, "foo"), "del false (gone)");
    CHECK(ec_hm_len(&m) == 2, "len after del");

    ec_hm_free(&m);
    /* double-free safe */
    ec_hm_free(&m);

    println("  backward compat: OK");
}

void test_stress_50k(void) {
    println("=== Stress: 50,000 entries ===");
    hm(int, int) m;
    hm_init(m);

    /* insert 50k sequential */
    for_range(i, 0, 50000) hm_set(m, (int)i, (int)(i ^ 0x5555));
    CHECK(hm_size(m) == 50000, "50k size");

    /* spot-check */
    int v;
    { int k = 0; CHECK(hm_get(m, k, &v) && v == 0x5555, "spot 0"); }
    { int k = 25000; CHECK(hm_get(m, k, &v) && v == (25000 ^ 0x5555), "spot 25000"); }
    { int k = 49999; CHECK(hm_get(m, k, &v) && v == (49999 ^ 0x5555), "spot 49999"); }

    /* remove all odd keys */
    for_range(i, 1, 50000) if (i % 2 == 1) { int k = (int)i; hm_remove(m, k); }
    CHECK(hm_size(m) == 25000, "size after remove odds");

    /* evens still there */
    for_range(i, 0, 50000) if (i % 2 == 0) {
        int k = (int)i;
        CHECK(hm_contains(m, k), "even key present");
    }

    hm_free(m);
    println("  50k stress: OK");
}

int main(void) {
    test_int_int();
    test_string_float();
    test_struct_keys();
    test_backward_compat();
    test_stress_50k();

    if (failures) {
        fprintf(stderr, "\n%d FAILURES\n", failures);
        return 1;
    }
    println("\n=== test_hashmap: ALL PASSED ===");
    return 0;
}
