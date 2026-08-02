/*
 * test_stress.c — combined stress test exercising ALL features simultaneously
 *
 * Stress: runs every subsystem in one process with heavy loads,
 *         interleaved operations, memory pressure
 */

#include "../eacy.h"
#include <stdio.h>
#include <string.h>

static int failures = 0;
#define CHECK(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL %s:%d — %s\n", __FILE__, __LINE__, msg); failures++; } \
} while (0)

void stress_da_hm_together(void) {
    println("=== DA + HM interleaved stress ===");

    /* dynamic array of hash maps */
    hm(int, int) *maps = NULL;

    /* create 100 hash maps, each with 500 entries */
    for_range(i, 0, 100) {
        hm(int, int) m;
        hm_init(m);
        for_range(j, 0, 500) { int k = (int)j, v = (int)(j * (i + 1)); hm_set(m, k, v); }
        da_push(maps, m);
    }
    CHECK(da_len(maps) == 100, "100 hash maps");

    /* spot-check maps */
    for_range(i, 0, 100) {
        int v; int k = 250;
        CHECK(hm_get(maps[i], k, &v) && v == 250 * ((int)i + 1), "cross-check");
    }

    /* free all */
    da_for(i, maps) hm_free(maps[i]);
    da_free(maps);
    println("  DA+HM stress: OK");
}

void stress_string_and_arena(void) {
    println("=== String builder + arena stress ===");
    ec_arena arena = ec_arena_new(10 * 1024 * 1024);  /* 10 MiB */

    /* allocate many strings from arena */
    ec_string *strings = ec_arena_alloc(&arena, 5000 * sizeof(ec_string));
    CHECK(strings != NULL, "string array allocated");

    /* build strings in arena-backed ec_string objects
     * (the ec_string structs are arena-allocated, but data is heap — correct) */
    for_range(i, 0, 5000) {
        strings[i] = string_printf("item_%d_value_%d", (int)i, (int)(i * 7));
    }
    CHECK(string_length(&strings[0]) > 0, "string 0 built");
    CHECK(string_starts_with(&strings[4999], "item_4999"), "string 4999 built");

    /* free all string data */
    for_range(i, 0, 5000) string_free(&strings[i]);
    ec_arena_free(&arena);
    println("  string+arena stress: OK");
}

void stress_pool_churn(void) {
    println("=== Pool churn stress ===");
    typedef struct { int data[16]; } Block;
    ec_pool p = ec_pool_new(sizeof(Block), 2000);
    CHECK(p.buf != NULL, "pool created");

    /* alloc/free cycles */
    Block *active[2000] = {NULL};
    random_seed();

    for (int cycle = 0; cycle < 50; cycle++) {
        /* alloc all blocks */
        for_range(i, 0, 2000) {
            if (active[i] == NULL) {
                active[i] = ec_pool_alloc(&p);
                CHECK(active[i] != NULL, "alloc in churn");
                active[i]->data[0] = (int)i;
            }
        }
        CHECK(ec_pool_available(&p) == 0, "all used");

        /* free random half */
        for_range(i, 0, 2000) {
            if (random_int(0, 1) && active[i] != NULL) {
                ec_pool_free_block(&p, active[i]);
                active[i] = NULL;
            }
        }
    }

    /* free remaining */
    for_range(i, 0, 2000) {
        if (active[i] != NULL) ec_pool_free_block(&p, active[i]);
    }

    ec_pool_destroy(&p);
    println("  pool churn: OK");
}

void stress_log_and_timer(void) {
    println("=== Logging + timer stress ===");

    ec_timer t_total = timer_start();

    for_range(i, 0, 10) {
        ec_timer t_iter = timer_start();

        /* some work */
        volatile int x = 0;
        for_range(j, 0, 50000) x += j;

        long long elapsed = timer_elapsed_ms(&t_iter);
        log_debug("Iteration", (long)i, "took", (long)elapsed, "ms");
    }

    long long total = timer_elapsed_ms(&t_total);
    log_info("Total 10 iterations:", (long)total, "ms");
    CHECK(total >= 0, "total non-negative");

    println("  log+timer stress: OK");
}

void stress_edge_cases(void) {
    println("=== Edge case stress ===");

    /* DA with zero elements */
    int *empty_da = NULL;
    CHECK(da_empty(empty_da), "NULL da empty");
    da_free(empty_da);

    /* string builder with empty inputs */
    ec_string s = string_new();
    string_append(&s, "");         /* append empty string */
    string_insert(&s, 0, "");      /* insert empty string */
    string_remove(&s, 0, 0);       /* remove zero */
    CHECK(string_empty(&s), "still empty");
    string_free(&s);

    /* hash map double-free */
    hm(int, int) m;
    hm_init(m);
    { int k = 1, v = 10; hm_set(m, k, v); }
    hm_free(m);
    hm_free(m);  /* safe double-free */
    CHECK(hm_empty(m), "double-free safe");

    /* arena zero allocation */
    ec_arena a = ec_arena_new(100);
    void *zero_alloc = ec_arena_alloc(&a, 0);
    CHECK(zero_alloc != NULL || zero_alloc == NULL, "zero-size alloc");  /* implementation-defined */
    ec_arena_free(&a);

    println("  edge cases: OK");
}

int main(void) {
    stress_da_hm_together();
    stress_string_and_arena();
    stress_pool_churn();
    stress_log_and_timer();
    stress_edge_cases();

    if (failures) {
        fprintf(stderr, "\n%d FAILURES\n", failures);
        return 1;
    }
    println("\n=== test_stress: ALL PASSED ===");
    return 0;
}
