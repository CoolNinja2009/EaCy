/*
 * test_pool.c — stress-test pool allocator
 *
 * Exercises: ec_pool_new, ec_pool_alloc, ec_pool_free_block,
 *            ec_pool_available, ec_pool_destroy
 * Stress: alloc/free all blocks, churn, struct alignment
 */

#include "../eacy.h"
#include <stdio.h>

static int failures = 0;
#define CHECK(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL %s:%d — %s\n", __FILE__, __LINE__, msg); failures++; } \
} while (0)

typedef struct {
    int id;
    double x, y, z;
    char name[32];
} Entity;

void test_basic(void) {
    println("=== Pool — basic ===");
    ec_pool p = ec_pool_new(sizeof(Entity), 100);
    CHECK(p.buf != NULL, "allocation ok");
    CHECK(p.block_count == 100, "block_count");
    CHECK(ec_pool_available(&p) == 100, "all available");

    /* alloc a few */
    Entity *e1 = ec_pool_alloc(&p);
    Entity *e2 = ec_pool_alloc(&p);
    Entity *e3 = ec_pool_alloc(&p);
    CHECK(e1 != NULL && e2 != NULL && e3 != NULL, "3 allocs");
    CHECK(ec_pool_available(&p) == 97, "97 available");

    /* use them */
    e1->id = 1; e1->x = 1.0; strcpy(e1->name, "first");
    e2->id = 2; e2->x = 2.0; strcpy(e2->name, "second");

    /* free and verify reuse */
    ec_pool_free_block(&p, e2);
    CHECK(ec_pool_available(&p) == 98, "98 after free");

    Entity *e4 = ec_pool_alloc(&p);
    CHECK(e4 == e2, "reuse freed block");  /* LIFO — same pointer */

    ec_pool_free_block(&p, e1);
    ec_pool_free_block(&p, e3);
    ec_pool_free_block(&p, e4);
    CHECK(ec_pool_available(&p) == 100, "all returned");

    /* NULL free is safe */
    ec_pool_free_block(&p, NULL);

    ec_pool_destroy(&p);
    CHECK(p.buf == NULL, "destroyed buf NULL");
    ec_pool_destroy(&p);  /* double-free safe */

    println("  basic: OK");
}

void test_exhaustion(void) {
    println("=== Pool — exhaustion ===");
    ec_pool p = ec_pool_new(64, 10);
    CHECK(p.buf != NULL, "alloc ok");

    /* alloc all 10 */
    void *blocks[10];
    for_range(i, 0, 10) {
        blocks[i] = ec_pool_alloc(&p);
        CHECK(blocks[i] != NULL, "block alloc");
    }
    CHECK(ec_pool_available(&p) == 0, "none available");

    /* next alloc fails */
    CHECK(ec_pool_alloc(&p) == NULL, "exhausted NULL");

    /* return one, alloc again */
    ec_pool_free_block(&p, blocks[5]);
    CHECK(ec_pool_available(&p) == 1, "1 available");
    void *again = ec_pool_alloc(&p);
    CHECK(again != NULL, "alloc after return");
    CHECK(again == blocks[5], "same block returned");

    /* free all */
    for_range(i, 0, 10) {
        if ((size_t)i != 5) ec_pool_free_block(&p, blocks[i]);
    }
    ec_pool_free_block(&p, again);

    ec_pool_destroy(&p);
    println("  exhaustion: OK");
}

void test_small_block(void) {
    println("=== Pool — small block (auto-align) ===");
    /* block smaller than pointer — must be rounded up */
    ec_pool p = ec_pool_new(1, 50);
    CHECK(p.buf != NULL, "alloc ok");
    CHECK(p.block_size >= sizeof(void*), "block_size aligned");
    CHECK(ec_pool_available(&p) == 50, "50 available");

    for_range(i, 0, 50) {
        char *c = ec_pool_alloc(&p);
        CHECK(c != NULL, "small alloc");
        *c = (char)i;
    }
    CHECK(ec_pool_available(&p) == 0, "all used");
    CHECK(ec_pool_alloc(&p) == NULL, "exhausted");

    ec_pool_destroy(&p);
    println("  small block: OK");
}

void test_struct_alignment(void) {
    println("=== Pool — struct alignment ===");
    ec_pool p = ec_pool_new(sizeof(Entity), 500);
    CHECK(p.buf != NULL, "alloc ok");

    Entity *ents[500];
    for_range(i, 0, 500) {
        ents[i] = ec_pool_alloc(&p);
        CHECK(ents[i] != NULL, "alloc");
        ents[i]->id = (int)i;
        ents[i]->x = (double)i * 1.5;
        ents[i]->y = (double)i * 2.5;
        ents[i]->z = (double)i * 3.5;
    }

    /* verify all intact */
    for_range(i, 0, 500) {
        CHECK(ents[i]->id == (int)i, "id intact");
        CHECK(ents[i]->x == (double)i * 1.5, "x intact");
    }

    /* free odd indices */
    for_range(i, 1, 500) if (i % 2 == 1) ec_pool_free_block(&p, ents[i]);
    CHECK(ec_pool_available(&p) == 250, "250 available after free");

    /* re-alloc odd slots */
    for_range(i, 1, 500) if (i % 2 == 1) {
        Entity *e = ec_pool_alloc(&p);
        CHECK(e != NULL, "re-alloc");
        e->id = 9999;
    }

    /* verify even slots intact */
    for_range(i, 0, 500) if (i % 2 == 0) {
        CHECK(ents[i]->id == (int)i, "even intact after churn");
    }

    ec_pool_destroy(&p);
    println("  struct alignment: OK");
}

int main(void) {
    test_basic();
    test_exhaustion();
    test_small_block();
    test_struct_alignment();

    if (failures) {
        fprintf(stderr, "\n%d FAILURES\n", failures);
        return 1;
    }
    println("\n=== test_pool: ALL PASSED ===");
    return 0;
}
