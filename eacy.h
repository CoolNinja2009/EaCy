/*
 * ============================================================================
 *  EaCy — a small header-only library that makes C pleasant to write
 * ============================================================================
 *
 *  Usage:
 *      #include "eacy.h"
 *
 *  EaCy is:
 *    - Pure C11 (uses _Generic, so a C11-capable compiler is required).
 *    - Header-only: one file, no build step, no linking.
 *    - Dependency-free: only the C standard library is used.
 *    - Portable: tested to compile cleanly under GCC, Clang, and MSVC,
 *      on both Windows and Linux.
 *
 *  Design notes:
 *    - Almost everything is `static inline`, so including this header in
 *      multiple translation units is safe and produces no duplicate-symbol
 *      linker errors.
 *    - Macros are used only where they add real ergonomic value (print(),
 *      repeat(), foreach(), min()/max()/clamp(), etc). Everything else is
 *      a plain function so it behaves predictably and is easy to step
 *      through in a debugger.
 *    - EaCy never allocates memory on the heap behind your back. Functions
 *      that allocate, such as `read_text_file()`, `cmd_capture()`, and the
 *      `ec_*` memory helpers, document caller ownership.
 *
 *  License: public domain / CC0. Do whatever you want with it.
 * ============================================================================
 */

#ifndef EACY_H
#define EACY_H

/* Request POSIX.1-2008 declarations (clock_gettime, nanosleep, CLOCK_MONOTONIC)
 * from glibc's headers. This must be defined before any system header is
 * included. It only affects non-Windows builds; MSVC ignores it. Guarded so
 * that a user who already set a feature-test macro on the command line is
 * not overridden. */
#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
    #define _POSIX_C_SOURCE 200809L
#endif


/* ---------------------------------------------------------------------------
 * Standard headers
 * ------------------------------------------------------------------------ */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

/* ---------------------------------------------------------------------------
 * Platform detection
 * ------------------------------------------------------------------------ */
#if defined(_WIN32)
    #define EC_PLATFORM_WINDOWS 1
#else
    #define EC_PLATFORM_POSIX 1
#endif

#if defined(EC_PLATFORM_WINDOWS)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
    #include <unistd.h>
    #include <sys/time.h>
    #include <sys/wait.h>
#endif

/* MSVC does not define `inline` for plain C the same way GCC/Clang do in
 * older standards, but under /std:c11 or /std:c17 it is fine. To be extra
 * safe on older MSVC configurations, map it explicitly. */
#if defined(_MSC_VER) && !defined(__cplusplus)
    #define EC_INLINE static __inline
#else
    #define EC_INLINE static inline
#endif

/* ===========================================================================
 * Section 0: small internal helpers used by the macros below
 *            (prefixed EC_ / ec_ to avoid polluting the user's namespace)
 * ========================================================================= */

/* Two-step token pasting so that arguments (like __LINE__) are expanded
 * before being pasted together. This is what lets repeat() / for_range()
 * be nested safely: each expansion gets a uniquely-named loop variable. */
#define EC_CONCAT_(a, b) a##b
#define EC_CONCAT(a, b) EC_CONCAT_(a, b)

/* Counts how many arguments were passed to a variadic macro (1..8).
 * This is a well-known, standard-conforming C99/C11 trick: we append a
 * descending sequence of numbers after the user's arguments, then pick
 * out the slot that lands exactly on the argument count. The sequence
 * is one slot longer than the max supported argument count so that the
 * trailing "..." in EC_ARG_COUNT_ is never left empty (some compilers
 * emit a pedantic warning otherwise). */
#define EC_ARG_COUNT(...) \
    EC_ARG_COUNT_(__VA_ARGS__, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define EC_ARG_COUNT_(_1, _2, _3, _4, _5, _6, _7, _8, _9, N, ...) N

/* "For each" expansion: applies f() to every argument, in order, and
 * calls ec_print_sep() (or any other separator function) between them.
 * Supports 1..8 arguments, which is enough for println()-style calls. */
#define EC_FE_1(f, s, x)          f(x)
#define EC_FE_2(f, s, x, ...)     f(x); s(); EC_FE_1(f, s, __VA_ARGS__)
#define EC_FE_3(f, s, x, ...)     f(x); s(); EC_FE_2(f, s, __VA_ARGS__)
#define EC_FE_4(f, s, x, ...)     f(x); s(); EC_FE_3(f, s, __VA_ARGS__)
#define EC_FE_5(f, s, x, ...)     f(x); s(); EC_FE_4(f, s, __VA_ARGS__)
#define EC_FE_6(f, s, x, ...)     f(x); s(); EC_FE_5(f, s, __VA_ARGS__)
#define EC_FE_7(f, s, x, ...)     f(x); s(); EC_FE_6(f, s, __VA_ARGS__)
#define EC_FE_8(f, s, x, ...)     f(x); s(); EC_FE_7(f, s, __VA_ARGS__)

#define EC_FE_SELECT_(N) EC_FE_##N
#define EC_FE_SELECT(N) EC_FE_SELECT_(N)

#define EC_FOR_EACH(f, s, ...) \
    EC_FE_SELECT(EC_ARG_COUNT(__VA_ARGS__))(f, s, __VA_ARGS__)

/* ===========================================================================
 * Section 1: Simple printing — print(...) / println(...)
 * ========================================================================= */

/* Each of these prints exactly one value, with no trailing newline or
 * separator. print()/println() below choose which one to call based on
 * the type of each argument, using _Generic. You normally never call
 * these directly. */
EC_INLINE void ec_print_int(int v)          { printf("%d", v); }
EC_INLINE void ec_print_long(long v)        { printf("%ld", v); }
EC_INLINE void ec_print_uint(unsigned int v) { printf("%u", v); }
EC_INLINE void ec_print_ulong(unsigned long v) { printf("%lu", v); }
EC_INLINE void ec_print_ullong(unsigned long long v) { printf("%llu", v); }
EC_INLINE void ec_print_float(float v)      { printf("%g", (double)v); }
EC_INLINE void ec_print_double(double v)    { printf("%g", v); }
EC_INLINE void ec_print_char(char v)        { putchar(v); }
EC_INLINE void ec_print_str(char *v)        { fputs(v, stdout); }
EC_INLINE void ec_print_cstr(const char *v) { fputs(v, stdout); }
EC_INLINE void ec_print_bool(bool v)        { fputs(v ? "true" : "false", stdout); }
EC_INLINE void ec_print_ptr(const void *v)  { printf("%p", v); }

/* Prints a single space. Used as the separator between arguments of
 * print("a", "b", "c") -> "a b c". */
EC_INLINE void ec_print_sep(void) { putchar(' '); }

/* _Generic-based dispatch: picks the right ec_print_* function for the
 * type of `x` at compile time. Falls back to printing a pointer for any
 * unrecognised type, which is usually the most useful thing to do.
 *
 * Supported types: int, long, unsigned int, unsigned long, unsigned long long,
 * float, double, char, char*, const char*, bool. String literals decay to
 * `char*`/`const char*` here exactly like they would as a normal function
 * argument.
 *
 * A plain-C quirk worth knowing: character constants like 'c' and the
 * true/false macros have type `int` in C (not `char`/`bool`), so
 * print('c') prints 99 and print(true) prints 1. Store the value in a
 * `char`/`bool` variable first if you want the char/word form:
 *     char c = 'c'; bool ok = true;
 *     println(c, ok);   // -> "c true"
 */
#define ec_print_one(x) _Generic((x), \
    bool:            ec_print_bool,   \
    char:            ec_print_char,   \
    int:             ec_print_int,    \
    long:            ec_print_long,   \
    unsigned int:    ec_print_uint,   \
    unsigned long:   ec_print_ulong,  \
    unsigned long long: ec_print_ullong, \
    float:           ec_print_float,  \
    double:          ec_print_double, \
    char*:           ec_print_str,    \
    const char*:     ec_print_cstr,   \
    default:         ec_print_ptr     \
)(x)

/**
 * print(...) — prints 1 to 8 values of (almost) any supported type,
 * separated by single spaces, followed by a trailing newline.
 *
 * Type is detected automatically at compile time — no format strings.
 * For just a blank line with nothing to print, use nl() instead of
 * calling print() with no arguments.
 *
 * Example:
 *     print("Age:", age);   // -> "Age: 25\n"
 */
#define print(...) \
    do { EC_FOR_EACH(ec_print_one, ec_print_sep, __VA_ARGS__); putchar('\n'); } while (0)

/**
 * println(...) — identical to print(...); kept as an alias so existing
 * code (and anyone coming from languages with a separate println) still
 * reads naturally. Both always end with a newline.
 *
 * Example:
 *     println("Done");      // -> "Done\n"
 */
#define println(...) print(__VA_ARGS__)

/**
 * nl() — prints a single newline character. The readable way to print
 * a blank line, since print()/println() both require at least one
 * argument.
 *
 * Example:
 *     println("First line");
 *     nl();
 *     println("Third line, with a blank line above it");
 */
#define nl() putchar('\n')

/* ===========================================================================
 * Section 2: Simple input
 * ========================================================================= */

/* Discards the rest of the current input line, including the newline.
 * Called internally after scanf-style reads so that a stray '\n' never
 * leaks into the next input_*() call. */
EC_INLINE void ec_flush_input_line(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { /* discard */ }
}

/**
 * input_int() — reads one line from stdin and parses it as an int.
 * Returns 0 if the line could not be parsed as a number.
 */
EC_INLINE int input_int(void) {
    char buf[256];
    if (!fgets(buf, sizeof(buf), stdin)) return 0;
    return (int)strtol(buf, NULL, 10);
}

/**
 * input_float() — reads one line from stdin and parses it as a float.
 * Returns 0.0f if the line could not be parsed as a number.
 */
EC_INLINE float input_float(void) {
    char buf[256];
    if (!fgets(buf, sizeof(buf), stdin)) return 0.0f;
    return strtof(buf, NULL);
}

/**
 * input_double() — reads one line from stdin and parses it as a double.
 * Returns 0.0 if the line could not be parsed as a number.
 */
EC_INLINE double input_double(void) {
    char buf[256];
    if (!fgets(buf, sizeof(buf), stdin)) return 0.0;
    return strtod(buf, NULL);
}

/**
 * input_char() — reads and returns the first non-whitespace character
 * typed by the user. Returns '\0' on EOF.
 */
EC_INLINE char input_char(void) {
    int c;
    do { c = getchar(); } while (c == '\n' || c == '\r' || c == ' ' || c == '\t');
    if (c == EOF) return '\0';
    ec_flush_input_line();
    return (char)c;
}

/**
 * input_string(buffer, size) — reads one line of text from stdin into
 * `buffer`, which must be at least `size` bytes. The trailing newline
 * (if any) is stripped. The result is always null-terminated.
 *
 * Example:
 *     char name[100];
 *     input_string(name, sizeof(name));
 */
EC_INLINE void input_string(char *buffer, size_t size) {
    if (size == 0) return;
    if (!fgets(buffer, (int)size, stdin)) {
        buffer[0] = '\0';
        return;
    }
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';
}

/**
 * scan_flush() — discards the rest of the current input line. Call this
 * after your last scan() call if you plan to follow it with
 * input_string()/input_char(), so a leftover '\n' from scan() doesn't
 * show up as an empty read (see the note on scan() below).
 */
EC_INLINE void scan_flush(void) {
    ec_flush_input_line();
}

/* ec_scan_* — internal, type-specific readers behind scan(). Each one
 * uses a fixed conversion specifier internally; the specifier is never
 * exposed to the caller, matching the rest of EaCy's "no format
 * strings" input API. */
EC_INLINE void ec_scan_int(int *out)       { if (scanf("%d", out) != 1) *out = 0; }
EC_INLINE void ec_scan_long(long *out)     { if (scanf("%ld", out) != 1) *out = 0; }
EC_INLINE void ec_scan_float(float *out)   { if (scanf("%f", out) != 1) *out = 0.0f; }
EC_INLINE void ec_scan_double(double *out) { if (scanf("%lf", out) != 1) *out = 0.0; }
EC_INLINE void ec_scan_bool(bool *out) {
    int tmp;
    *out = (scanf("%d", &tmp) == 1) && (tmp != 0);
}

/* Does nothing between scan() arguments — scanf's numeric conversions
 * already skip leading whitespace (spaces, tabs, newlines) on their
 * own, so no explicit separator is needed the way print() needs one. */
EC_INLINE void ec_scan_sep(void) { /* no-op */ }

/* _Generic dispatch on the *pointee* type, so scan(&x) picks the right
 * reader for whatever `x` is. Only numeric/bool pointer types are
 * supported — see the doc comment on scan() for why char* is excluded. */
#define ec_scan_one(x) _Generic((x), \
    int*:    ec_scan_int,    \
    long*:   ec_scan_long,   \
    float*:  ec_scan_float,  \
    double*: ec_scan_double, \
    bool*:   ec_scan_bool    \
)(x)

/**
 * scan(...) — a scanf() replacement with no format strings. Pass 1 to 8
 * pointers to variables and each one is filled in from stdin, with its
 * type detected automatically at compile time (just like print()).
 *
 * Supported pointer types: int*, long*, float*, double*, bool*.
 * Values may be separated by spaces or newlines, exactly like scanf.
 *
 * Example:
 *     int x, y;
 *     print("Enter two numbers: ");
 *     scan(&x, &y);
 *     println("Sum:", x + y);
 *
 * `char*` is intentionally NOT supported here: scanf-style "%s" reads
 * are a classic buffer-overflow source because they don't know your
 * buffer's size. Use input_string(buffer, size) for text, and
 * input_char() for a single character — both are bounds-safe.
 *
 * Note: like real scanf, scan() leaves the trailing '\n' in the input
 * buffer after reading the last number on a line. If you call
 * input_string() or input_char() right after a scan(), call
 * scan_flush() first to discard that leftover newline.
 */
#define scan(...) \
    do { EC_FOR_EACH(ec_scan_one, ec_scan_sep, __VA_ARGS__); } while (0)

/* ===========================================================================
 * Section 3: Random numbers
 * ========================================================================= */

/**
 * random_seed() — seeds the random number generator using the current
 * time. Call this once near the start of your program if you want
 * different results on every run. If you never call it, the sequence of
 * random_*() values will be the same every run (useful for tests).
 */
EC_INLINE void random_seed(void) {
    srand((unsigned int)time(NULL));
}

/**
 * random_int(min, max) — returns a random integer in the inclusive
 * range [min, max]. `min` must be <= `max`.
 *
 * Note: uses rand() internally, so it is fine for games/demos but is
 * not cryptographically secure and has a very slight modulo bias.
 */
EC_INLINE int random_int(int min, int max) {
    if (min > max) { int t = min; min = max; max = t; }
    return min + (rand() % (max - min + 1));
}

/**
 * random_float(min, max) — returns a random float uniformly distributed
 * in the range [min, max].
 */
EC_INLINE float random_float(float min, float max) {
    float t = (float)rand() / (float)RAND_MAX;
    return min + t * (max - min);
}

/* ===========================================================================
 * Section 4: Utility macros — repeat / foreach / for_range
 * ========================================================================= */

/**
 * repeat(n) — runs the following block n times.
 *
 * Example:
 *     repeat(10) {
 *         println("Hello");
 *     }
 *
 * Safe to nest: each use gets its own uniquely-named loop counter.
 */
#define repeat(n) \
    for (long EC_CONCAT(ec_repeat_i_, __LINE__) = 0; \
         EC_CONCAT(ec_repeat_i_, __LINE__) < (long)(n); \
         EC_CONCAT(ec_repeat_i_, __LINE__)++)

/**
 * foreach(index, array) — iterates `index` from 0 to the number of
 * elements in `array` (computed via sizeof, so this only works on real
 * arrays, not decayed pointers).
 *
 * Example:
 *     int nums[5] = {1, 2, 3, 4, 5};
 *     foreach(i, nums) {
 *         println(nums[i]);
 *     }
 */
#define foreach(index, array) \
    for (size_t index = 0; index < (sizeof(array) / sizeof((array)[0])); index++)

/**
 * for_range(i, start, end) — iterates `i` over [start, end), i.e. start
 * is included and end is excluded, like Python's range().
 *
 * Example:
 *     for_range(i, 0, 5) {
 *         println(i);   // prints 0 1 2 3 4
 *     }
 */
#define for_range(i, start, end) \
    for (long i = (long)(start); i < (long)(end); i++)

/* ===========================================================================
 * Section 5: Math helpers
 * ========================================================================= */

/* NOTE on these macros: like most short expression-style macros in C,
 * `a` and `b` may be evaluated more than once. Avoid passing expressions
 * with side effects (e.g. min(x++, y)) to them. */

#ifndef min
/** min(a, b) — the smaller of a and b. */
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
/** max(a, b) — the larger of a and b. */
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

/** clamp(x, lo, hi) — restricts x to the range [lo, hi]. */
#define clamp(x, lo, hi) ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))

/** lerp(a, b, t) — linear interpolation between a and b at t (0..1). */
#define lerp(a, b, t) ((a) + ((b) - (a)) * (t))

/**
 * swap(a, b, type) — swaps the values of two variables of the given
 * type. A type argument is required because standard C11 has no
 * portable `typeof` (it is a GNU/C23 extension, and EaCy targets plain
 * C11 + MSVC).
 *
 * Example:
 *     int x = 1, y = 2;
 *     swap(x, y, int);
 */
#define swap(a, b, type) \
    do { type ec_swap_tmp_ = (a); (a) = (b); (b) = ec_swap_tmp_; } while (0)

/* ===========================================================================
 * Section 6: String helpers
 * ========================================================================= */

/**
 * starts_with(str, prefix) — returns true if `str` begins with `prefix`.
 */
EC_INLINE bool starts_with(const char *restrict str, const char *restrict prefix) {
    if (!str || !prefix) return false;
    size_t prefix_len = strlen(prefix);
    return strncmp(str, prefix, prefix_len) == 0;
}

/**
 * ends_with(str, suffix) — returns true if `str` ends with `suffix`.
 */
EC_INLINE bool ends_with(const char *restrict str, const char *restrict suffix) {
    if (!str || !suffix) return false;
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (suffix_len > str_len) return false;
    return strcmp(str + (str_len - suffix_len), suffix) == 0;
}
/**
 * contains(str, needle) — returns true if `needle` occurs anywhere
 * inside `str`.
 */
EC_INLINE bool contains(const char *restrict str, const char *restrict needle) {
    if (!needle || !*needle) return needle ? true : false;
    if (!str) return false;
    return strstr(str, needle) != NULL;
}

/**
 * equals(a, b) — returns true if two strings have identical contents.
 * (Just a readable wrapper around strcmp.)
 */
EC_INLINE bool equals(const char *restrict a, const char *restrict b) {
    return strcmp(a, b) == 0;
}

/**
 * trim(str) — trims leading and trailing whitespace from `str` IN
 * PLACE and returns a pointer to the trimmed string.
 *
 * Because leading whitespace is skipped rather than moved, the returned
 * pointer may point partway into the original buffer — don't free()
 * the returned pointer directly, free the original buffer instead.
 *
 * Example:
 *     char buf[] = "   hi there   ";
 *     char *t = trim(buf);   // t -> "hi there"
 */
EC_INLINE char *trim(char *restrict str) {
    if (!str) return str;
    /* Skip leading whitespace. */
    while (*str && isspace((unsigned char)*str)) str++;
    /* If we landed on the null terminator, the string is all whitespace. */
    if (*str == '\0') return str;
    /* Trim trailing whitespace. */
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

/**
 * lowercase(str) — converts `str` to lowercase IN PLACE.
 */
EC_INLINE void lowercase(char *restrict str) {
    for (; *str; str++) *str = (char)tolower((unsigned char)*str);
}

/**
 * uppercase(str) — converts `str` to uppercase IN PLACE.
 */
EC_INLINE void uppercase(char *restrict str) {
    for (; *str; str++) *str = (char)toupper((unsigned char)*str);
}

/* ===========================================================================
 * Section 7: File helpers
 * ========================================================================= */

/**
 * read_text_file(path) — reads an entire text file into a newly
 * allocated, null-terminated buffer and returns it, or NULL on failure
 * (file not found, out of memory, etc).
 *
 * HEAP ALLOCATION: this function calls malloc(). The caller owns the
 * returned pointer and must free() it (or pass it to ec_free()) when
 * done.
 *
 * Example:
 *     char *text = read_text_file("notes.txt");
 *     if (text) { println(text); free(text); }
 */
EC_INLINE char *read_text_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long size = ftell(f);
    if (size < 0) { fclose(f); return NULL; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return NULL; }

    char *buffer = (char *)malloc((size_t)size + 1);
    if (!buffer) { fclose(f); return NULL; }

    size_t read = fread(buffer, 1, (size_t)size, f);
    buffer[read] = '\0';
    fclose(f);
    return buffer;
}

/**
 * write_text_file(path, content) — writes `content` to `path`,
 * overwriting any existing file. Returns true on success.
 */
EC_INLINE bool write_text_file(const char *path, const char *content) {
    FILE *f = fopen(path, "wb");
    if (!f) return false;
    size_t len = strlen(content);
    size_t written = fwrite(content, 1, len, f);
    fclose(f);
    return written == len;
}

/**
 * append_text_file(path, content) — appends `content` to the end of
 * `path`, creating the file if it does not already exist. Returns true
 * on success.
 */
EC_INLINE bool append_text_file(const char *path, const char *content) {
    FILE *f = fopen(path, "ab");
    if (!f) return false;
    size_t len = strlen(content);
    size_t written = fwrite(content, 1, len, f);
    fclose(f);
    return written == len;
}

/* ===========================================================================
 * Section 7b: Command runner
 * ========================================================================= */

#if defined(EC_PLATFORM_WINDOWS)
    #define EC_POPEN_  _popen
    #define EC_PCLOSE_ _pclose
#else
    #define EC_POPEN_  popen
    #define EC_PCLOSE_ pclose
#endif

/**
 * cmd_run(command) - runs `command` through the host shell and returns its
 * exit code. Returns -1 if the shell could not be started.
 *
 * Example:
 *     int code = cmd_run("gcc main.c -o main");
 *     if (code == 0) println("build ok");
 */
EC_INLINE int cmd_run(const char *command) {
    if (!command) return -1;
    int status = system(command);
    if (status == -1) return -1;
#if defined(EC_PLATFORM_POSIX)
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    return status;
#else
    return status;
#endif
}

/**
 * cmd_ok(command) - convenience wrapper around cmd_run().
 */
EC_INLINE bool cmd_ok(const char *command) {
    return cmd_run(command) == 0;
}

/**
 * cmd_capture(command) - runs `command` and captures stdout into a newly
 * allocated, null-terminated string. Returns NULL if the command could not be
 * started or memory allocation fails.
 *
 * HEAP ALLOCATION: the caller owns the returned pointer and must free() it.
 */
EC_INLINE char *cmd_capture(const char *command) {
    if (!command) return NULL;

    FILE *pipe = EC_POPEN_(command, "r");
    if (!pipe) return NULL;

    size_t len = 0;
    size_t cap = 256;
    char *out = (char *)malloc(cap);
    if (!out) {
        EC_PCLOSE_(pipe);
        return NULL;
    }
    out[0] = '\0';

    char chunk[256];
    while (fgets(chunk, sizeof(chunk), pipe)) {
        size_t n = strlen(chunk);
        if (len + n + 1 > cap) {
            size_t new_cap = cap;
            while (len + n + 1 > new_cap) new_cap *= 2;
            char *grown = (char *)realloc(out, new_cap);
            if (!grown) {
                free(out);
                EC_PCLOSE_(pipe);
                return NULL;
            }
            out = grown;
            cap = new_cap;
        }
        memcpy(out + len, chunk, n + 1);
        len += n;
    }

    EC_PCLOSE_(pipe);
    return out;
}

/* ===========================================================================
 * Section 8: Time
 * ========================================================================= */

/**
 * sleep_ms(milliseconds) — pauses the current thread for approximately
 * the given number of milliseconds.
 */
EC_INLINE void sleep_ms(unsigned int milliseconds) {
#if defined(EC_PLATFORM_WINDOWS)
    Sleep(milliseconds);
#else
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (long)(milliseconds % 1000) * 1000000L;
    nanosleep(&ts, NULL);
#endif
}

/**
 * current_time_ms() — returns a monotonically increasing millisecond
 * timestamp. The absolute value is meaningless (it is not wall-clock
 * time) — only differences between two calls are meaningful, which
 * makes it ideal for measuring elapsed time.
 *
 * Example:
 *     long long start = current_time_ms();
 *     do_work();
 *     println("Took", (long)(current_time_ms() - start));
 */
EC_INLINE long long current_time_ms(void) {
#if defined(EC_PLATFORM_WINDOWS)
    return (long long)GetTickCount64();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + (long long)ts.tv_nsec / 1000000LL;
#endif
}

/* ===========================================================================
 * Section 9: Colors
 * ========================================================================= */

/**
 * ec_init_colors() — enables ANSI escape-code interpretation in the
 * current console. On Linux/macOS terminals this is a no-op (ANSI codes
 * already work). On Windows it turns on Virtual Terminal Processing for
 * stdout, which is required on older versions of the Windows console
 * (Windows 10 and later usually support this; calling it is always
 * safe and cheap regardless).
 *
 * Call this once near the start of main() if you plan to use the
 * print_red()/print_green()/print_blue()/reset_color() macros below.
 */
EC_INLINE void ec_init_colors(void) {
#if defined(EC_PLATFORM_WINDOWS)
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h == INVALID_HANDLE_VALUE) return;
    DWORD mode = 0;
    if (!GetConsoleMode(h, &mode)) return;
    SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
    /* POSIX terminals support ANSI escapes natively — nothing to do. */
}

/** print_red() — switches subsequent output to red text. */
#define print_red()     fputs("\x1b[31m", stdout)
/** print_green() — switches subsequent output to green text. */
#define print_green()   fputs("\x1b[32m", stdout)
/** print_blue() — switches subsequent output to blue text. */
#define print_blue()    fputs("\x1b[34m", stdout)
/** reset_color() — restores the terminal's default text color. */
#define reset_color()   fputs("\x1b[0m", stdout)

/* ===========================================================================
 * Section 10: Assertions
 * ========================================================================= */

/**
 * assert_msg(condition, message) — if `condition` is false, prints
 * `message` (along with the file and line number) to stderr and aborts
 * the program. Unlike the standard `assert()`, this is NOT disabled
 * when NDEBUG is defined — it is meant for explicit, intentional checks
 * rather than debug-only sanity checks.
 *
 * Example:
 *     assert_msg(age >= 0, "age cannot be negative");
 */
#define assert_msg(condition, message) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "Assertion failed: %s\n  at %s:%d\n  condition: %s\n", \
                    (message), __FILE__, __LINE__, #condition); \
            abort(); \
        } \
    } while (0)

/* ===========================================================================
 * Section 11: Memory helpers
 * ========================================================================= */

/* Define EC_DEBUG_MEMORY before including eacy.h to enable a tiny
 * global allocation counter. This is a simple debugging aid, not a leak
 * detector: it counts how many blocks are currently allocated via
 * ec_malloc()/ec_calloc()/ec_free(), so you can sanity-check that every
 * allocation was eventually freed. */
#if defined(EC_DEBUG_MEMORY)
static long ec_alloc_count = 0;
#endif

/**
 * ec_malloc(size) — like malloc(), but when EC_DEBUG_MEMORY is defined,
 * also increments a global live-allocation counter.
 *
 * HEAP ALLOCATION: this function allocates memory that must eventually
 * be released with ec_free() (or plain free()).
 */
EC_INLINE void *ec_malloc(size_t size) {
    void *ptr = malloc(size);
#if defined(EC_DEBUG_MEMORY)
    if (ptr) ec_alloc_count++;
#endif
    return ptr;
}

/**
 * ec_calloc(count, size) — like calloc(), but when EC_DEBUG_MEMORY is
 * defined, also increments a global live-allocation counter.
 *
 * HEAP ALLOCATION: this function allocates memory that must eventually
 * be released with ec_free() (or plain free()).
 */
EC_INLINE void *ec_calloc(size_t count, size_t size) {
    void *ptr = calloc(count, size);
#if defined(EC_DEBUG_MEMORY)
    if (ptr) ec_alloc_count++;
#endif
    return ptr;
}

/**
 * ec_free(ptr) — like free(), but when EC_DEBUG_MEMORY is defined, also
 * decrements the global live-allocation counter. Safe to call with
 * NULL, same as free().
 */
EC_INLINE void ec_free(void *ptr) {
#if defined(EC_DEBUG_MEMORY)
    if (ptr) ec_alloc_count--;
#endif
    free(ptr);
}

#if defined(EC_DEBUG_MEMORY)
/**
 * ec_debug_alloc_count() — returns the number of blocks currently
 * allocated via ec_malloc()/ec_calloc() that have not yet been released
 * via ec_free(). Only available when EC_DEBUG_MEMORY is defined before
 * this header is included.
 */
EC_INLINE long ec_debug_alloc_count(void) {
    return ec_alloc_count;
}
#endif

/* ===========================================================================
 * Section 12: Dynamic arrays (stretchy buffers)
 * =========================================================================
 *
 * Type-safe, growable arrays that work with any pointer type. Each array
 * stores a small header (length + capacity) immediately before the
 * user-visible data, so da_len() / da_cap() are O(1) and da_push()
 * amortises to O(1) per element.
 *
 * Rules of thumb:
 *   - ALWAYS initialise with da_init(arr) or T *arr = NULL.
 *   - ONLY pass lvalues (named variables) to macros that mutate the array
 *     (da_push, da_pop, da_insert, da_remove, da_free, da_clear,
 *      da_reserve).  Passing a temporary or expression is undefined.
 *   - NEVER mix da_* arrays with stack arrays or manual malloc.  The
 *     pointer passed to da_push MUST be NULL or a previous da_* result.
 *   - Macros evaluate `arr` multiple times — avoid side-effecting
 *     expressions like da_push(arr, getchar()).
 *
 * Example:
 *     int *nums = NULL;
 *     da_push(nums, 10);
 *     da_push(nums, 20);
 *     da_push(nums, 30);
 *     da_for(i, nums) println(nums[i]);   // 10 20 30
 *     println("len:", (long)da_len(nums)); // 3
 *     da_free(nums);
 */

typedef struct {
    size_t len;   /* number of live elements */
    size_t cap;   /* allocated capacity (element count, not bytes) */
} ec_da_header;

/* Internal: given a valid data pointer, returns a pointer to its header. */
#define ec_da_hdr_(arr)  (((ec_da_header *)(arr)) - 1)

/**
 * da_init(arr) — initialises a dynamic array to empty.  Identical to
 * writing `arr = NULL` by hand, but makes intent explicit.
 */
#define da_init(arr)     ((arr) = NULL)

/**
 * da_len(arr) — returns the number of live elements.  Returns 0 for a
 * NULL array (which is considered empty).
 */
#define da_len(arr)      ((arr) ? ec_da_hdr_(arr)->len : 0)

/**
 * da_cap(arr) — returns the currently allocated capacity (max elements
 * before the next realloc).  Returns 0 for a NULL array.
 */
#define da_cap(arr)      ((arr) ? ec_da_hdr_(arr)->cap : 0)

/**
 * da_empty(arr) — returns true if the array is empty (NULL or zero
 * length).  Equivalent to da_len(arr) == 0.
 */
#define da_empty(arr)   (da_len(arr) == 0)

/**
 * da_front(arr) — returns the first element.  Undefined if the array
 * is empty.
 */
#define da_front(arr)    ((arr)[0])

/**
 * da_back(arr) — returns the last element.  Undefined if the array is
 * empty — callers should guard with da_len(arr) > 0.
 */
#define da_back(arr)     ((arr)[ec_da_hdr_(arr)->len - 1])

/** @deprecated alias for da_back — use da_back instead. */
#define da_last(arr)     da_back(arr)

/**
 * da_clear(arr) — resets the length to 0 but keeps the allocated
 * capacity.  Subsequent da_push() calls reuse the existing buffer.
 */
#define da_clear(arr)    do { if (arr) ec_da_hdr_(arr)->len = 0; } while (0)

/**
 * da_free(arr) — frees the entire array and sets the pointer to NULL.
 * Safe to call on an already-NULL array (no-op).
 */
#define da_free(arr) do { \
    if (arr) { ec_free(ec_da_hdr_(arr)); (arr) = NULL; } \
} while (0)

/**
 * da_pop(arr) — removes and returns the last element.  Undefined if the
 * array is empty — callers should guard with da_len(arr) > 0.
 */
#define da_pop(arr)      ((arr)[--ec_da_hdr_(arr)->len])

/* Internal: ensures the array has at least `ec_da_need_` capacity.
 * Grows by doubling from a minimum of 8, or to exactly `ec_da_need_`
 * if that's larger.  On allocation failure the macro `break`s out of
 * the caller's do-while, leaving `arr` untouched. */
#define ec_da_grow_(arr, ec_da_need_) do { \
    size_t ec_da_cur_ = (arr) ? ec_da_hdr_(arr)->cap : 0; \
    if (ec_da_cur_ < (size_t)(ec_da_need_)) { \
        size_t ec_da_new_ = ec_da_cur_ ? ec_da_cur_ * 2 : 8; \
        if (ec_da_new_ < (size_t)(ec_da_need_)) \
            ec_da_new_ = (size_t)(ec_da_need_); \
        size_t ec_da_sz_ = sizeof(ec_da_header) + ec_da_new_ * sizeof(*(arr)); \
        ec_da_header *ec_da_hdr_ = (ec_da_header *)realloc( \
            (arr) ? ec_da_hdr_(arr) : NULL, ec_da_sz_); \
        if (!ec_da_hdr_) break; \
        if (!(arr)) ec_da_hdr_->len = 0; \
        ec_da_hdr_->cap = ec_da_new_; \
        (arr) = (void *)(ec_da_hdr_ + 1); \
    } \
} while (0)

/**
 * da_push(arr, item) — appends `item` to the end of the array,
 * automatically growing capacity when needed.  On allocation failure
 * the array is left unchanged (and the item is silently dropped).
 *
 * Example:
 *     double *vals = NULL;
 *     da_push(vals, 3.14);
 *     da_push(vals, 2.71);
 */
#define da_push(arr, item) do { \
    ec_da_grow_(arr, ((arr) ? ec_da_hdr_(arr)->len + 1 : 1)); \
    if (!(arr)) break; \
    ec_da_header *ec_da_h_ = ec_da_hdr_(arr); \
    (arr)[ec_da_h_->len++] = (item); \
} while (0)

/**
 * da_push_many(arr, src, count) — appends `count` elements from `src`
 * (a C array or pointer) in one shot.  Uses a single capacity check
 * and memcpy, so this is MUCH faster than a da_push loop for bulk
 * insertion.  `src` and `arr` MUST NOT overlap.
 *
 * Example:
 *     int  more[] = {4, 5, 6};
 *     da_push_many(nums, more, 3);
 */
#define da_push_many(arr, src, count) do { \
    size_t ec_da_n_ = (size_t)(count); \
    if (ec_da_n_ == 0) break; \
    ec_da_grow_(arr, ((arr) ? ec_da_hdr_(arr)->len + ec_da_n_ : ec_da_n_)); \
    if (!(arr)) break; \
    ec_da_header *ec_da_h_ = ec_da_hdr_(arr); \
    memcpy(&(arr)[ec_da_h_->len], (src), ec_da_n_ * sizeof(*(arr))); \
    ec_da_h_->len += ec_da_n_; \
} while (0)

/**
 * da_reserve(arr, n) — ensures the array has room for at least `n`
 * elements without further reallocation.  A NULL array is allocated
 * fresh.  On failure the array is left unchanged.
 */
#define da_reserve(arr, n)  ec_da_grow_(arr, (size_t)(n))

/**
 * da_insert(arr, i, item) — inserts `item` at index `i`, shifting
 * existing elements to the right.  `i` may be da_len(arr) (equivalent
 * to da_push).  Grows automatically.
 */
#define da_insert(arr, i, item) do { \
    size_t ec_da_idx_ = (size_t)(i); \
    ec_da_grow_(arr, ((arr) ? ec_da_hdr_(arr)->len + 1 : 1)); \
    if (!(arr)) break; \
    ec_da_header *ec_da_h_ = ec_da_hdr_(arr); \
    if (ec_da_idx_ < ec_da_h_->len) { \
        memmove(&(arr)[ec_da_idx_ + 1], &(arr)[ec_da_idx_], \
                (ec_da_h_->len - ec_da_idx_) * sizeof(*(arr))); \
    } \
    (arr)[ec_da_idx_] = (item); \
    ec_da_h_->len++; \
} while (0)

/**
 * da_remove(arr, i) — removes the element at index `i`, shifting
 * subsequent elements left.  Undefined if `i` is out of range.
 */
#define da_remove(arr, i) do { \
    size_t ec_da_idx_ = (size_t)(i); \
    ec_da_header *ec_da_h_ = ec_da_hdr_(arr); \
    if (ec_da_idx_ < ec_da_h_->len - 1) { \
        memmove(&(arr)[ec_da_idx_], &(arr)[ec_da_idx_ + 1], \
                (ec_da_h_->len - ec_da_idx_ - 1) * sizeof(*(arr))); \
    } \
    ec_da_h_->len--; \
} while (0)

/**
 * da_resize(arr, n) — changes the array length to `n`.  If `n` is
 * larger than the current length, new elements are zero-initialised.
 * If `n` is smaller, the array is truncated.  Grows capacity
 * automatically when needed.
 */
#define da_resize(arr, n) do { \
    size_t ec_da_n_ = (size_t)(n); \
    size_t ec_da_cur_ = da_len(arr); \
    if (ec_da_n_ > ec_da_cur_) { \
        ec_da_grow_(arr, ec_da_n_); \
        if (!(arr)) break; \
        ec_da_header *ec_da_h_ = ec_da_hdr_(arr); \
        memset(&(arr)[ec_da_h_->len], 0, \
               (ec_da_n_ - ec_da_h_->len) * sizeof(*(arr))); \
        ec_da_h_->len = ec_da_n_; \
    } else if (ec_da_n_ < ec_da_cur_ && (arr)) { \
        ec_da_hdr_(arr)->len = ec_da_n_; \
    } \
} while (0)

/* Internal: copies a dynamic array's header + data.  Returns a pointer
 * to the data portion (past the header), or NULL on failure. */
EC_INLINE void *ec_da_copy_impl_(const void *arr, size_t elem_size) {
    if (!arr) return NULL;
    const ec_da_header *src = ((const ec_da_header *)arr) - 1;
    size_t total = sizeof(ec_da_header) + src->cap * elem_size;
    ec_da_header *dst = (ec_da_header *)malloc(total);
    if (!dst) return NULL;
    memcpy(dst, src, total);
    return dst + 1;
}

/**
 * da_copy(arr, type) — returns a deep copy of the dynamic array.
 * `type` must be the element type (e.g. int, float, MyStruct).
 * Returns NULL on allocation failure or if `arr` is NULL.
 *
 * HEAP ALLOCATION: the returned copy must be freed with da_free().
 *
 * Example:
 *     int *clone = da_copy(nums, int);
 *     da_free(clone);
 */
#define da_copy(arr, type) ((type *)ec_da_copy_impl_((arr), sizeof(type)))

/* --- Built-in comparators for da_sort --- */
EC_INLINE int ec_cmp_int(const void *a, const void *b) {
    int ia = *(const int *)a, ib = *(const int *)b;
    return (ia > ib) - (ia < ib);
}
EC_INLINE int ec_cmp_int_desc(const void *a, const void *b)   { return ec_cmp_int(b, a); }

EC_INLINE int ec_cmp_long(const void *a, const void *b) {
    long la = *(const long *)a, lb = *(const long *)b;
    return (la > lb) - (la < lb);
}
EC_INLINE int ec_cmp_float(const void *a, const void *b) {
    float fa = *(const float *)a, fb = *(const float *)b;
    if (fa != fa || fb != fb) {
        /* At least one is NaN: push NaN to the end (treat as > all). */
        return (fa != fa) ? ((fb != fb) ? 0 : 1) : -1;
    }
    return (fa > fb) - (fa < fb);
}
EC_INLINE int ec_cmp_float_desc(const void *a, const void *b) { return ec_cmp_float(b, a); }

EC_INLINE int ec_cmp_double(const void *a, const void *b) {
    double da = *(const double *)a, db = *(const double *)b;
    if (da != da || db != db) {
        return (da != da) ? ((db != db) ? 0 : 1) : -1;
    }
    return (da > db) - (da < db);
}
EC_INLINE int ec_cmp_double_desc(const void *a, const void *b){ return ec_cmp_double(b, a); }

EC_INLINE int ec_cmp_str(const void *a, const void *b) {
    const char *sa = *(const char *const *)a;
    const char *sb = *(const char *const *)b;
    return strcmp(sa, sb);
}
EC_INLINE int ec_cmp_str_desc(const void *a, const void *b) { return ec_cmp_str(b, a); }

/**
 * da_sort(arr, cmp) — sorts the array in place using `cmp` as the
 * comparison function (same signature as qsort's comparator).
 * Use the built-in ec_cmp_* functions or write your own.
 *
 * Example:
 *     da_sort(nums, ec_cmp_int);          // ascending
 *     da_sort(nums, ec_cmp_int_desc);     // descending
 *     da_sort(names, ec_cmp_str);         // alphabetical
 */
#define da_sort(arr, cmp) \
    qsort((arr), da_len(arr), sizeof(*(arr)), (cmp))

/**
 * da_for(i, arr) — iterates `i` (a size_t) from 0 to da_len(arr)-1.
 * The array MUST NOT be modified in ways that change its length during
 * iteration (push/pop/insert/remove/clear/free).
 *
 * Example:
 *     da_for(i, names) println(names[i]);
 */
#define da_for(i, arr) \
    for (size_t i = 0, ec_da_end_ = da_len(arr); i < ec_da_end_; i++)

/* ===========================================================================
 * Section 13: Arena (bump / linear) allocator
 * =========================================================================
 *
 * An arena is a simple bump-pointer allocator: you pre-allocate a big
 * block, then "allocate" from it by advancing a cursor.  Individual
 * allocations CANNOT be freed — the whole arena is freed or reset at
 * once.  This makes it ideal for:
 *   - Per-frame allocations in games
 *   - Temporary scratch data during a single request / pass
 *   - Avoiding malloc churn when many small allocations share a lifetime
 *
 * Allocations are 8-byte aligned by default.
 *
 * Example:
 *     ec_arena arena = ec_arena_new(1024 * 1024);   // 1 MiB
 *     int *nums = ec_arena_alloc(&arena, 100 * sizeof(int));
 *     char *str = ec_arena_alloc(&arena, 256);
 *     // ... use nums and str ...
 *     ec_arena_reset(&arena);   // all arena memory reusable
 *     // ... allocate more ...
 *     ec_arena_free(&arena);    // done
 */

typedef struct {
    unsigned char *buf;    /* underlying block */
    size_t         cap;    /* total bytes in buf */
    size_t         offset; /* next free byte */
} ec_arena;

/**
 * ec_arena_new(capacity) — creates an arena with the given initial
 * capacity in bytes.  Returns an arena in a valid-but-empty state
 * (buf == NULL) if the allocation fails.  Check arena.buf against NULL
 * before using.
 *
 * HEAP ALLOCATION: the returned arena owns its backing buffer, which
 * must be released with ec_arena_free().
 */
EC_INLINE ec_arena ec_arena_new(size_t capacity) {
    ec_arena a;
    a.buf    = (unsigned char *)malloc(capacity);
    a.cap    = a.buf ? capacity : 0;
    a.offset = 0;
    return a;
}

/**
 * ec_arena_alloc(arena, size) — bumps out `size` bytes and returns a
 * pointer to them (8-byte aligned).  Returns NULL if the arena does
 * not have enough remaining space.  The memory is UNINITIALISED (like
 * malloc).
 */
EC_INLINE void *ec_arena_alloc(ec_arena *a, size_t size) {
    if (!a->buf || a->offset + size > a->cap) return NULL;
    void *ptr = a->buf + a->offset;
    a->offset += size;
    /* 8-byte alignment for the next allocation */
    a->offset = (a->offset + 7) & ~(size_t)7;
    return ptr;
}

/**
 * ec_arena_alloc_zero(arena, size) — like ec_arena_alloc(), but fills
 * the returned memory with zeros (like calloc).
 */
EC_INLINE void *ec_arena_alloc_zero(ec_arena *a, size_t size) {
    void *p = ec_arena_alloc(a, size);
    if (p) memset(p, 0, size);
    return p;
}

/**
 * ec_arena_reset(arena) — resets the bump pointer to the beginning.
 * The backing buffer is reused; all previous allocations from this arena
 * become logically invalid (the memory is still there but will be
 * overwritten by future ec_arena_alloc calls).
 */
EC_INLINE void ec_arena_reset(ec_arena *a) {
    a->offset = 0;
}

/**
 * ec_arena_free(arena) — frees the backing buffer and zeros out the
 * arena struct.  Safe to call multiple times.
 */
EC_INLINE void ec_arena_free(ec_arena *a) {
    free(a->buf);
    a->buf    = NULL;
    a->cap    = 0;
    a->offset = 0;
}

/**
 * ec_arena_remaining(arena) — returns the number of bytes still
 * available before the arena is exhausted.
 */
EC_INLINE size_t ec_arena_remaining(const ec_arena *a) {
    if (!a->buf || a->offset >= a->cap) return 0;
    return a->cap - a->offset;
}

/* ===========================================================================
 * Section 14: Simple CLI argument parsing
 * =========================================================================
 *
 * A no-dependency argument parser for command-line tools.  Create an
 * ec_args struct once at the top of main() with ec_args_new(argc, argv),
 * then pass a pointer to it for every query.  Supports:
 *   - Flags: --verbose, -v, /verbose (detected as present/absent)
 *   - Key-value: --output out.txt, -o out.txt (value follows the flag)
 *   - Positional: everything that isn't a recognised flag/value pair
 *
 * No allocations, no copies — pointers into the original argv.
 *
 * Example:
 *     int main(int argc, char **argv) {
 *         ec_args args = ec_args_new(argc, argv);
 *         if (ec_args_has(&args, "--help"))   { print_usage(); return 0; }
 *         bool verbose = ec_args_has(&args, "--verbose") || ec_args_has(&args, "-v");
 *         const char *out = ec_args_val(&args, "--output");  // NULL if absent
 *         for (int i = 0; i < ec_args_pos_count(&args); i++)
 *             println("  positional:", ec_args_pos(&args, i));
 *     }
 */

typedef struct {
    int    argc;
    char **argv;
} ec_args;

/**
 * ec_args_new(argc, argv) — captures argc/argv for the query functions
 * below.  Call exactly once, at the top of main().  The returned struct
 * is safe to pass across translation units (no hidden static state).
 */
EC_INLINE ec_args ec_args_new(int argc, char **argv) {
    ec_args a = {argc, argv};
    return a;
}

/**
 * ec_args_has(args, flag) — returns true if `flag` appears anywhere in
 * the argument list.  `flag` should include leading dashes (e.g.
 * "--help", "-v").  The match is an exact string comparison.
 */
EC_INLINE bool ec_args_has(const ec_args *a, const char *flag) {
    for (int i = 1; i < a->argc; i++) {
        if (a->argv[i] && strcmp(a->argv[i], flag) == 0)
            return true;
    }
    return false;
}

/**
 * ec_args_val(args, flag) — returns the string immediately following
 * `flag` in argv, or NULL if the flag is absent or is the last
 * argument.  Useful for "--output out.txt" or "-o out.txt".
 */
EC_INLINE const char *ec_args_val(const ec_args *a, const char *flag) {
    for (int i = 1; i < a->argc - 1; i++) {
        if (a->argv[i] && strcmp(a->argv[i], flag) == 0)
            return a->argv[i + 1];
    }
    return NULL;
}

/**
 * ec_args_pos_count(args) — returns the number of arguments excluding
 * argv[0] (the program name).  All arguments — flags, flag values, and
 * positional — are counted.  This is a simple arg count; if you need to
 * distinguish positional from flags, filter argv manually.
 */
EC_INLINE int ec_args_pos_count(const ec_args *a) {
    return a->argc > 0 ? a->argc - 1 : 0;
}

/**
 * ec_args_pos(args, i) — returns the i-th argument after the program
 * name (0-indexed: i=0 gives argv[1]), or NULL if i is out of range.
 * All arguments — flags included — are accessible via this function.
 */
EC_INLINE const char *ec_args_pos(const ec_args *a, int i) {
    int idx = i + 1;  /* skip argv[0] */
    if (idx < 0 || idx >= a->argc) return NULL;
    return a->argv[idx];
}

/**
 * ec_args_count(args) — returns the raw argument count (equivalent to
 * argc).  Rarely needed; provided for completeness.
 */
EC_INLINE int ec_args_count(const ec_args *a) {
    return a->argc;
}

/* ===========================================================================
 * Section 15: Generic hash map  (open addressing, linear probing, FNV-1a)
 * =========================================================================
 *
 * A type-safe, macro-driven hash map that works with any key and value
 * type.  Internally uses a single polymorphic implementation: keys and
 * values are stored inline via memcpy so the map owns its data.
 *
 * Open addressing with linear probing, FNV-1a hashing, and power-of-two
 * capacity for fast bitmask indexing.  Tombstone entries keep the
 * probing chain intact after removals.
 *
 * String keys (char*) are detected automatically via _Generic: string
 * literals, char* variables, and const char* variables all work as
 * string keys — no casts needed.  Comparison and hashing use the string
 * *content* (strcmp / FNV-1a over the string), not the pointer address.
 * The caller must ensure that string keys remain valid for the lifetime
 * of the entry.
 *
 * Example (int → int):
 *     hm(int, int) scores;
 *     hm_init(scores);
 *     int k1 = 42, v1 = 100, k2 = 7, v2 = 200;
 *     hm_set(scores, k1, v1);
 *     hm_set(scores, k2, v2);
 *     int val;
 *     int key = 42;
 *     if (hm_get(scores, key, &val)) println("score:", val);  // 100
 *     hm_free(scores);
 *
 * Example (string → float):
 *     hm(char*, float) prices;
 *     hm_init(prices);
 *     const char *apple = "apple", *bread = "bread";
 *     hm_set(prices, apple, 1.29f);
 *     hm_set(prices, bread, 3.49f);
 *     float price;
 *     const char *lookup = "apple";
 *     if (hm_get(prices, lookup, &price)) println("price:", price);
 *     hm_free(prices);
 *
 * HEAP ALLOCATION: hm_init() does not allocate; the slot table is
 * allocated on the first hm_set() call.  hm_free() releases all
 * internal memory.  Each hm_set() copies the key and value into the
 * map (shallow memcpy — for pointer-typed values, the caller retains
 * ownership of the pointed-to data).
 */

/* ---- Internal: polymorphic hash map ------------------------------------ */

#define EC_HM_INIT_CAP   16
#define EC_HM_MAX_LOAD    7   /* numerator   */
#define EC_HM_LOAD_SCALE  10  /* denominator => 70 % max load */

typedef struct {
    unsigned char *slots;    /* flat array: [key][val][state] per slot */
    size_t         cap;      /* number of slots (always power of 2) */
    size_t         len;      /* occupied slots */
    size_t         used;     /* occupied + tombstones (drives load factor) */
    size_t         key_size; /* bytes per key   (0 → uninitialised) */
    size_t         val_size; /* bytes per value (0 → uninitialised) */
    bool           str_keys; /* true when key_size == sizeof(char*) */
} ec_hashmap;

/* Total bytes per slot: key + val + 1 state byte. */
EC_INLINE size_t ec_hm_slot_sz_(const ec_hashmap *m) {
    return m->key_size + m->val_size + 1;
}

/* Pointers into a slot. */
EC_INLINE unsigned char *ec_hm_slot_(const ec_hashmap *m, size_t i) {
    return m->slots + i * ec_hm_slot_sz_(m);
}
EC_INLINE unsigned char *ec_hm_key_(const ec_hashmap *m, size_t i) {
    return ec_hm_slot_(m, i);
}
EC_INLINE unsigned char *ec_hm_val_(const ec_hashmap *m, size_t i) {
    return ec_hm_slot_(m, i) + m->key_size;
}
EC_INLINE uint8_t *ec_hm_state_(const ec_hashmap *m, size_t i) {
    return (uint8_t *)(ec_hm_val_(m, i) + m->val_size);
}

/* FNV-1a 64-bit over raw bytes. */
EC_INLINE uint64_t ec_hm_hash_bytes_(const void *data, size_t len) {
    uint64_t h = 14695981039346656037ULL;
    const unsigned char *p = (const unsigned char *)data;
    for (size_t i = 0; i < len; i++) {
        h ^= (uint64_t)p[i];
        h *= 1099511628211ULL;
    }
    return h;
}

/* Hash and comparison helpers for string keys (char*).  These are kept
 * separate so that the compiler does not warn about array-bounds when
 * the same inline functions are called with non-string (e.g. int) keys. */
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif
EC_INLINE uint64_t ec_hm_hash_str_(const void *key) {
    const char *s;
    memcpy(&s, key, sizeof(s));
    return ec_hm_hash_bytes_(s, s ? strlen(s) : 0);
}

EC_INLINE bool ec_hm_key_eq_str_(const unsigned char *slot_key,
                                  const void *search_key) {
    const char *sa;  memcpy(&sa, slot_key,   sizeof(sa));
    const char *sb;  memcpy(&sb, search_key, sizeof(sb));
    if (!sa || !sb) return sa == sb;
    return strcmp(sa, sb) == 0;
}
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

/* Compute the hash for a key pointer.  For string keys, hash the string
 * content; otherwise hash the raw key bytes. */
EC_INLINE uint64_t ec_hm_hash_key_(const ec_hashmap *m, const void *key) {
    if (m->str_keys) return ec_hm_hash_str_(key);
    return ec_hm_hash_bytes_(key, m->key_size);
}

/* Compare two keys.  For string keys, use strcmp; otherwise memcmp. */
EC_INLINE bool ec_hm_key_eq_(const ec_hashmap *m,
                             const unsigned char *slot_key,
                             const void *search_key) {
    if (m->str_keys) return ec_hm_key_eq_str_(slot_key, search_key);
    return memcmp(slot_key, search_key, m->key_size) == 0;
}

/* Internal: resize to new_cap (power of 2).  Rehashes all live entries.
 * On failure the map is unchanged. */
EC_INLINE bool ec_hm_resize_(ec_hashmap *m, size_t new_cap) {
    size_t slot_sz = ec_hm_slot_sz_(m);
    unsigned char *old_slots = m->slots;
    size_t         old_cap   = m->cap;

    m->slots = (unsigned char *)calloc(new_cap, slot_sz);
    if (!m->slots) { m->slots = old_slots; return false; }
    m->cap  = new_cap;
    m->len  = 0;
    m->used = 0;

    for (size_t i = 0; i < old_cap; i++) {
        uint8_t st = *(uint8_t *)(old_slots + i * slot_sz
                                  + m->key_size + m->val_size);
        if (st != 1) continue;  /* not occupied */
        unsigned char *old_key = old_slots + i * slot_sz;
        uint64_t h  = ec_hm_hash_key_(m, old_key);
        size_t   idx = (size_t)(h & (new_cap - 1));
        while (*ec_hm_state_(m, idx) == 1)
            idx = (idx + 1) & (new_cap - 1);
        memcpy(ec_hm_key_(m, idx),  old_key,             m->key_size);
        memcpy(ec_hm_val_(m, idx),  old_key + m->key_size, m->val_size);
        *ec_hm_state_(m, idx) = 1;
        m->len++;  m->used++;
    }
    free(old_slots);
    return true;
}

/* Internal: grow the slot table if load factor exceeds 70 %. */
EC_INLINE bool ec_hm_grow_if_needed_(ec_hashmap *m) {
    if (m->cap == 0) {
        /* First allocation. */
        size_t slot_sz = ec_hm_slot_sz_(m);
        m->slots = (unsigned char *)calloc(EC_HM_INIT_CAP, slot_sz);
        if (!m->slots) return false;
        m->cap = EC_HM_INIT_CAP;
        return true;
    }
    if (m->used * EC_HM_LOAD_SCALE >= m->cap * EC_HM_MAX_LOAD)
        return ec_hm_resize_(m, m->cap * 2);
    return true;
}

/* ---- Public typed macros ----------------------------------------------- */

#define EC_HM_VAL_PTR_(x)  ((const void *)&(x))
/* Internal: core insertion logic shared by hm_set. */
EC_INLINE void ec_hm_set_impl_(ec_hashmap *m,
                                const void *key, const void *val) {
    uint64_t h  = ec_hm_hash_key_(m, key);
    size_t   idx = (size_t)(h & (m->cap - 1));
    size_t   tomb = (size_t)-1;
    for (;;) {
        uint8_t st = *ec_hm_state_(m, idx);
        if (st == 0) {
            size_t ins = (tomb != (size_t)-1) ? tomb : idx;
            memcpy(ec_hm_key_(m, ins), key, m->key_size);
            memcpy(ec_hm_val_(m, ins), val, m->val_size);
            *ec_hm_state_(m, ins) = 1;
            m->len++;
            if (tomb == (size_t)-1) m->used++;
            return;
        }
        if (st == 2) {
            if (tomb == (size_t)-1) tomb = idx;
        } else if (ec_hm_key_eq_(m, ec_hm_key_(m, idx), key)) {
            memcpy(ec_hm_val_(m, idx), val, m->val_size);
            return;
        }
        idx = (idx + 1) & (m->cap - 1);
    }
}

/**
 * hm(K, V) — declares a hash map variable with the given key and value
 * types.  Expands to `ec_hashmap`; the types are captured by the other
 * hm_* macros via sizeof.
 */
#define hm(K, V) ec_hashmap

/**
 * hm_init(m) — zero-initialises a hash map.  No allocation is performed
 * until the first hm_set() call.  Safe to call on a map from the stack.
 */
#define hm_init(m) memset(&(m), 0, sizeof(m))

/**
 * hm_set(m, k, v) — inserts or updates a key-value pair.  Both key and
 * value are copied into the map (shallow memcpy).  On allocation failure
 * the map is unchanged (the item is silently dropped).
 *
 * On the first call, key and value types are detected via _Generic:
 * char* and const char* are recognised as string keys (compared by
 * content via strcmp, hashed via FNV-1a over the string).
 * All other types are compared/hashed bytewise via memcmp/FNV-1a.
 * Subsequent calls MUST use the same types.
 *
 * IMPORTANT: `k` and `v` must be lvalues (variables).  Even string
 * literals must be stored in a variable first:
 *     const char *apple = "apple";
 *     hm_set(prices, apple, 1.29f);
 *
 * HEAP ALLOCATION: on first call (or when the map grows), allocates
 * or reallocates the internal slot table.
 */
#define hm_set(m, k, v) do { \
    ec_hashmap *ec_hm_m_ = &(m); \
    if (ec_hm_m_->key_size == 0) { \
        ec_hm_m_->key_size = _Generic((k), \
            char*: sizeof(char*), const char*: sizeof(char*), \
            default: sizeof(k)); \
        ec_hm_m_->val_size = _Generic((v), \
            char*: sizeof(char*), const char*: sizeof(char*), \
            default: sizeof(v)); \
        ec_hm_m_->str_keys  = _Generic((k), \
            char*: true, const char*: true, default: false); \
    } \
    if (!ec_hm_grow_if_needed_(ec_hm_m_)) break; \
    ec_hm_set_impl_(ec_hm_m_, EC_HM_VAL_PTR_(k), EC_HM_VAL_PTR_(v)); \
} while (0)

/**
 * hm_get(m, k, v) — looks up `k` and, if found, copies the associated
 * value into `*v`.  `v` must be a pointer to a variable of the correct
 * value type.  Returns true if the key was found.
 *
 * IMPORTANT: `k` must be an lvalue (a named variable).  For string-keyed
 * maps, use a const char* variable for the lookup key:
 *     const char *key = "apple";
 *     hm_get(prices, key, &price);
 *
 * Example:
 *     int val;
 *     int key = 42;
 *     if (hm_get(scores, key, &val)) println("Found:", val);
 */
#define hm_get(m, k, v) \
    ec_hm_get_impl_(&(m), EC_HM_VAL_PTR_(k), (void *)(v))

EC_INLINE bool ec_hm_get_impl_(const ec_hashmap *m,
                               const void *key, void *val_out) {
    if (!m->slots || m->len == 0 || m->key_size == 0) return false;
    uint64_t h  = ec_hm_hash_key_(m, key);
    size_t   idx = (size_t)(h & (m->cap - 1));
    for (;;) {
        uint8_t st = *ec_hm_state_(m, idx);
        if (st == 0) return false;
        if (st == 1 && ec_hm_key_eq_(m, ec_hm_key_(m, idx), key)) {
            if (val_out) memcpy(val_out, ec_hm_val_(m, idx), m->val_size);
            return true;
        }
        idx = (idx + 1) & (m->cap - 1);
    }
}
#define hm_contains(m, k) \
    ec_hm_contains_impl_(&(m), EC_HM_VAL_PTR_(k))

EC_INLINE bool ec_hm_contains_impl_(const ec_hashmap *m, const void *key) {
    return ec_hm_get_impl_(m, key, NULL);
}

/**
 * hm_remove(m, k) — removes `k` from the map.  Returns true if the key
 * was present.  The slot becomes a tombstone so the probing chain is
 * preserved.
 */
#define hm_remove(m, k) \
    ec_hm_remove_impl_(&(m), EC_HM_VAL_PTR_(k))

EC_INLINE bool ec_hm_remove_impl_(ec_hashmap *m, const void *key) {
    if (!m->slots || m->len == 0 || m->key_size == 0) return false;
    uint64_t h  = ec_hm_hash_key_(m, key);
    size_t   idx = (size_t)(h & (m->cap - 1));
    for (;;) {
        uint8_t st = *ec_hm_state_(m, idx);
        if (st == 0) return false;
        if (st == 1 && ec_hm_key_eq_(m, ec_hm_key_(m, idx), key)) {
            *ec_hm_state_(m, idx) = 2;  /* tombstone */
            m->len--;
            return true;
        }
        idx = (idx + 1) & (m->cap - 1);
    }
}

/**
 * hm_clear(m) — removes all entries but retains the allocated slot
 * table.  Subsequent hm_set() calls reuse the existing buffer.
 */
#define hm_clear(m) do { \
    ec_hashmap *ec_hm_mc_ = &(m); \
    if (ec_hm_mc_->slots) { \
        memset(ec_hm_mc_->slots, 0, ec_hm_mc_->cap * ec_hm_slot_sz_(ec_hm_mc_)); \
    } \
    ec_hm_mc_->len = 0; \
    ec_hm_mc_->used = 0; \
} while (0)

/**
 * hm_free(m) — frees the slot table and zeros the map.  Safe to call
 * on an already-freed or zero-initialised map.
 */
#define hm_free(m) do { \
    ec_hashmap *ec_hm_mf_ = &(m); \
    free(ec_hm_mf_->slots); \
    ec_hm_mf_->slots = NULL; \
    ec_hm_mf_->cap = 0; \
    ec_hm_mf_->len = 0; \
    ec_hm_mf_->used = 0; \
    ec_hm_mf_->key_size = 0; \
    ec_hm_mf_->val_size = 0; \
} while (0)

/**
 * hm_size(m) — returns the number of key-value pairs currently stored.
 */
#define hm_size(m)  ((m).len)

/**
 * hm_empty(m) — returns true if the map contains no entries.
 */
#define hm_empty(m) ((m).len == 0)


/* Internal: strdup for backward-compat ec_hm_set / ec_hm_del. */
EC_INLINE char *ec_hm_strdup_(const char *s, size_t len) {
    char *copy = (char *)malloc(len + 1);
    if (!copy) return NULL;
    memcpy(copy, s, len);
    copy[len] = '\0';
    return copy;
}
/* ---- Backward-compatible string→string wrappers ------------------------ */

/**
 * ec_hm_new() — creates a string→string hash map.  Equivalent to
 * `hm(char*, char*); hm_init(m);`.  Provided for backward compatibility.
 *
 * HEAP ALLOCATION: the slot table is allocated on first set, not here.
 * The returned map must be released with ec_hm_free().
 */
EC_INLINE ec_hashmap ec_hm_new(void) {
    ec_hashmap m;
    memset(&m, 0, sizeof(m));
    m.key_size = sizeof(char*);
    m.val_size = sizeof(char*);
    m.str_keys = true;
    return m;
}

/**
 * ec_hm_set(map, key, val) — string→string convenience wrapper.
 * Returns true on success.
 */
EC_INLINE bool ec_hm_set(ec_hashmap *m, const char *key, const char *val) {
    if (m->key_size == 0) {
        m->key_size = sizeof(char*);
        m->val_size = sizeof(char*);
        m->str_keys = true;
    }
    if (!ec_hm_grow_if_needed_(m)) return false;
    uint64_t h  = ec_hm_hash_key_(m, &key);
    size_t   idx = (size_t)(h & (m->cap - 1));
    size_t   tomb = (size_t)-1;
    for (;;) {
        uint8_t st = *ec_hm_state_(m, idx);
        if (st == 0) {
            size_t ins = (tomb != (size_t)-1) ? tomb : idx;
            char **kp = (char **)ec_hm_key_(m, ins);
            char **vp = (char **)ec_hm_val_(m, ins);
            *kp = key ? ec_hm_strdup_(key, strlen(key)) : NULL;
            if (key && !*kp) return false;
            *vp = val ? ec_hm_strdup_(val, strlen(val)) : NULL;
            if (val && !*vp) { free(*kp); *kp = NULL; return false; }
            *ec_hm_state_(m, ins) = 1;
            m->len++;
            if (tomb == (size_t)-1) m->used++;
            return true;
        }
        if (st == 2) {
            if (tomb == (size_t)-1) tomb = idx;
        } else if (ec_hm_key_eq_(m, ec_hm_key_(m, idx), &key)) {
            char **vp = (char **)ec_hm_val_(m, idx);
            char *new_v = val ? ec_hm_strdup_(val, strlen(val)) : NULL;
            if (val && !new_v) return false;
            free(*vp);
            *vp = new_v;
            return true;
        }
        idx = (idx + 1) & (m->cap - 1);
    }
}

/**
 * ec_hm_get(map, key) — string→string convenience wrapper.
 * Returns the value associated with `key`, or NULL if not found.
 * The returned pointer is owned by the map — do not free it.
 */
EC_INLINE const char *ec_hm_get(const ec_hashmap *m, const char *key) {
    if (!m->slots || m->len == 0) return NULL;
    uint64_t h  = ec_hm_hash_key_(m, &key);
    size_t   idx = (size_t)(h & (m->cap - 1));
    for (;;) {
        uint8_t st = *ec_hm_state_(m, idx);
        if (st == 0) return NULL;
        if (st == 1 && ec_hm_key_eq_(m, ec_hm_key_(m, idx), &key))
            return *(const char *const *)ec_hm_val_(m, idx);
        idx = (idx + 1) & (m->cap - 1);
    }
}

/**
 * ec_hm_has(map, key) — returns true if `key` exists in the map.
 */
EC_INLINE bool ec_hm_has(const ec_hashmap *m, const char *key) {
    return ec_hm_get(m, key) != NULL;
}

/**
 * ec_hm_del(map, key) — removes `key` from the map.  Returns true if
 * the key was present.  Internal string copies are freed.
 */
EC_INLINE bool ec_hm_del(ec_hashmap *m, const char *key) {
    if (!m->slots || m->len == 0) return false;
    uint64_t h  = ec_hm_hash_key_(m, &key);
    size_t   idx = (size_t)(h & (m->cap - 1));
    for (;;) {
        uint8_t st = *ec_hm_state_(m, idx);
        if (st == 0) return false;
        if (st == 1 && ec_hm_key_eq_(m, ec_hm_key_(m, idx), &key)) {
            free(*(char **)ec_hm_key_(m, idx));
            free(*(char **)ec_hm_val_(m, idx));
            *ec_hm_state_(m, idx) = 2;
            m->len--;
            return true;
        }
        idx = (idx + 1) & (m->cap - 1);
    }
}

/**
 * ec_hm_len(map) — returns the number of key-value pairs in the map.
 */
EC_INLINE size_t ec_hm_len(const ec_hashmap *m) { return m->len; }

/**
 * ec_hm_free(map) — frees all internally-owned string copies and the
 * slot table.  The map is zeroed out.
 */
EC_INLINE void ec_hm_free(ec_hashmap *m) {
    if (!m->slots) { memset(m, 0, sizeof(*m)); return; }
    if (m->str_keys && m->key_size == sizeof(char*)) {
        for (size_t i = 0; i < m->cap; i++) {
            if (*ec_hm_state_(m, i) == 1) {
                free(*(char **)ec_hm_key_(m, i));
                free(*(char **)ec_hm_val_(m, i));
            }
        }
    }
    free(m->slots);
    memset(m, 0, sizeof(*m));
}


/* ===========================================================================
 * Section 16: Pool allocator  (fixed-size, O(1) alloc / free)
 * =========================================================================
 *
 * A pool allocator pre-allocates a fixed number of identically-sized
 * blocks and serves them from an intrusive free list.  Both allocation
 * and deallocation are O(1) — just pop / push a linked-list node.
 * Zero fragmentation, zero per-object heap overhead.
 *
 * Ideal for:
 *   - Entity / component systems in games
 *   - Particle pools
 *   - Any workload where you allocate and free many objects of the same
 *     size, and you know the maximum count at startup.
 *
 * Example:
 *     typedef struct { float x, y; int hp; } Enemy;
 *     ec_pool pool = ec_pool_new(sizeof(Enemy), 1000);
 *     Enemy *e = ec_pool_alloc(&pool);   // O(1) — grab from free list
 *     e->hp = 100;
 *     ec_pool_free_block(&pool, e);      // O(1) — return to free list
 *     ec_pool_destroy(&pool);
 */

/* Intrusive free-list node stored in freed blocks.  Block size is
 * automatically rounded up to at least sizeof(ec_pool_free). */
typedef struct ec_pool_free {
    struct ec_pool_free *next;
} ec_pool_free;

typedef struct {
    unsigned char *buf;         /* backing buffer */
    size_t         block_size;  /* aligned block size in bytes */
    size_t         block_count; /* total number of blocks */
    ec_pool_free  *free_list;   /* head of the free list */
} ec_pool;

/**
 * ec_pool_new(block_size, block_count) — creates a pool of
 * `block_count` blocks, each `block_size` bytes (auto-aligned to
 * pointer width and rounded up to at least sizeof(void*)).
 *
 * Returns a valid-but-empty pool (buf == NULL) on allocation failure.
 *
 * HEAP ALLOCATION: the returned pool owns its backing buffer, which
 * must be released with ec_pool_destroy().
 */
EC_INLINE ec_pool ec_pool_new(size_t block_size, size_t block_count) {
    ec_pool p;

    /* Each block must be large enough to hold the free-list pointer
     * when it is freed.  Align to pointer size for all platforms. */
    if (block_size < sizeof(ec_pool_free))
        block_size = sizeof(ec_pool_free);
    block_size = (block_size + sizeof(void *) - 1) & ~(sizeof(void *) - 1);

    p.block_size  = block_size;
    p.block_count = block_count;
    p.buf         = (unsigned char *)malloc(block_size * block_count);
    p.free_list   = NULL;

    /* Build the initial free list — all blocks are free at creation. */
    if (p.buf) {
        for (size_t i = 0; i < block_count; i++) {
            ec_pool_free *blk = (ec_pool_free *)(p.buf + i * block_size);
            blk->next = p.free_list;
            p.free_list = blk;
        }
    }

    return p;
}

/**
 * ec_pool_alloc(pool) — returns a pointer to a free block, or NULL if
 * the pool is exhausted.  O(1).  The memory is UNINITIALISED.
 */
EC_INLINE void *ec_pool_alloc(ec_pool *p) {
    if (!p->free_list) return NULL;
    ec_pool_free *blk = p->free_list;
    p->free_list = blk->next;
    return (void *)blk;
}

/**
 * ec_pool_free_block(pool, block) — returns a block to the pool.
 * O(1).  `block` MUST have come from ec_pool_alloc() on the same pool;
 * passing a stray pointer is undefined.  Safe to call with NULL
 * (no-op).
 */
EC_INLINE void ec_pool_free_block(ec_pool *p, void *block) {
    if (!block) return;
    ec_pool_free *blk = (ec_pool_free *)block;
    blk->next = p->free_list;
    p->free_list = blk;
}

/**
 * ec_pool_available(pool) — returns the number of blocks currently
 * free in the pool.  O(n) in the free-list length (walks the list).
 * Use sparingly; for production checks prefer ec_pool_alloc() == NULL.
 */
EC_INLINE size_t ec_pool_available(const ec_pool *p) {
    size_t n = 0;
    for (ec_pool_free *f = p->free_list; f; f = f->next) n++;
    return n;
}

/**
 * ec_pool_destroy(pool) — frees the backing buffer and zeros out the
 * pool struct.  Safe to call multiple times.
 */
EC_INLINE void ec_pool_destroy(ec_pool *p) {
    free(p->buf);
    p->buf         = NULL;
    p->block_size  = 0;
    p->block_count = 0;
    p->free_list   = NULL;
}

/* ===========================================================================
 * Section 17: String builder
 * =========================================================================
 *
 * A growable, null-terminated string buffer.  Build strings piece by
 * piece without worrying about buffer sizes, snprintf arithmetic, or
 * manual realloc.  The buffer is always null-terminated so .data is
 * always a valid C string.
 *
 * Internally uses the same realloc-doubling strategy as dynamic arrays,
 * so repeated appends are amortized O(1).
 *
 * Example:
 *     ec_string s = string_new();
 *     string_append(&s, "Hello, ");
 *     string_append(&s, name);
 *     string_appendf(&s, " — you are visitor #%d", count);
 *     println(s.data);
 *     string_free(&s);
 */

typedef struct {
    char   *data;   /* null-terminated buffer (NULL when empty + no alloc) */
    size_t  len;    /* current length, excluding null terminator */
    size_t  cap;    /* usable capacity, excluding the null byte */
} ec_string;

/**
 * string_new() — returns an empty string builder.  No allocation is
 * performed until the first append.
 */
EC_INLINE ec_string string_new(void) {
    ec_string s = {NULL, 0, 0};
    return s;
}

/* Internal: ensure room for `needed` additional chars (+ null). */
EC_INLINE void ec_string_grow_(ec_string *s, size_t needed) {
    if (s->len + needed < s->cap) return;
    size_t new_cap = s->cap ? s->cap * 2 : 32;
    size_t min_need = s->len + needed + 1;  /* +1 for null */
    if (new_cap < min_need) new_cap = min_need;
    char *nd = (char *)realloc(s->data, new_cap);
    if (!nd) return;
    if (!s->data) nd[0] = '\0';
    s->data = nd;
    s->cap  = new_cap;
}

/**
 * string_append(s, str) — appends a C string.
 */
EC_INLINE void string_append(ec_string *s, const char *str) {
    size_t slen = strlen(str);
    if (slen == 0) return;
    ec_string_grow_(s, slen);
    if (!s->data) return;
    memcpy(s->data + s->len, str, slen);
    s->len += slen;
    s->data[s->len] = '\0';
}

/**
 * string_append_char(s, c) — appends a single character.
 */
EC_INLINE void string_append_char(ec_string *s, char c) {
    ec_string_grow_(s, 1);
    if (!s->data) return;
    s->data[s->len++] = c;
    s->data[s->len] = '\0';
}

/**
 * string_append_int(s, val) — appends the decimal representation of `val`.
 */
EC_INLINE void string_append_int(ec_string *s, int val) {
    char buf[32];
    int n = snprintf(buf, sizeof(buf), "%d", val);
    if (n <= 0) return;
    ec_string_grow_(s, (size_t)n);
    if (!s->data) return;
    memcpy(s->data + s->len, buf, (size_t)n);
    s->len += (size_t)n;
    s->data[s->len] = '\0';
}

/**
 * string_append_long(s, val) — appends the decimal representation of `val`.
 */
EC_INLINE void string_append_long(ec_string *s, long val) {
    char buf[32];
    int n = snprintf(buf, sizeof(buf), "%ld", val);
    if (n <= 0) return;
    ec_string_grow_(s, (size_t)n);
    if (!s->data) return;
    memcpy(s->data + s->len, buf, (size_t)n);
    s->len += (size_t)n;
    s->data[s->len] = '\0';
}

/**
 * string_append_float(s, val) — appends the shortest decimal representation.
 */
EC_INLINE void string_append_float(ec_string *s, float val) {
    char buf[64];
    int n = snprintf(buf, sizeof(buf), "%g", (double)val);
    if (n <= 0) return;
    ec_string_grow_(s, (size_t)n);
    if (!s->data) return;
    memcpy(s->data + s->len, buf, (size_t)n);
    s->len += (size_t)n;
    s->data[s->len] = '\0';
}

/**
 * string_append_double(s, val) — appends the shortest decimal representation.
 */
EC_INLINE void string_append_double(ec_string *s, double val) {
    char buf[64];
    int n = snprintf(buf, sizeof(buf), "%g", val);
    if (n <= 0) return;
    ec_string_grow_(s, (size_t)n);
    if (!s->data) return;
    memcpy(s->data + s->len, buf, (size_t)n);
    s->len += (size_t)n;
    s->data[s->len] = '\0';
}

/**
 * string_append_bool(s, val) — appends "true" or "false".
 */
EC_INLINE void string_append_bool(ec_string *s, bool val) {
    string_append(s, val ? "true" : "false");
}

/**
 * string_appendf(s, fmt, ...) — printf-style append.  Formats into the
 * buffer using the given format string and arguments.
 *
 * Example:
 *     string_appendf(&s, "%s: %d/%d", label, current, total);
 */
EC_INLINE void string_appendf(ec_string *s, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (n <= 0) return;

    va_start(args, fmt);
    ec_string_grow_(s, (size_t)n);
    if (!s->data) { va_end(args); return; }
    vsnprintf(s->data + s->len, (size_t)n + 1, fmt, args);
    va_end(args);
    s->len += (size_t)n;
}

/**
 * string_fromf(fmt, ...) — alias for string_printf().  Provided for
 * backward compatibility and as a more descriptive name for one-shot
 * formatted string creation.
 *
 * HEAP ALLOCATION: the returned string must be freed with string_free().
 *
 * Example:
 *     ec_string msg = string_fromf("Page %d of %d", 3, 10);
 *     println(msg.data);
 *     string_free(&msg);
 */
#define string_fromf(...) string_printf(__VA_ARGS__)

/**
 * string_clear(s) — resets the string to empty (length 0) without
 * freeing the underlying buffer.  `s->data` remains a valid empty
 * string ("").
 */
EC_INLINE void string_clear(ec_string *s) {
    if (s->data) { s->data[0] = '\0'; }
    s->len = 0;
}

/**
 * string_free(s) — frees the underlying buffer and zeros the struct.
 * Safe to call on an already-freed or zero-initialised string.
 */
EC_INLINE void string_free(ec_string *s) {
    free(s->data);
    s->data = NULL;
    s->len  = 0;
    s->cap  = 0;
}

/**
 * string_len(s) — returns the current length (excluding null).
 */
EC_INLINE size_t string_len(const ec_string *s) { return s->len; }

/**
 * string_empty(s) — returns true if the string is empty.
 */
EC_INLINE bool string_empty(const ec_string *s) { return s->len == 0; }

/**
 * string_cstr(s) — returns a const pointer to the null-terminated
 * buffer.  Returns "" (empty string) if the string has no allocation.
 * The pointer is owned by the ec_string — do not free it.
 */
EC_INLINE const char *string_cstr(const ec_string *s) {
    return s->data ? s->data : "";
}

/**
 * string_from(text) — creates a new ec_string initialised with a copy
 * of the given C string.  The argument may be NULL, in which case an
 * empty string is returned.
 *
 * HEAP ALLOCATION: the returned string must be freed with string_free().
 *
 * Example:
 *     ec_string s = string_from("hello");
 *     println(s.data);
 *     string_free(&s);
 */
EC_INLINE ec_string string_from(const char *text) {
    ec_string s = string_new();
    if (text) string_append(&s, text);
    return s;
}

/**
 * string_insert(s, index, text) — inserts `text` at position `index` in
 * the string, shifting existing characters to the right.  `index` may be
 * `s->len` (equivalent to string_append).  Undefined if `index` > `s->len`.
 *
 * O(n) where n is the number of characters shifted.
 *
 * Example:
 *     ec_string s = string_from("hello");
 *     string_insert(&s, 5, " world");   // "hello world"
 *     string_free(&s);
 */
EC_INLINE void string_insert(ec_string *s, size_t index, const char *text) {
    size_t tlen = strlen(text);
    if (tlen == 0) return;
    ec_string_grow_(s, tlen);
    if (!s->data) return;
    if (index < s->len) {
        memmove(s->data + index + tlen, s->data + index, s->len - index);
    }
    memcpy(s->data + index, text, tlen);
    s->len += tlen;
    s->data[s->len] = '\0';
}

/**
 * string_remove(s, index, count) — removes `count` characters starting
 * at `index`, shifting subsequent characters left.  If `index + count`
 * exceeds the string length, only the characters up to the end are
 * removed.  Safe to call on an empty string (no-op).
 *
 * O(n) where n is the number of characters shifted.
 *
 * Example:
 *     ec_string s = string_from("hello world");
 *     string_remove(&s, 5, 6);          // "hello"
 *     string_free(&s);
 */
EC_INLINE void string_remove(ec_string *s, size_t index, size_t count) {
    if (index >= s->len) return;
    if (index + count > s->len) count = s->len - index;
    if (count == 0) return;
    memmove(s->data + index, s->data + index + count, s->len - index - count + 1);
    s->len -= count;
}

/**
 * string_capacity(s) — returns the currently allocated capacity
 * (excluding the null terminator).  Returns 0 if the string has no
 * allocation yet.
 */
EC_INLINE size_t string_capacity(const ec_string *s) { return s->cap; }

/**
 * string_equals(a, b) — returns true if the two ec_strings have
 * identical contents.  Equivalent to strcmp(a.data, b.data) == 0.
 * Either argument may be NULL (treated as empty).
 *
 * Example:
 *     ec_string a = string_from("abc");
 *     ec_string b = string_from("abc");
 *     if (string_equals(&a, &b)) println("match");
 *     string_free(&a); string_free(&b);
 */
EC_INLINE bool string_equals(const ec_string *a, const ec_string *b) {
    const char *ad = a && a->data ? a->data : "";
    const char *bd = b && b->data ? b->data : "";
    return strcmp(ad, bd) == 0;
}

/**
 * string_contains(s, text) — returns true if `text` occurs anywhere
 * inside the ec_string's data.
 *
 * Example:
 *     ec_string s = string_from("hello world");
 *     if (string_contains(&s, "world")) println("found it");
 *     string_free(&s);
 */
EC_INLINE bool string_contains(const ec_string *s, const char *text) {
    if (!s || !s->data) return !*text;
    return strstr(s->data, text) != NULL;
}

/**
 * string_starts_with(s, text) — returns true if the ec_string's data
 * begins with `text`.
 *
 * Example:
 *     ec_string s = string_from("/usr/local/bin");
 *     if (string_starts_with(&s, "/usr")) println("in /usr");
 *     string_free(&s);
 */
EC_INLINE bool string_starts_with(const ec_string *s, const char *text) {
    if (!s || !s->data) return !*text;
    size_t tlen = strlen(text);
    return s->len >= tlen && memcmp(s->data, text, tlen) == 0;
}

/**
 * string_ends_with(s, text) — returns true if the ec_string's data ends
 * with `text`.
 *
 * Example:
 *     ec_string s = string_from("report.pdf");
 *     if (string_ends_with(&s, ".pdf")) println("PDF file");
 *     string_free(&s);
 */
EC_INLINE bool string_ends_with(const ec_string *s, const char *text) {
    if (!s || !s->data) return !*text;
    size_t tlen = strlen(text);
    if (tlen > s->len) return false;
    return memcmp(s->data + s->len - tlen, text, tlen) == 0;
}

/**
 * string_lowercase(s) — converts the ec_string's contents to lowercase
 * IN PLACE.
 *
 * Example:
 *     ec_string s = string_from("HELLO");
 *     string_lowercase(&s);   // "hello"
 *     string_free(&s);
 */
EC_INLINE void string_lowercase(ec_string *s) {
    if (!s || !s->data) return;
    for (size_t i = 0; i < s->len; i++)
        s->data[i] = (char)tolower((unsigned char)s->data[i]);
}

/**
 * string_uppercase(s) — converts the ec_string's contents to uppercase
 * IN PLACE.
 *
 * Example:
 *     ec_string s = string_from("hello");
 *     string_uppercase(&s);   // "HELLO"
 *     string_free(&s);
 */
EC_INLINE void string_uppercase(ec_string *s) {
    if (!s || !s->data) return;
    for (size_t i = 0; i < s->len; i++)
        s->data[i] = (char)toupper((unsigned char)s->data[i]);
}

/**
 * string_trim(s) — removes leading and trailing whitespace from the
 * ec_string IN PLACE.  The capacity is unchanged; only the length is
 * updated and a new null terminator is written.
 *
 * Example:
 *     ec_string s = string_from("   hi there   ");
 *     string_trim(&s);        // "hi there"
 *     string_free(&s);
 */
EC_INLINE void string_trim(ec_string *s) {
    if (!s || !s->data || s->len == 0) return;
    size_t start = 0;
    while (start < s->len && isspace((unsigned char)s->data[start])) start++;
    if (start == s->len) { s->data[0] = '\0'; s->len = 0; return; }
    size_t end = s->len - 1;
    while (end > start && isspace((unsigned char)s->data[end])) end--;
    size_t new_len = end - start + 1;
    if (start > 0) memmove(s->data, s->data + start, new_len);
    s->data[new_len] = '\0';
    s->len = new_len;
}

/**
 * string_printf(fmt, ...) — creates a new ec_string from a printf-style
 * format string and arguments.  The buffer is sized exactly to fit the
 * formatted result.
 *
 * HEAP ALLOCATION: the returned string must be freed with string_free().
 *
 * Example:
 *     ec_string msg = string_printf("Value: %d (%.2f%%)", 42, 87.3);
 *     println(msg.data);
 *     string_free(&msg);
 */
EC_INLINE ec_string string_printf(const char *fmt, ...) {
    ec_string s = string_new();
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (n <= 0) return s;

    s.cap  = (size_t)n + 1;
    s.data = (char *)malloc(s.cap);
    if (!s.data) { s.cap = 0; return s; }

    va_start(args, fmt);
    vsnprintf(s.data, s.cap, fmt, args);
    va_end(args);
    s.len = (size_t)n;
    return s;
}

/**
 * string_length(s) — returns the current length (excluding the null
 * terminator).  Identical to string_len(s); provided as a more
 * descriptive alias.
 */
EC_INLINE size_t string_length(const ec_string *s) { return s->len; }


/* ===========================================================================
 * Section 18: Logging
 * =========================================================================
 *
 * Colored, timestamped logging macros that write to stderr.  Each
 * message is automatically prefixed with a [HH:MM:SS] timestamp and
 * the log level, and the level label is colorised with ANSI escapes.
 *
 * On Windows, call ec_init_colors() once at the start of main() to
 * enable ANSI escape processing in the console.  On Linux / macOS,
 * ANSI escapes work natively and ec_init_colors() is a no-op.
 *
 * Define EC_NO_COLORS before including eacy.h to disable ANSI color
 * codes globally — only the raw text and timestamp are printed.
 *
 * Example:
 *     ec_init_colors();
 *     log_info("Server starting on port", port);
 *     log_warn("Config file not found, using defaults");
 *     log_error("Connection refused");
 *     log_debug("Request took", elapsed, "ms");
 */

#if !defined(EC_NO_COLORS)
  #define EC_LOG_RED_     "\x1b[31m"
  #define EC_LOG_YELLOW_  "\x1b[33m"
  #define EC_LOG_CYAN_    "\x1b[36m"
  #define EC_LOG_DIM_     "\x1b[2m"
  #define EC_LOG_RESET_   "\x1b[0m"
#else
  #define EC_LOG_RED_     ""
  #define EC_LOG_YELLOW_  ""
  #define EC_LOG_CYAN_    ""
  #define EC_LOG_DIM_     ""
  #define EC_LOG_RESET_   ""
#endif


/* Internal: stderr-aware print helpers so log messages go to stderr,
 * not stdout.  These mirror ec_print_* / ec_print_one. */
EC_INLINE void ec_log_int_(int v)          { fprintf(stderr, "%d", v); }
EC_INLINE void ec_log_long_(long v)        { fprintf(stderr, "%ld", v); }
EC_INLINE void ec_log_uint_(unsigned int v) { fprintf(stderr, "%u", v); }
EC_INLINE void ec_log_ulong_(unsigned long v) { fprintf(stderr, "%lu", v); }
EC_INLINE void ec_log_ullong_(unsigned long long v) { fprintf(stderr, "%llu", v); }
EC_INLINE void ec_log_float_(float v)      { fprintf(stderr, "%g", (double)v); }
EC_INLINE void ec_log_double_(double v)    { fprintf(stderr, "%g", v); }
EC_INLINE void ec_log_char_(char v)        { fputc(v, stderr); }
EC_INLINE void ec_log_str_(char *v)        { fputs(v, stderr); }
EC_INLINE void ec_log_cstr_(const char *v) { fputs(v, stderr); }
EC_INLINE void ec_log_bool_(bool v)        { fputs(v ? "true" : "false", stderr); }
EC_INLINE void ec_log_ptr_(const void *v)  { fprintf(stderr, "%p", v); }
EC_INLINE void ec_log_sep_(void)           { fputc(' ', stderr); }

#define ec_log_one_(x) _Generic((x), \
    bool:            ec_log_bool_,   \
    char:            ec_log_char_,   \
    int:             ec_log_int_,    \
    long:            ec_log_long_,   \
    unsigned int:    ec_log_uint_,   \
    unsigned long:   ec_log_ulong_,  \
    unsigned long long: ec_log_ullong_, \
    float:           ec_log_float_,  \
    double:          ec_log_double_, \
    char*:           ec_log_str_,    \
    const char*:     ec_log_cstr_,   \
    default:         ec_log_ptr_     \
)(x)
/* Internal: writes a [HH:MM:SS] timestamp to stderr. */
EC_INLINE void ec_log_timestamp_(void) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char buf[16];
    strftime(buf, sizeof(buf), "[%H:%M:%S]", tm_info);
    fputs(buf, stderr);
}

/**
 * log_info(...) — prints an info-level message to stderr with a
 * cyan "INFO" label and timestamp.  Arguments are forwarded to
 * print() (no format strings — pass values directly).
 *
 * Example:
 *     log_info("Listening on port", 8080);
 *     // -> [14:32:05] INFO  Listening on port 8080
 */
#define log_info(...) do { \
    ec_log_timestamp_(); \
    fputs(EC_LOG_CYAN_ " INFO  " EC_LOG_RESET_, stderr); \
    EC_FOR_EACH(ec_log_one_, ec_log_sep_, __VA_ARGS__); \
    fputc('\n', stderr); \
} while (0)

/**
 * log_warn(...) — prints a warning-level message to stderr with a
 * yellow "WARN" label and timestamp.
 *
 * Example:
 *     log_warn("Low disk space:", free_mb, "MB remaining");
 *     // -> [14:32:06] WARN  Low disk space: 42 MB remaining
 */
#define log_warn(...) do { \
    ec_log_timestamp_(); \
    fputs(EC_LOG_YELLOW_ " WARN  " EC_LOG_RESET_, stderr); \
    EC_FOR_EACH(ec_log_one_, ec_log_sep_, __VA_ARGS__); \
    fputc('\n', stderr); \
} while (0)

/**
 * log_error(...) — prints an error-level message to stderr with a
 * red "ERROR" label and timestamp.
 *
 * Example:
 *     log_error("Failed to open", filename);
 *     // -> [14:32:07] ERROR Failed to open data.txt
 */
#define log_error(...) do { \
    ec_log_timestamp_(); \
    fputs(EC_LOG_RED_ " ERROR " EC_LOG_RESET_, stderr); \
    EC_FOR_EACH(ec_log_one_, ec_log_sep_, __VA_ARGS__); \
    fputc('\n', stderr); \
} while (0)

/**
 * log_debug(...) — prints a debug-level message to stderr with a
 * dimmed "DEBUG" label and timestamp.  Useful for development
 * tracing; compile it out by wrapping calls in #if guards if
 * desired.
 *
 * Example:
 *     log_debug("Request #", req_id, "from", client_ip);
 *     // -> [14:32:08] DEBUG Request #42 from 127.0.0.1
 */
#define log_debug(...) do { \
    ec_log_timestamp_(); \
    fputs(EC_LOG_DIM_ " DEBUG " EC_LOG_RESET_, stderr); \
    EC_FOR_EACH(ec_log_one_, ec_log_sep_, __VA_ARGS__); \
    fputc('\n', stderr); \
} while (0)

/* ===========================================================================
 * Section 19: Stopwatch (simple high-resolution timer)
 * =========================================================================
 *
 * A thin wrapper around current_time_ms() that makes ad-hoc timing
 * measurements easy to write and read.
 *
 * Example:
 *     ec_timer t = timer_start();
 *     do_work();
 *     println("Took", (long)timer_elapsed_ms(&t), "ms");
 */

typedef struct {
    long long start;   /* timestamp captured at timer_start() */
} ec_timer;

/**
 * timer_start() — captures the current monotonic time and returns an
 * ec_timer ready for measurement.  The value is obtained from
 * current_time_ms(), so it is safe against wall-clock adjustments.
 */
EC_INLINE ec_timer timer_start(void) {
    ec_timer t;
    t.start = current_time_ms();
    return t;
}

/**
 * timer_restart(t) — resets the timer to the current time.  Equivalent
 * to `*t = timer_start()` but slightly more descriptive at the call
 * site.
 */
EC_INLINE void timer_restart(ec_timer *t) {
    t->start = current_time_ms();
}

/**
 * timer_elapsed_ms(t) — returns the number of milliseconds that have
 * elapsed since the timer was started or last restarted.
 */
EC_INLINE long long timer_elapsed_ms(const ec_timer *t) {
    return current_time_ms() - t->start;
}

/**
 * timer_elapsed_seconds(t) — returns elapsed time in seconds (with
 * sub-millisecond precision as a double).
 */
EC_INLINE double timer_elapsed_seconds(const ec_timer *t) {
    return (double)(current_time_ms() - t->start) / 1000.0;
}

/* ===========================================================================
 * Section 20: Benchmark helpers
 * =========================================================================
 *
 * Zero-fuss benchmarking macros built on top of ec_timer.  Wrap any
 * block of code and EaCy prints the elapsed time automatically.
 *
 * Example (single run):
 *     benchmark("qsort 1e6 ints") {
 *         qsort(data, 1000000, sizeof(int), ec_cmp_int);
 *     }
 *     // -> qsort 1e6 ints: 42 ms
 *
 * Example (averaged over N runs):
 *     benchmark_avg("FFT 4096", 100) {
 *         fft_4096(signal);
 *     }
 *     // -> FFT 4096: 0.127 ms avg
 */

/**
 * benchmark(label) { ... } — times the execution of a single block and
 * prints the label followed by the elapsed time in milliseconds.
 */
#define benchmark(label) \
    for (ec_timer ec_bm_t_ = timer_start(), *_ec_bm_d_ = &ec_bm_t_; \
         _ec_bm_d_ != NULL; \
         printf("%s: %lld ms\n", label, \
                (long long)timer_elapsed_ms(_ec_bm_d_)), \
         _ec_bm_d_ = NULL)

/**
 * benchmark_avg(label, n) { ... } — runs the block `n` times and
 * prints the label followed by the average elapsed time in
 * milliseconds.  `n` is evaluated once.
 */
#define benchmark_avg(label, n) \
    for (struct { long long total; long i; ec_timer t; } \
         ec_bm_ = {0, 0, {current_time_ms()}}; \
         ec_bm_.i < (long)(n) || \
         (printf("%s: %.3f ms avg\n", label, \
                 (double)ec_bm_.total / (double)(n)), 0); \
         ec_bm_.total += timer_elapsed_ms(&ec_bm_.t), \
         ec_bm_.i++, \
         ec_bm_.t = timer_start())

#endif /* EACY_H */
