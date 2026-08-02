/*
 * test_string_builder.c — stress-test ec_string (all functions)
 *
 * Exercises: every string_* function, edge cases, large strings
 * Stress: 10k appends, insert/remove at boundaries, format stress
 */

#include "../eacy.h"
#include <stdio.h>
#include <string.h>

static int failures = 0;
#define CHECK(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL %s:%d — %s\n", __FILE__, __LINE__, msg); failures++; } \
} while (0)

void test_create_and_basic(void) {
    println("=== create & basic ===");
    ec_string s = string_new();
    CHECK(string_empty(&s), "empty new");
    CHECK(string_length(&s) == 0, "len 0");
    CHECK(string_len(&s) == 0, "string_len 0");
    CHECK(string_capacity(&s) == 0, "cap 0 (no alloc)");
    CHECK(equals(string_cstr(&s), ""), "cstr empty");

    /* string_from */
    ec_string s2 = string_from("hello");
    CHECK(string_length(&s2) == 5, "from len");
    CHECK(equals(s2.data, "hello"), "from content");
    CHECK(string_capacity(&s2) >= 5, "from capacity");

    /* string_from(NULL) */
    ec_string s3 = string_from(NULL);
    CHECK(string_empty(&s3), "from NULL empty");

    /* string_printf */
    ec_string s4 = string_printf("v%d.%d.%d", 1, 2, 3);
    CHECK(equals(s4.data, "v1.2.3"), "printf");
    CHECK(string_length(&s4) == 6, "printf len");

    string_free(&s);
    string_free(&s2);
    string_free(&s3);
    string_free(&s4);
    println("  create: OK");
}

void test_append(void) {
    println("=== append ===");
    ec_string s = string_new();

    string_append(&s, "Hello");
    string_append_char(&s, ' ');
    string_append(&s, "World");
    CHECK(equals(s.data, "Hello World"), "append str+char");

    string_append_int(&s, 42);
    CHECK(equals(s.data, "Hello World42"), "append_int");

    string_append_long(&s, 1234567890L);
    CHECK(string_contains(&s, "1234567890"), "append_long");

    string_append_float(&s, 3.14f);
    CHECK(string_contains(&s, "3.14"), "append_float");

    string_append_double(&s, 2.718);
    CHECK(string_contains(&s, "2.718"), "append_double");

    string_append_bool(&s, true);
    CHECK(string_ends_with(&s, "true"), "append_bool true");

    string_append_bool(&s, false);
    CHECK(string_ends_with(&s, "false"), "append_bool false");

    string_appendf(&s, " [%d/%d]", 5, 10);
    CHECK(string_ends_with(&s, "[5/10]"), "appendf");

    string_free(&s);
    println("  append: OK");
}

void test_insert_remove(void) {
    println("=== insert / remove ===");
    ec_string s = string_from("hello world");

    string_insert(&s, 0, "say ");
    CHECK(equals(s.data, "say hello world"), "insert at 0");

    string_insert(&s, string_len(&s), "!!!");
    CHECK(equals(s.data, "say hello world!!!"), "insert at end");

    string_insert(&s, 3, "ing");
    CHECK(equals(s.data, "saying hello world!!!"), "insert middle");

    string_remove(&s, 0, 7);
    CHECK(equals(s.data, "hello world!!!"), "remove from 0");

    string_remove(&s, 5, 6);
    CHECK(equals(s.data, "hello!!!"), "remove middle");

    string_remove(&s, string_len(&s) - 3, 3);
    CHECK(equals(s.data, "hello"), "remove from end");

    /* remove past end (clamped) */
    string_remove(&s, 3, 100);
    CHECK(equals(s.data, "hel"), "remove clamped");

    /* remove from empty */
    string_clear(&s);
    string_remove(&s, 0, 1);  /* no-op */
    CHECK(string_empty(&s), "remove from empty no-op");

    /* insert into empty */
    string_insert(&s, 0, "restored");
    CHECK(equals(s.data, "restored"), "insert into empty");

    string_free(&s);
    println("  insert/remove: OK");
}

void test_inspection(void) {
    println("=== inspection ===");
    ec_string s = string_from("hello world");

    CHECK(string_length(&s) == 11, "length");
    CHECK(string_len(&s) == 11, "len");
    CHECK(!string_empty(&s), "not empty");
    CHECK(string_capacity(&s) >= 11, "capacity");

    CHECK(string_contains(&s, "world"), "contains");
    CHECK(!string_contains(&s, "xyz"), "!contains");
    CHECK(string_starts_with(&s, "hello"), "starts_with");
    CHECK(!string_starts_with(&s, "world"), "!starts_with");
    CHECK(string_ends_with(&s, "world"), "ends_with");
    CHECK(!string_ends_with(&s, "hello"), "!ends_with");

    /* string_equals */
    ec_string s2 = string_from("hello world");
    CHECK(string_equals(&s, &s2), "equals");
    ec_string s3 = string_from("different");
    CHECK(!string_equals(&s, &s3), "!equals");

    /* string_cstr */
    CHECK(equals(string_cstr(&s), "hello world"), "cstr");

    string_free(&s);
    string_free(&s2);
    string_free(&s3);
    println("  inspection: OK");
}

void test_mutation(void) {
    println("=== mutation ===");
    ec_string s = string_from("  HeLLo WoRLd  ");
    string_trim(&s);
    CHECK(equals(s.data, "HeLLo WoRLd"), "trim");

    string_lowercase(&s);
    CHECK(equals(s.data, "hello world"), "lowercase");

    string_uppercase(&s);
    CHECK(equals(s.data, "HELLO WORLD"), "uppercase");

    /* empty string mutation */
    ec_string e = string_new();
    string_trim(&e);     /* no-op */
    string_lowercase(&e);  /* no-op */
    string_uppercase(&e);  /* no-op */
    CHECK(string_empty(&e), "empty mutation safe");

    string_free(&s);
    string_free(&e);
    println("  mutation: OK");
}

void test_clear_reuse(void) {
    println("=== clear & reuse ===");
    ec_string s = string_from("initial content");
    size_t cap = string_capacity(&s);

    string_clear(&s);
    CHECK(string_empty(&s), "empty after clear");
    CHECK(string_capacity(&s) == cap, "capacity preserved");

    /* reuse buffer */
    string_append(&s, "reused");
    CHECK(equals(s.data, "reused"), "reuse after clear");
    CHECK(string_capacity(&s) == cap, "still same capacity");

    string_free(&s);
    println("  clear/reuse: OK");
}

void test_stress_10k(void) {
    println("=== Stress: 10,000 appends ===");
    ec_string s = string_new();

    /* append numbers 0..9999 */
    for_range(i, 0, 10000) {
        string_append_int(&s, (int)i);
        string_append_char(&s, ',');
    }
    CHECK(string_length(&s) > 40000, "10k length sanity");
    CHECK(string_starts_with(&s, "0,1,2,3,"), "starts with 0,1,2,3,");

    /* clear and rebuild */
    string_clear(&s);
    for_range(i, 0, 5000) {
        string_appendf(&s, "item_%d ", (int)i);
    }
    CHECK(string_starts_with(&s, "item_0 item_1"), "fmt rebuild");

    string_free(&s);
    println("  10k stress: OK");
}

int main(void) {
    test_create_and_basic();
    test_append();
    test_insert_remove();
    test_inspection();
    test_mutation();
    test_clear_reuse();
    test_stress_10k();

    if (failures) {
        fprintf(stderr, "\n%d FAILURES\n", failures);
        return 1;
    }
    println("\n=== test_string_builder: ALL PASSED ===");
    return 0;
}
