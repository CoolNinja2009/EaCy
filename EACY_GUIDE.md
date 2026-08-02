# EaCy — The C Library That Feels Like a Scripting Language

**One header. Zero dependencies. Compiles on GCC, Clang, and MSVC.**

Drop `eacy.h` next to your `.c` file and `#include "eacy.h"`. No build system,
no linking, no package manager.

EaCy is designed for **speed** — most functions compile down to a handful of
instructions, macros cache repeated lookups, and hot paths are kept
branch-free where possible.

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [Printing](#printing)
3. [Input](#input)
4. [Loops](#loops)
5. [Math](#math)
6. [Strings](#strings)
7. [Random Numbers](#random-numbers)
8. [Files](#files)
9. [Time](#time)
10. [Colors](#colors)
11. [Assertions](#assertions)
12. [Dynamic Arrays](#dynamic-arrays)
13. [Hash Map](#hash-map)
14. [Arena Allocator](#arena-allocator)
15. [Pool Allocator](#pool-allocator)
16. [String Builder](#string-builder)
17. [CLI Arguments](#cli-arguments)
18. [Memory Debugging](#memory-debugging)
19. [Complete Programs](#complete-programs)
20. [Naming Conventions](#naming-conventions)
21. [Performance Notes](#performance-notes)

---

## Quick Start

```c
#include "eacy.h"

int main(void) {
    println("Hello, world!");
    print("The answer is", 42);
    return 0;
}
```

```
$ gcc -std=c11 -O2 main.c -o main && ./main
Hello, world!
The answer is 42
```

---

## Printing

No format strings. Pass values — EaCy picks the right printer at compile time
via `_Generic`.

```c
int   age   = 30;
float pi    = 3.14159f;
char *name  = "Alice";
bool  admin = true;

println(age);              // 30
print("Name:", name);      // Name: Alice
print("pi =", pi);         // pi = 3.14159
print("admin:", admin);    // admin: true
nl();                       // blank line
```

**Supported types:** `int`, `long`, `float`, `double`, `char`, `char*`,
`const char*`, `bool`. Any other type prints its pointer address.

**Gotcha:** In C, `'c'` and `true`/`false` are `int`, not `char`/`bool`.
`print('c')` prints `99`. Store in a variable for the readable form:

```c
char c = 'c';
bool b = true;
println(c, b);  // c true
```

`print(...)` and `println(...)` are identical — both end with a newline.
Use `nl()` for a blank line.

---

## Input

### Type-safe scanning with `scan()`

```c
int x, y;
print("Enter two numbers: ");
scan(&x, &y);
println("Sum:", x + y);
```

The compiler picks the right `scanf` format based on the pointer type.
Supports `int*`, `long*`, `float*`, `double*`, `bool*`.

`char*` is deliberately **not** supported here (buffer overflow risk).
Use `input_string()` for text.

### Safe string input

```c
char name[100];
print("Your name: ");
input_string(name, sizeof(name));   // bounds-safe, strips \n
println("Hello,", name);
```

### Convenience readers

```c
int    i = input_int();      // reads a whole line, parses to int
float  f = input_float();
double d = input_double();
char   c = input_char();     // first non-whitespace char
```

### Important: flush after `scan()`

`scan()` leaves the trailing `\n` in the buffer. If you follow it with
`input_string()` or `input_char()`, call `scan_flush()` between them.

---

## Loops

### `repeat(n)` — do something N times

```c
repeat(5) println("Hi!");   // prints 5 times
```

Nestable — each gets its own counter via `__LINE__` token pasting.

### `foreach(i, arr)` — iterate a static array

```c
int nums[] = {10, 20, 30, 40, 50};
foreach(i, nums) println(nums[i]);
```

Uses `sizeof` — works on real arrays only, not decayed pointers.

### `for_range(i, start, end)` — numeric range

```c
for_range(i, 0, 5) println((long)i);  // 0 1 2 3 4
```

Half-open: `start` is included, `end` is excluded (Python-style).

---

## Math

```c
int a = 3, b = 7;
println("min:",   min(a, b));        // 3
println("max:",   max(a, b));        // 7
println("clamp:", clamp(12, 0, 10)); // 10
println("lerp:",  lerp(0.0f, 100.0f, 0.5f));  // 50.0

int x = 1, y = 2;
swap(x, y, int);   // x=2, y=1  (explicit type required — C11 has no typeof)
```

All are macros — avoid side-effecting arguments (`min(x++, y)` calls `x++` twice).

---

## Strings

```c
char *s = "hello.txt";

starts_with(s, "he");     // true
ends_with(s, ".txt");     // true
contains(s, "lo");        // true
equals(s, "hello.txt");   // true
```

All string functions take `const char *restrict` — the compiler can
auto-vectorize comparisons on large strings.

### Mutating functions (in-place)

```c
char buf[] = "   hello world   ";
char *t = trim(buf);       // "hello world"  (pointer into original buffer!)
lowercase(buf);            // "hello world"  (already lowercase)
uppercase(buf);            // "HELLO WORLD"
```

`trim()` modifies the buffer in place and returns a pointer into it.
Do **not** `free()` the returned pointer — free the original buffer.

---

## Random Numbers

```c
random_seed();                      // seed from current time (call once)
int   r  = random_int(1, 6);        // 1..6
float rf = random_float(0, 100);    // 0.0..100.0
```

Skip `random_seed()` for reproducible runs (useful in tests). Uses `rand()`
internally — not crypto-grade, but fine for games and demos.

---

## Files

```c
// Read entire file (allocates — you must free)
char *text = read_text_file("data.txt");
if (text) {
    println(text);
    free(text);
}

// Write / append (no allocation)
write_text_file("out.txt", "Hello!\n");
append_text_file("out.txt", "More text\n");
```

---

## Time

```c
long long start = current_time_ms();
sleep_ms(1500);                            // pause 1.5s
println("Elapsed:", (long)(current_time_ms() - start));
```

`current_time_ms()` is monotonic (won't jump if the system clock changes).
The absolute value is meaningless — only differences matter.

---

## Colors

```c
ec_init_colors();    // call once (enables ANSI on Windows console)

print_red();     println("Error!");
print_green();   println("Success!");
print_blue();    println("Info");
reset_color();    println("Back to normal");
```

Cross-platform: native ANSI on Linux/macOS, Virtual Terminal Processing on
Windows 10+.

---

## Assertions

```c
int age = -5;
assert_msg(age >= 0, "Age cannot be negative");
```

Always active (not disabled by `NDEBUG`). Prints file, line, condition, and
message to stderr, then calls `abort()`.

---

## Dynamic Arrays

Type-safe, growable arrays that work with any pointer type. Each array stores
a tiny header (length + capacity) behind the scenes. `da_push` amortizes to
O(1) per element.

### Basic usage

```c
int *nums = NULL;          // or: da_init(nums)
da_push(nums, 10);
da_push(nums, 20);
da_push(nums, 30);

assert(da_front(nums) == 10);       // first element
assert(da_back(nums) == 30);        // last element
assert(!da_empty(nums));            // not empty

da_for(i, nums) println(nums[i]);   // 10 20 30
println("len:", (long)da_len(nums)); // 3
da_free(nums);
```

### Insert and remove

```c
int *items = NULL;
da_push(items, 1);
da_push(items, 3);
da_insert(items, 1, 2);    // [1, 2, 3]
da_remove(items, 0);       // [2, 3]
da_free(items);
```

### Stack operations

```c
double *stack = NULL;
da_push(stack, 1.0);
da_push(stack, 2.0);
double top  = da_pop(stack);    // 2.0
double back = da_back(stack);   // 1.0
da_free(stack);
```

### Bulk push (high performance)

```c
int *nums = NULL;
int  more[] = {4, 5, 6, 7, 8};
da_push_many(nums, more, 5);   // single capacity check + memcpy
```

Use `da_push_many` instead of a `da_push` loop when you have a C array ready.
This is **much** faster — one `memcpy` vs N individual element writes.

### Sort, copy, resize

```c
int *vals = NULL;
da_push(vals, 3);
da_push(vals, 1);
da_push(vals, 2);

// Sort in place with built-in comparators
da_sort(vals, ec_cmp_int);         // 1, 2, 3
da_sort(vals, ec_cmp_int_desc);    // 3, 2, 1

// Deep copy — independent clone
int *clone = da_copy(vals, int);

// Resize: grow (zero-fill) or shrink (truncate)
da_resize(vals, 10);   // now 10 elements, new ones are zero
da_resize(vals, 2);    // truncate to 2 elements

da_free(vals);
da_free(clone);
```

### Pre-allocate and reuse

```c
char **names = NULL;
da_reserve(names, 1000);    // one allocation

for_range(i, 0, 100) {
    char buf[64];
    snprintf(buf, sizeof(buf), "item_%ld", i);
    da_push(names, strdup(buf));
}

da_clear(names);            // len=0, capacity stays — reuse buffer
da_free(names);
```

### Reference

| Macro | What it does |
|---|---|
| `da_init(arr)` | Set to NULL (explicit) |
| `da_empty(arr)` | True if empty |
| `da_len(arr)` | Element count |
| `da_cap(arr)` | Allocated capacity |
| `da_front(arr)` | First element |
| `da_back(arr)` | Last element |
| `da_push(arr, item)` | Append item |
| `da_push_many(arr, src, n)` | Bulk append from C array |
| `da_pop(arr)` | Remove and return last |
| `da_insert(arr, i, item)` | Insert at index |
| `da_remove(arr, i)` | Remove at index |
| `da_resize(arr, n)` | Resize (zero-fill on grow, truncate on shrink) |
| `da_copy(arr, type)` | Deep copy — free with `da_free` |
| `da_sort(arr, cmp)` | Sort in place (use `ec_cmp_*` comparators) |
| `da_clear(arr)` | Reset length (keep capacity) |
| `da_reserve(arr, n)` | Pre-allocate for n elements |
| `da_free(arr)` | Free everything |
| `da_for(i, arr)` | For-each loop (caches length) |

### Built-in sort comparators

| Comparator | Order |
|---|---|
| `ec_cmp_int` / `ec_cmp_int_desc` | int ascending / descending |
| `ec_cmp_long` / `ec_cmp_long_desc` | long ascending / descending |
| `ec_cmp_float` / `ec_cmp_float_desc` | float ascending / descending |
| `ec_cmp_double` / `ec_cmp_double_desc` | double ascending / descending |
| `ec_cmp_str` / `ec_cmp_str_desc` | `char*` alphabetical / reverse |

Write your own comparator with the standard `qsort` signature:
`int cmp(const void *a, const void *b)`.

### Performance notes

- `da_for` caches `da_len(arr)` into a local variable — the length check
  isn't re-evaluated every iteration.
- `da_push` and `da_insert` cache the header pointer after the grow check,
  eliminating redundant pointer arithmetic on the store path.
- The growth factor is 2x (from a minimum of 8), standard for amortized O(1).
- When `EC_DEBUG_MEMORY` is defined, `da_push`/`da_reserve` use plain
  `malloc`/`realloc` (not `ec_malloc`), so these allocations are not counted
  by the debug tracker. Use `ec_malloc` for your own allocations.

---

## Hash Map

Fast string-to-string hash map. Open addressing with linear probing (great
cache behavior), FNV-1a 64-bit hashing, power-of-2 capacity with bitmask
lookup.

### Basic usage

```c
ec_hashmap m = ec_hm_new();

ec_hm_set(&m, "name",  "Alice");
ec_hm_set(&m, "score", "42");
ec_hm_set(&m, "city",  "Paris");

println(ec_hm_get(&m, "name"));   // Alice
println(ec_hm_get(&m, "city"));   // Paris

if (ec_hm_has(&m, "score")) println("score exists");

ec_hm_del(&m, "score");           // remove key
println("len:", (long)ec_hm_len(&m));  // 2

ec_hm_free(&m);                   // free all keys, values, and slots
```

### Updating values

```c
ec_hm_set(&m, "score", "42");    // insert
ec_hm_set(&m, "score", "99");    // update — old value is freed
println(ec_hm_get(&m, "score")); // 99
```

### Iterating all entries

```c
ec_hashmap m = ec_hm_new();
ec_hm_set(&m, "a", "1");
ec_hm_set(&m, "b", "2");

for (size_t i = 0; i < m.cap; i++) {
    if (m.slots[i].state == EC_HM_OCCUPIED) {
        print(m.slots[i].key, "=", m.slots[i].val);
    }
}
ec_hm_free(&m);
```

### Reference

| Function | What it does |
|---|---|
| `ec_hm_new()` | Create empty map (16 slots) |
| `ec_hm_set(m, k, v)` | Insert or update (copies k, v) |
| `ec_hm_get(m, k)` | Get value or NULL |
| `ec_hm_has(m, k)` | Check existence |
| `ec_hm_del(m, k)` | Remove key (frees copies) |
| `ec_hm_len(m)` | Number of entries |
| `ec_hm_free(m)` | Free everything |

### Performance notes

- **FNV-1a 64-bit** — excellent distribution for short ASCII keys with
  minimal instruction count (1 XOR + 1 multiply per byte).
- **Power-of-2 capacity** — index is `hash & (cap - 1)`, which is a single
  AND instruction (no expensive modulo).
- **70% max load factor** — tables resize at 2x when `used * 10 >= cap * 7`,
  keeping probe chains short (average ~1.5 probes at 70%).
- **Tombstones** — deletion marks slots instead of shifting entries.
- **Linear probing** — cache-friendly sequential access within a probe chain.

---

## Arena Allocator

A bump-pointer allocator. Allocate thousands of small objects in nanoseconds,
free them all at once. No per-object bookkeeping, zero fragmentation.

```c
ec_arena arena = ec_arena_new(1024 * 1024);  // 1 MiB
if (!arena.buf) { println("Out of memory"); return 1; }

int  *nums = ec_arena_alloc(&arena, 100 * sizeof(int));
char *str  = ec_arena_alloc(&arena, 256);
double *vals = ec_arena_alloc_zero(&arena, 50 * sizeof(double));

ec_arena_free(&arena);  // everything freed at once
```

### Per-frame pattern

```c
ec_arena arena = ec_arena_new(10 * 1024 * 1024);

repeat(frame, 60) {
    Vec3 *positions = ec_arena_alloc(&arena, 10000 * sizeof(Vec3));
    simulate_frame(positions);
    ec_arena_reset(&arena);   // rewind — reuse same memory
}

ec_arena_free(&arena);
```

### Reference

| Function | What it does |
|---|---|
| `ec_arena_new(cap)` | Create arena with `cap` bytes |
| `ec_arena_alloc(a, sz)` | Bump-allocate `sz` bytes (8-byte aligned) |
| `ec_arena_alloc_zero(a, sz)` | Bump-allocate + zero-fill |
| `ec_arena_reset(a)` | Rewind bump pointer to start |
| `ec_arena_free(a)` | Free backing buffer, zero struct |
| `ec_arena_remaining(a)` | Bytes left before exhaustion |

---

## Pool Allocator

Fixed-size block allocator with O(1) alloc and free. Pre-allocate N blocks
of the same size, then serve them from an intrusive free list. Zero
fragmentation, zero per-object overhead beyond the block itself.

```c
typedef struct { float x, y; int hp; } Enemy;

ec_pool pool = ec_pool_new(sizeof(Enemy), 1000);
if (!pool.buf) { println("Out of memory"); return 1; }

Enemy *e1 = ec_pool_alloc(&pool);   // grab a free block
e1->hp = 100;

ec_pool_free_block(&pool, e1);      // return it
ec_pool_destroy(&pool);
```

### Pool vs Arena

| | Pool | Arena |
|---|---|---|
| Block size | Fixed | Variable |
| Free individual objects | Yes, O(1) | No |
| Alloc overhead | Pop from free list | Bump pointer |
| Best for | Entities, particles | Scratch data, frames |

### Reference

| Function | What it does |
|---|---|
| `ec_pool_new(sz, n)` | Create pool of `n` blocks, each `sz` bytes |
| `ec_pool_alloc(p)` | Take a free block (or NULL) |
| `ec_pool_free_block(p, blk)` | Return a block |
| `ec_pool_available(p)` | Count free blocks (O(n)) |
| `ec_pool_destroy(p)` | Free backing buffer |

---

## String Builder

A growable string buffer. Build strings incrementally without buffer-size
arithmetic, manual `realloc`, or `snprintf` gymnastics. The buffer is always
null-terminated — `.data` is always a valid C string.

### Basic usage

```c
ec_string s = string_new();

string_append(&s, "Hello, ");
string_append(&s, name);
string_appendf(&s, " — you are visitor #%d", count);
println(s.data);          // "Hello, Alice — you are visitor #42"

string_free(&s);
```

### Appending values

```c
ec_string s = string_new();
string_append_int(&s, 42);       // "42"
string_append_char(&s, ' ');
string_append_double(&s, 3.14);  // "42 3.14"
string_append_bool(&s, true);    // "42 3.14true"
string_clear(&s);
string_free(&s);
```

### One-shot formatted strings

```c
ec_string msg = string_fromf("%s v%d.%d", app_name, major, minor);
println(msg.data);
string_free(&msg);
```

### Reuse with string_clear

```c
ec_string s = string_new();
repeat(frame, 60) {
    string_appendf(&s, "Frame %ld\n", frame);
    // use s.data ...
    string_clear(&s);   // resets length to 0, keeps buffer
}
string_free(&s);
```

### Reference

| Function | What it does |
|---|---|
| `string_new()` | Create empty string (no allocation) |
| `string_append(s, str)` | Append C string |
| `string_append_char(s, c)` | Append single char |
| `string_append_int(s, n)` | Append int as decimal |
| `string_append_long(s, n)` | Append long as decimal |
| `string_append_float(s, f)` | Append float |
| `string_append_double(s, d)` | Append double |
| `string_append_bool(s, b)` | Append "true" / "false" |
| `string_appendf(s, fmt, ...)` | printf-style append |
| `string_fromf(fmt, ...)` | Create string from format |
| `string_clear(s)` | Reset to empty (keeps buffer) |
| `string_free(s)` | Free buffer, zero struct |
| `string_len(s)` | Current length |
| `string_empty(s)` | True if empty |
| `string_cstr(s)` | `const char*` (never NULL) |

### Performance notes

- **Doubling realloc** — same amortized-O(1) strategy as dynamic arrays.
- **No allocation until first append** — `string_new()` is zero-cost.
- **`string_clear` reuses the buffer** — faster than free + new for loops.

---

## CLI Arguments

No-dependency `argc`/`argv` parser.

```c
int main(int argc, char **argv) {
    ec_args_init(argc, argv);

    if (ec_args_has("--help") || ec_args_has("-h")) {
        println("Usage: mytool [--verbose] [--output FILE] <files...>");
        return 0;
    }

    bool verbose       = ec_args_has("--verbose") || ec_args_has("-v");
    const char *output = ec_args_val("--output");
    if (!output) output = ec_args_val("-o");

    for (int i = 0; i < ec_args_pos_count(); i++)
        print("  arg:", ec_args_pos(i));

    return 0;
}
```

### Reference

| Function | What it does |
|---|---|
| `ec_args_init(argc, argv)` | Capture args (call once) |
| `ec_args_has("--flag")` | `true` if flag is present |
| `ec_args_val("--key")` | Value after flag, or `NULL` |
| `ec_args_pos(i)` | i-th positional arg (0-indexed) |
| `ec_args_pos_count()` | Number of positional args |
| `ec_args_count()` | Raw `argc` |

No allocations, no copies — pointers into original `argv`.

---

## Memory Debugging

```c
#define EC_DEBUG_MEMORY
#include "eacy.h"

int main(void) {
    char *a = ec_malloc(100);
    char *b = ec_malloc(200);
    println("Live blocks:", ec_debug_alloc_count());  // 2

    ec_free(a);
    println("Live blocks:", ec_debug_alloc_count());  // 1

    ec_free(b);
    println("Live blocks:", ec_debug_alloc_count());  // 0
    return 0;
}
```

Define **before** the `#include`. This counts blocks, not bytes — a sanity
check, not a full leak detector.

---

## Complete Programs

### Number guessing game

```c
#include "eacy.h"

int main(void) {
    random_seed();
    int secret = random_int(1, 100);
    int guess = 0, tries = 0;

    println("Guess the number (1-100)!");
    while (guess != secret) {
        print("Your guess: ");
        guess = input_int();
        tries++;
        if (guess < secret)      println("Too low!");
        else if (guess > secret) println("Too high!");
    }
    print("You got it in", tries,
          tries == 1 ? "try!" : "tries!");
    return 0;
}
```

### Word counter with CLI args

```c
#include "eacy.h"

int main(int argc, char **argv) {
    ec_args_init(argc, argv);
    const char *path = ec_args_pos(0);
    if (!path) { println("Usage: wc <file>"); return 1; }

    char *text = read_text_file(path);
    if (!text) { print("Cannot open:", path); return 1; }

    int words = 0;
    bool inword = false;
    for (char *p = text; *p; p++) {
        if (isspace((unsigned char)*p)) inword = false;
        else if (!inword) { inword = true; words++; }
    }

    println("Words:", words);
    free(text);
    return 0;
}
```

### CSV to JSON with string builder

```c
#include "eacy.h"

int main(void) {
    char *csv = read_text_file("data.csv");
    if (!csv) { println("Cannot open data.csv"); return 1; }

    ec_string json = string_new();
    string_append(&json, "[");

    char *line = strtok(csv, "\n");
    bool first = true;
    while (line) {
        if (!first) string_append_char(&json, ',');
        first = false;

        string_appendf(&json, "{\"name\":\"%s\"}", trim(line));
        line = strtok(NULL, "\n");
    }

    string_append(&json, "]");
    println(json.data);

    string_free(&json);
    free(csv);
    return 0;
}
```

### Top-K words with hash map + sort

```c
#include "eacy.h"

int main(int argc, char **argv) {
    ec_args_init(argc, argv);
    const char *path = ec_args_pos(0);
    if (!path) { println("Usage: topk <file>"); return 1; }

    char *text = read_text_file(path);
    if (!text) { print("Cannot open:", path); return 1; }

    ec_hashmap counts = ec_hm_new();

    char *word = strtok(text, " \t\n\r.,;:!?\"'()");
    while (word) {
        lowercase(word);
        const char *cnt = ec_hm_get(&counts, word);
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", cnt ? atoi(cnt) + 1 : 1);
        ec_hm_set(&counts, word, buf);
        word = strtok(NULL, " \t\n\r.,;:!?\"'()");
    }

    // Collect into sortable array
    typedef struct { char *word; int count; } WC;
    WC *items = NULL;
    for (size_t i = 0; i < counts.cap; i++) {
        if (counts.slots[i].state == EC_HM_OCCUPIED) {
            WC w = {counts.slots[i].key, atoi(counts.slots[i].val)};
            da_push(items, w);
        }
    }

    // Sort by count descending
    qsort(items, da_len(items), sizeof(WC), ec_cmp_int_desc);

    da_for(i, items) {
        if (i >= 10) break;
        print(items[i].word, ":", items[i].count);
    }

    da_free(items);
    ec_hm_free(&counts);
    free(text);
    return 0;
}
```

---

## Naming Conventions

EaCy follows a two-tier naming rule:

**Everyday helpers — no prefix.** These are the functions you use in every
program: `print`, `scan`, `da_push`, `string_append`, `random_int`,
`starts_with`, `sleep_ms`, etc.

**Advanced or infrequent features — `ec_` prefix.** These signal "this does
something non-trivial": `ec_arena_new`, `ec_hm_set`, `ec_pool_alloc`,
`ec_malloc`, `ec_init_colors`, `ec_args_init`.

Types for advanced features also carry the `ec_` prefix (`ec_hashmap`,
`ec_string`, `ec_arena`, `ec_pool`) even when their everyday functions
don't (`string_append`, `string_free`).

This keeps the 95%-use-case API clean while avoiding namespace collisions
for the specialized 5%.

---

## Performance Notes

### What makes EaCy fast

| Technique | Where | Impact |
|---|---|---|
| `restrict` pointers | All string functions | Enables auto-vectorization |
| `_Generic` dispatch | `print`, `scan` | Zero runtime overhead |
| `static inline` | All functions | Inlined at `-O2` |
| Header caching | `da_push`, `da_insert`, `da_remove` | Eliminates redundant pointer arithmetic |
| Length caching | `da_for` | `da_len()` computed once |
| `memcpy` bulk ops | `da_push_many` | Single grow check + vectorized copy |
| Bitmask modulo | Hash map | `hash & (cap-1)` vs `%` |
| FNV-1a hash | Hash map | 1 XOR + 1 multiply per byte |
| Linear probing | Hash map | Cache-line-friendly |
| Intrusive free list | Pool allocator | O(1) alloc/free, zero overhead |
| Bump pointer | Arena allocator | 5-10 instructions per alloc |

### Things to watch

- **Dynamic array macros** evaluate `arr` multiple times — use named variables.
- **`da_for` is read-only** — don't mutate length during iteration.
- **Arena allocations are uninitialised** — use `ec_arena_alloc_zero` for zeros.
- **Pool block size is auto-aligned** to pointer width.
- **Hash map copies keys and values** — each `ec_hm_set` does 2 mallocs.
- **`print`/`scan` max 8 arguments.**

---

## Platform & Compiler Support

- **C11** or later (`_Generic` is a hard requirement)
- **GCC**, **Clang**, **MSVC** — CI-tested on all three
- **Windows**, **Linux**, **macOS**
- Also compiles as **C++** (wrapped in `extern "C"`)

---

## License

Public domain / CC0. Use it for anything.
