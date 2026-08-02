/*
 * test_cli.c — stress-test CLI argument parsing
 *
 * Exercises: ec_args_new, ec_args_has, ec_args_val, ec_args_pos,
 *            ec_args_pos_count, ec_args_count
 * Stress: many flags, -- and - forms, / flag form (Windows)
 */

#include "../eacy.h"
#include <stdio.h>
#include <string.h>

static int failures = 0;
#define CHECK(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL %s:%d — %s\n", __FILE__, __LINE__, msg); failures++; } \
} while (0)

int main(int argc, char **argv) {
    ec_args args = ec_args_new(argc, argv);

    println("=== CLI Arguments ===");
    println("argc:", (long)ec_args_count(&args));

    /* flags */
    if (ec_args_has(&args, "--verbose") || ec_args_has(&args, "-v"))
        println("  verbose: ON");
    else
        println("  verbose: OFF");

    if (ec_args_has(&args, "--help") || ec_args_has(&args, "-h") || ec_args_has(&args, "/?")) {
        println("  Help requested");
    }

    /* key-value */
    const char *output = ec_args_val(&args, "--output");
    if (!output) output = ec_args_val(&args, "-o");
    if (output) print("  output:", output);

    /* positional args */
    int pos_count = ec_args_pos_count(&args);
    println("  positional count:", (long)pos_count);
    for_range(i, 0, pos_count) {
        print("   ", (long)i, ":", ec_args_pos(&args, i));
    }

    /* internal checks */
    CHECK(ec_args_count(&args) == argc, "arg count matches");
    CHECK(ec_args_pos_count(&args) == (argc > 0 ? argc - 1 : 0), "pos count = argc-1");

    if (failures) {
        fprintf(stderr, "\n%d FAILURES\n", failures);
        return 1;
    }
    println("\n=== test_cli: ALL PASSED ===");
    return 0;
}
