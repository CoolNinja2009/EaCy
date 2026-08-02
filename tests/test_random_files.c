/*
 * test_random_files.c — stress-test random numbers and file I/O
 *
 * Exercises: random_seed, random_int, random_float,
 *            read_text_file, write_text_file, append_text_file
 * Stress: distribution checks, file create/read/append/delete cycle
 */

#include "../eacy.h"
#include <stdio.h>
#include <math.h>

static int failures = 0;
#define CHECK(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL %s:%d — %s\n", __FILE__, __LINE__, msg); failures++; } \
} while (0)

void test_random_int(void) {
    println("=== random_int ===");
    random_seed();

    /* range check */
    for_range(i, 0, 10000) {
        int r = random_int(1, 6);
        CHECK(r >= 1 && r <= 6, "dice in range");
    }

    /* swapped bounds (auto-corrected) */
    for_range(i, 0, 100) {
        int r = random_int(10, 1);
        CHECK(r >= 1 && r <= 10, "swapped bounds");
    }

    /* single value range */
    for_range(i, 0, 100) {
        int r = random_int(42, 42);
        CHECK(r == 42, "single value range");
    }

    /* negative range */
    for_range(i, 0, 1000) {
        int r = random_int(-10, 10);
        CHECK(r >= -10 && r <= 10, "negative range");
    }

    println("  random_int: OK");
}

void test_random_float(void) {
    println("=== random_float ===");
    float sum = 0.0f;
    int count = 10000;
    for_range(i, 0, count) {
        float r = random_float(0.0f, 1.0f);
        CHECK(r >= 0.0f && r <= 1.0f, "in range [0,1]");
        sum += r;
    }
    /* average should be ~0.5 */
    float avg = sum / (float)count;
    CHECK(avg > 0.45f && avg < 0.55f, "avg near 0.5");

    /* negative to positive range */
    for_range(i, 0, 1000) {
        float r = random_float(-100.0f, 100.0f);
        CHECK(r >= -100.0f && r <= 100.0f, "in range [-100,100]");
    }

    println("  random_float: OK");
}

void test_file_write_read(void) {
    println("=== File write / read ===");
    const char *path = "_test_file.txt";
    const char *content = "Hello, EaCy file test!\nLine 2\nLine 3";

    /* write */
    CHECK(write_text_file(path, content), "write_text_file");
    CHECK(write_text_file(path, "overwritten"), "write_text_file overwrite");

    /* read back */
    char *text = read_text_file(path);
    CHECK(text != NULL, "read_text_file");
    CHECK(equals(text, "overwritten"), "content matches");
    free(text);

    remove(path);
    println("  write/read: OK");
}

void test_file_append(void) {
    println("=== File append ===");
    const char *path = "_test_append.txt";

    /* start fresh */
    CHECK(write_text_file(path, "line1\n"), "write initial");

    /* append */
    CHECK(append_text_file(path, "line2\n"), "append 1");
    CHECK(append_text_file(path, "line3\n"), "append 2");

    /* read back */
    char *text = read_text_file(path);
    CHECK(text != NULL, "read back");
    CHECK(equals(text, "line1\nline2\nline3\n"), "appended content");
    free(text);

    remove(path);
    println("  append: OK");
}

void test_file_missing(void) {
    println("=== File missing ===");
    char *text = read_text_file("_nonexistent_file_xyz.txt");
    CHECK(text == NULL, "missing file returns NULL");
    println("  missing file: OK");
}

void test_file_empty(void) {
    println("=== File empty ===");
    const char *path = "_test_empty.txt";
    CHECK(write_text_file(path, ""), "write empty");

    char *text = read_text_file(path);
    CHECK(text != NULL, "read empty file");
    CHECK(equals(text, ""), "empty content");
    free(text);

    remove(path);
    println("  empty file: OK");
}

int main(void) {
    test_random_int();
    test_random_float();
    test_file_write_read();
    test_file_append();
    test_file_missing();
    test_file_empty();

    if (failures) {
        fprintf(stderr, "\n%d FAILURES\n", failures);
        return 1;
    }
    println("\n=== test_random_files: ALL PASSED ===");
    return 0;
}
