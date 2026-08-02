/*
 * test_strings.c — stress-test raw string helpers
 *
 * Exercises: starts_with, ends_with, contains, equals, trim, lowercase, uppercase
 * Stress: edge cases (empty strings, null, all-whitespace, long strings)
 */

#include "../eacy.h"
#include <stdio.h>
#include <string.h>

static int failures = 0;
#define CHECK(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL %s:%d — %s\n", __FILE__, __LINE__, msg); failures++; } \
} while (0)

void test_starts_with(void) {
    println("=== starts_with ===");
    CHECK( starts_with("hello world", "hello"), "basic");
    CHECK( starts_with("hello", "hello"), "exact match");
    CHECK(!starts_with("hello", "hellx"), "mismatch");
    CHECK( starts_with("hello", ""), "empty prefix");
    CHECK( starts_with("", ""), "both empty");
    CHECK(!starts_with("", "x"), "empty str non-empty prefix");
    CHECK( starts_with("abc", "a"), "single char");
    CHECK(!starts_with("abc", "abcd"), "prefix longer than str");
    println("  starts_with: OK");
}

void test_ends_with(void) {
    println("=== ends_with ===");
    CHECK( ends_with("hello world", "world"), "basic");
    CHECK( ends_with("world", "world"), "exact match");
    CHECK(!ends_with("world", "worlx"), "mismatch");
    CHECK( ends_with("hello", ""), "empty suffix");
    CHECK(!ends_with("", "x"), "empty str non-empty suffix");
    CHECK( ends_with("abc", "c"), "single char");
    CHECK(!ends_with("abc", "abcd"), "suffix longer than str");
    CHECK( ends_with("hello.txt", ".txt"), "extension");
    println("  ends_with: OK");
}

void test_contains(void) {
    println("=== contains ===");
    CHECK( contains("abcdef", "cde"), "middle");
    CHECK( contains("abcdef", "a"), "start");
    CHECK( contains("abcdef", "f"), "end");
    CHECK( contains("abc", "abc"), "exact");
    CHECK(!contains("abc", "xyz"), "not found");
    CHECK( contains("abc", ""), "empty needle");
    CHECK(!contains("", "x"), "empty haystack");
    CHECK( contains("aaaa", "aa"), "overlapping");
    /* stress: long haystack */
    char big[2002] = {0};
    memset(big, 'x', 2000);
    big[1999] = 'y';
    CHECK( contains(big, "y"), "long string end");
    CHECK( contains(big, "x"), "long string start");
    CHECK(!contains(big, "z"), "long string miss");
    println("  contains: OK");
}

void test_equals(void) {
    println("=== equals ===");
    CHECK( equals("hello", "hello"), "same");
    CHECK(!equals("hello", "Hello"), "case diff");
    CHECK( equals("", ""), "both empty");
    CHECK(!equals("a", ""), "one empty");
    CHECK(!equals("", "b"), "other empty");
    CHECK( equals("x", "x"), "single char");
    println("  equals: OK");
}

void test_trim(void) {
    println("=== trim ===");
    char buf1[] = "   hello   ";
    char *t = trim(buf1);
    CHECK(equals(t, "hello"), "both sides");
    CHECK(t >= buf1 && t <= buf1 + sizeof(buf1), "returned pointer inside buffer");

    char buf2[] = "hello";
    t = trim(buf2);
    CHECK(equals(t, "hello"), "no whitespace");

    char buf3[] = "";
    t = trim(buf3);
    CHECK(equals(t, ""), "empty");

    char buf4[] = "      ";
    t = trim(buf4);
    CHECK(equals(t, ""), "all whitespace");

    char buf5[] = "\t\n  spaced \r\t";
    t = trim(buf5);
    CHECK(equals(t, "spaced"), "mixed whitespace");

    char buf6[] = "  a  ";
    t = trim(buf6);
    CHECK(equals(t, "a"), "single non-ws");

    println("  trim: OK");
}

void test_lowercase_uppercase(void) {
    println("=== lowercase / uppercase ===");
    char buf1[] = "HELLO World 123";
    lowercase(buf1);
    CHECK(equals(buf1, "hello world 123"), "lowercase mixed");

    uppercase(buf1);
    CHECK(equals(buf1, "HELLO WORLD 123"), "uppercase");

    char buf2[] = "";
    lowercase(buf2);
    CHECK(equals(buf2, ""), "lowercase empty");
    uppercase(buf2);
    CHECK(equals(buf2, ""), "uppercase empty");

    char buf3[] = "already lower";
    lowercase(buf3);
    CHECK(equals(buf3, "already lower"), "lowercase idempotent");

    char buf4[] = "ALREADY UPPER";
    uppercase(buf4);
    CHECK(equals(buf4, "ALREADY UPPER"), "uppercase idempotent");

    println("  lowercase/uppercase: OK");
}

int main(void) {
    test_starts_with();
    test_ends_with();
    test_contains();
    test_equals();
    test_trim();
    test_lowercase_uppercase();

    if (failures) {
        fprintf(stderr, "\n%d FAILURES\n", failures);
        return 1;
    }
    println("\n=== test_strings: ALL PASSED ===");
    return 0;
}
