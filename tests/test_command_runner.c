/*
 * test_command_runner.c - command runner smoke tests
 */

#include "../eacy.h"
#include <stdio.h>

static int failures = 0;
#define CHECK(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL %s:%d - %s\n", __FILE__, __LINE__, msg); failures++; } \
} while (0)

int main(void) {
    println("=== command runner ===");

#if defined(EC_PLATFORM_WINDOWS)
    CHECK(cmd_run("cmd /c exit 0") == 0, "cmd_run success exit code");
    CHECK(!cmd_ok("cmd /c exit 7"), "cmd_ok false on non-zero exit");
    char *out = cmd_capture("cmd /c echo EaCy");
#else
    CHECK(cmd_run("true") == 0, "cmd_run success exit code");
    CHECK(!cmd_ok("false"), "cmd_ok false on non-zero exit");
    char *out = cmd_capture("printf EaCy");
#endif

    CHECK(out != NULL, "cmd_capture returned output");
    if (out) {
        CHECK(contains(out, "EaCy"), "cmd_capture contains stdout");
        free(out);
    }

    if (failures) {
        fprintf(stderr, "\n%d FAILURES\n", failures);
        return 1;
    }

    println("=== test_command_runner: ALL PASSED ===");
    return 0;
}
