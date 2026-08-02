/*
 * test_da.c — stress-test dynamic arrays with all macros
 *
 * Exercises: da_init, da_push, da_pop, da_insert, da_remove, da_resize,
 *            da_copy, da_sort, da_for, da_push_many, da_reserve, da_clear,
 *            da_free, da_front, da_back, da_empty, da_len, da_cap
 * Stress: 100k elements, bulk ops, sort, copy, resize up/down
 */

#include "../eacy.h"
#include <stdio.h>
#include <string.h>

static int failures = 0;
#define CHECK(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL %s:%d — %s\n", __FILE__, __LINE__, msg); failures++; } \
} while (0)

void test_basic_ops(void) {
    println("=== Dynamic Arrays — basic ops ===");
    int *arr = NULL;
    CHECK(da_len(arr) == 0, "len NULL");
    CHECK(da_cap(arr) == 0, "cap NULL");
    CHECK(da_empty(arr), "empty NULL");

    da_push(arr, 10);
    da_push(arr, 20);
    da_push(arr, 30);
    CHECK(da_len(arr) == 3, "len after 3 pushes");
    CHECK(!da_empty(arr), "not empty");
    CHECK(da_front(arr) == 10, "front");
    CHECK(da_back(arr) == 30, "back");

    CHECK(da_pop(arr) == 30, "pop 30");
    CHECK(da_len(arr) == 2, "len after pop");
    CHECK(da_back(arr) == 20, "back after pop");

    da_clear(arr);
    CHECK(da_len(arr) == 0, "len after clear");
    CHECK(da_empty(arr), "empty after clear");

    da_free(arr);
    CHECK(arr == NULL, "NULL after free");
    println("  basic ops: OK");
}

void test_insert_remove(void) {
    println("=== insert / remove ===");
    int *arr = NULL;
    da_push(arr, 1);
    da_push(arr, 3);
    da_insert(arr, 1, 2);  /* [1,2,3] */
    CHECK(da_len(arr) == 3, "len after insert");
    CHECK(arr[0]==1 && arr[1]==2 && arr[2]==3, "insert middle");

    da_insert(arr, 0, 0);  /* [0,1,2,3] */
    CHECK(arr[0]==0, "insert at start");

    da_insert(arr, da_len(arr), 4);  /* [0,1,2,3,4] */
    CHECK(arr[4]==4, "insert at end");

    da_remove(arr, 0);  /* [1,2,3,4] */
    CHECK(da_len(arr)==4 && arr[0]==1, "remove start");

    da_remove(arr, 2);  /* [1,2,4] */
    CHECK(da_len(arr)==3 && arr[2]==4, "remove middle");

    da_remove(arr, da_len(arr)-1);  /* [1,2] */
    CHECK(da_len(arr)==2 && arr[1]==2, "remove end");

    da_free(arr);
    println("  insert/remove: OK");
}

void test_resize(void) {
    println("=== resize ===");
    int *arr = NULL;
    da_push(arr, 1);
    da_push(arr, 2);

    da_resize(arr, 5);  /* grow with zeros */
    CHECK(da_len(arr) == 5, "len after grow resize");
    CHECK(arr[0]==1 && arr[1]==2, "preserved existing");
    CHECK(arr[2]==0 && arr[3]==0 && arr[4]==0, "zero-filled");

    da_resize(arr, 1);  /* shrink */
    CHECK(da_len(arr) == 1, "len after shrink");
    CHECK(arr[0]==1, "preserved after shrink");

    da_resize(arr, 0);  /* shrink to zero */
    CHECK(da_len(arr) == 0, "len after zero resize");

    da_free(arr);
    println("  resize: OK");
}

void test_sort(void) {
    println("=== sort ===");
    int *arr = NULL;
    da_push(arr, 5);
    da_push(arr, 2);
    da_push(arr, 8);
    da_push(arr, 2);
    da_push(arr, 1);

    da_sort(arr, ec_cmp_int);
    CHECK(arr[0]==1 && arr[1]==2 && arr[2]==2 && arr[3]==5 && arr[4]==8, "sort asc");

    da_sort(arr, ec_cmp_int_desc);
    CHECK(arr[0]==8 && arr[4]==1, "sort desc");

    /* float sort */
    float *farr = NULL;
    da_push(farr, 3.5f);
    da_push(farr, 1.0f);
    da_push(farr, 2.5f);
    da_sort(farr, ec_cmp_float);
    CHECK(farr[0]==1.0f && farr[1]==2.5f && farr[2]==3.5f, "float sort");

    da_free(arr);
    da_free(farr);
    println("  sort: OK");
}

void test_copy(void) {
    println("=== copy ===");
    int *src = NULL;
    da_push(src, 10);
    da_push(src, 20);
    da_push(src, 30);

    int *cpy = da_copy(src, int);
    CHECK(cpy != NULL, "copy not NULL");
    CHECK(da_len(cpy) == 3, "copy len");
    CHECK(cpy[0]==10 && cpy[1]==20 && cpy[2]==30, "copy values");
    CHECK(cpy != src, "copy is independent");

    /* modify original, copy unchanged */
    da_push(src, 40);
    CHECK(da_len(cpy) == 3, "copy independent after source grow");

    da_free(src);
    da_free(cpy);
    println("  copy: OK");
}

void test_push_many(void) {
    println("=== push_many ===");
    int *arr = NULL;
    int data[] = {1, 2, 3, 4, 5};
    da_push_many(arr, data, 5);
    CHECK(da_len(arr) == 5, "push_many len");
    CHECK(arr[0]==1 && arr[4]==5, "push_many values");

    /* append more */
    int more[] = {6, 7, 8};
    da_push_many(arr, more, 3);
    CHECK(da_len(arr) == 8, "push_many append");
    CHECK(arr[5]==6 && arr[7]==8, "push_many append values");

    /* push_many zero count */
    da_push_many(arr, more, 0);
    CHECK(da_len(arr) == 8, "push_many zero no-op");

    da_free(arr);
    println("  push_many: OK");
}

void test_reserve(void) {
    println("=== reserve ===");
    int *arr = NULL;
    da_reserve(arr, 1000);
    CHECK(arr != NULL, "reserve allocated");
    CHECK(da_cap(arr) >= 1000, "reserve capacity");
    CHECK(da_len(arr) == 0, "reserve doesn't change len");

    /* push after reserve should not realloc (capacity sufficient) */
    size_t cap_before = da_cap(arr);
    for_range(i, 0, 500) da_push(arr, (int)i);
    CHECK(da_cap(arr) == cap_before, "no realloc after reserve");

    da_free(arr);
    println("  reserve: OK");
}

void test_stress_100k(void) {
    println("=== Stress: 100,000 elements ===");
    int *arr = NULL;

    /* push 100k */
    for_range(i, 0, 100000) da_push(arr, (int)(i * 3 % 10000));
    CHECK(da_len(arr) == 100000, "100k len");

    /* verify some positions */
    CHECK(arr[0] == 0, "arr[0]");
    CHECK(arr[50000] == (int)(50000*3 % 10000), "arr[50000]");
    CHECK(arr[99999] == (int)(99999*3 % 10000), "arr[99999]");

    /* sort and verify */
    da_sort(arr, ec_cmp_int);
    CHECK(arr[0] <= arr[50000] && arr[50000] <= arr[99999], "sorted monotonic");

    /* pop all */
    while (!da_empty(arr)) da_pop(arr);
    CHECK(da_len(arr) == 0, "all popped");

    da_free(arr);
    println("  100k stress: OK");
}

int main(void) {
    test_basic_ops();
    test_insert_remove();
    test_resize();
    test_sort();
    test_copy();
    test_push_many();
    test_reserve();
    test_stress_100k();

    if (failures) {
        fprintf(stderr, "\n%d FAILURES\n", failures);
        return 1;
    }
    println("\n=== test_da: ALL PASSED ===");
    return 0;
}
