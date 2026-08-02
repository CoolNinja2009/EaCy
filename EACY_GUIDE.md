# EaCy — The C Library That Feels Like a Scripting Language

**One header. Zero dependencies. Compiles on GCC, Clang, and MSVC.**

Drop `eacy.h` next to your `.c` file and `#include "eacy.h"`. No build system,
no linking, no package manager.

EaCy is designed for **speed** — most functions compile down to a handful of
instructions, macros cache repeated lookups, and hot paths are kept
branch-free where possible.

---


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
18. [Logging](#logging)
19. [Stopwatch](#stopwatch)
20. [Benchmark](#benchmark)
21. [Memory Debugging](#memory-debugging)
22. [Complete Programs](#complete-programs)
23. [Naming Conventions](#naming-conventions)
24. [Performance Notes](#performance-notes)
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

A type-safe, macro-driven hash map for any key and value type. Open
addressing with linear probing, FNV-1a hashing, 70% load factor.
Keys and values are stored inline (shallow copy), so the map owns its data.

### Declaring a map

Use `hm(KeyType, ValueType)` — it expands to the internal `ec_hashmap` type.

```c
hm(int, int)    scores;    // int → int
hm(char*, float) prices;   // string → float
hm(MyKey, MyVal) custom;   // custom struct → struct
```

### Basic usage

```c
hm(int, int) scores;
hm_init(scores);                       // no allocation yet

hm_set(scores, 42, 100);
hm_set(scores, 7, 200);
hm_set(scores, 99, 300);
assert(hm_size(scores) == 3);

int val;
if (hm_get(scores, 42, &val))          // found → copies into val
    println("Score:", val);            // Score: 100

if (hm_contains(scores, 99))
    println("99 exists");

hm_remove(scores, 7);                  // remove key
assert(hm_size(scores) == 2);

hm_free(scores);                       // free everything
```

### String keys

When key type is `char*`, comparison and hashing use the string *content*
(strcmp / FNV-1a), not the pointer address. Keys MUST be `char*` lvalues —
pass a variable, not a string literal directly:

```c
hm(char*, float) prices;
hm_init(prices);

const char *apple  = "apple";          // lvalue — correct
hm_set(prices, apple, 1.29f);
// hm_set(prices, "apple", 1.29f);     // WRONG — literal treated as array

float price;
if (hm_get(prices, apple, &price))
    println("Price:", price);

hm_free(prices);
```

To set with a literal, cast it: `hm_set(prices, (char*)"apple", 1.29f)`.

### Clear and reuse

```c
hm_clear(scores);                      // keep buffer, reset count
hm_set(scores, 1, 10);                 // reuse existing memory
```

### Backward-compatible string→string API

The original `ec_hm_*` functions still work:

```c
ec_hashmap m = ec_hm_new();
ec_hm_set(&m, "name", "Alice");        // keys/values are strdup'd
println(ec_hm_get(&m, "name"));        // "Alice"
ec_hm_del(&m, "name");
ec_hm_free(&m);
```

Use this when you need the map to own string copies (free'd on remove/free).

### Reference — generic API

| Macro | What it does |
|---|---|
| `hm(K,V)` | Declare variable of type `ec_hashmap` |
| `hm_init(m)` | Zero-initialise (no allocation) |
| `hm_set(m, k, v)` | Insert or update (shallow copy) |
| `hm_get(m, k, v)` | Look up — copies value into `*v`, returns bool |
| `hm_contains(m, k)` | `true` if key exists |
| `hm_remove(m, k)` | Remove key (returns `true` if present) |
| `hm_clear(m)` | Remove all entries (keeps buffer) |
| `hm_free(m)` | Free slot table, zero struct |
| `hm_size(m)` | Number of entries |
| `hm_empty(m)` | `true` if empty |

### Reference — backward-compatible API

| Function | What it does |
|---|---|
| `ec_hm_new()` | Create string→string map |
| `ec_hm_set(m, k, v)` | Insert (strdup's k and v) |
| `ec_hm_get(m, k)` | Get value or NULL |
| `ec_hm_has(m, k)` | Check existence |
| `ec_hm_del(m, k)` | Remove (frees copies) |
| `ec_hm_len(m)` | Number of entries |
| `ec_hm_free(m)` | Free everything |

### Performance

- **FNV-1a 64-bit** — 1 XOR + 1 multiply per byte, excellent distribution.
- **Power-of-2 capacity** — `hash & (cap - 1)` (single AND, no modulo).
- **70% max load** — resize at 2×, average ~1.5 probes per lookup.
- **Tombstones** — deletions mark slots, preserving probe chains.
- **Linear probing** — cache-friendly sequential access.

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
| `string_len(s)` | Current length |
| `string_length(s)` | Alias for string_len |
| `string_empty(s)` | True if empty |
| `string_cstr(s)` | `const char*` (never NULL) |
| `string_capacity(s)` | Allocated capacity |
| `string_from(str)` | Create string from C string |
| `string_printf(fmt, ...)` | Create string from format |
| `string_insert(s, i, str)` | Insert at index |
| `string_remove(s, i, n)` | Remove n chars at index |
| `string_equals(a, b)` | True if contents match |
| `string_contains(s, str)` | True if substring found |
| `string_starts_with(s, str)` | True if starts with |
| `string_ends_with(s, str)` | True if ends with |
| `string_lowercase(s)` | Convert to lowercase |
| `string_uppercase(s)` | Convert to uppercase |
| `string_trim(s)` | Trim leading and trailing whitespace |

### Creating strings

```c
ec_string a = string_from("hello");            // copy of C string
ec_string b = string_printf("v%d.%d", 2, 1);   // printf-style
println(a.data, b.data);  // hello v2.1
string_free(&a); string_free(&b);
```

### Inspecting and comparing

```c
ec_string s = string_from("hello world");
string_contains(&s, "world");       // true
string_starts_with(&s, "hello");    // true
string_ends_with(&s, "world");      // true
string_equals(&s, &s2);             // compare two ec_strings
string_free(&s);
```

### Mutation in place

```c
ec_string s = string_from("  HELLO  ");
string_trim(&s);        // "HELLO"
string_lowercase(&s);   // "hello"
string_uppercase(&s);   // "HELLO"

string_insert(&s, 0, "say ");       // "say HELLO"
string_remove(&s, 4, 5);            // "say "
string_free(&s);
```
 
 ### Performance notes

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


## Logging

Colored, timestamped logging macros that write to stderr. Each message is
prefixed with a `[HH:MM:SS]` timestamp and a color-coded level label.

```c
ec_init_colors();    // call once (enables ANSI on Windows)

log_info("Server starting on port", 8080);
log_warn("Config not found, using defaults");
log_error("Connection refused:", host);
log_debug("Request #", req_id, "took", elapsed, "ms");
```

Output:

```
[14:32:05] INFO  Server starting on port 8080
[14:32:06] WARN  Config not found, using defaults
[14:32:07] ERROR Connection refused: db.example.com
[14:32:08] DEBUG Request #42 took 15 ms
```

Colors: cyan (INFO), yellow (WARN), red (ERROR), dim/gray (DEBUG).

Define `EC_NO_COLORS` before including `eacy.h` to disable ANSI codes:

```c
#define EC_NO_COLORS
#include "eacy.h"
```

| Macro | Level | Color |
|---|---|---|
| `log_info(...)` | Informational | Cyan |
| `log_warn(...)` | Warning | Yellow |
| `log_error(...)` | Error | Red |
| `log_debug(...)` | Debug trace | Dim |

Arguments use the same type-dispatch as `print()` — no format strings.

---

## Stopwatch

A thin wrapper around `current_time_ms()` for ad-hoc timing. Monotonic,
wall-clock-safe, sub-millisecond precision.

```c
ec_timer t = timer_start();
do_expensive_work();
println("Took", (long)timer_elapsed_ms(&t), "ms");
println("Took", timer_elapsed_seconds(&t), "s");

timer_restart(&t);     // reset to now
```

| Function | What it does |
|---|---|
| `timer_start()` | Capture current time, return `ec_timer` |
| `timer_restart(t)` | Reset `t` to current time |
| `timer_elapsed_ms(t)` | Milliseconds since start/restart |
| `timer_elapsed_seconds(t)` | Seconds since start/restart (double) |

---

## Benchmark

Zero-fuss benchmarking macros. Wrap any block — EaCy prints the elapsed time.

```c
benchmark("qsort 1e6 ints") {
    qsort(data, 1000000, sizeof(int), ec_cmp_int);
}
// → qsort 1e6 ints: 42 ms
```

Average over N runs:

```c
benchmark_avg("FFT 4096", 100) {
    fft_4096(signal);
}
// → FFT 4096: 0.127 ms avg
```

| Macro | What it does |
|---|---|
| `benchmark(label) { ... }` | Time block once, print result |
| `benchmark_avg(label, n) { ... }` | Time block `n` times, print average |
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
`starts_with`, `sleep_ms`, `log_info`, `timer_start`, `benchmark`, etc.

**Advanced or infrequent features — `ec_` prefix.** These signal "this does
something non-trivial": `ec_arena_new`, `ec_hm_set`, `ec_pool_alloc`,
`ec_malloc`, `ec_init_colors`, `ec_args_init`.

Types for advanced features also carry the `ec_` prefix (`ec_hashmap`,
`ec_string`, `ec_arena`, `ec_pool`, `ec_timer`) even when their everyday
functions don't (`string_append`, `string_free`, `timer_start`).

This keeps the 95%-use-case API clean while avoiding namespace collisions
for the specialized 5%.

---

## Performance Notes

### What makes EaCy fast

| Technique | Where | Impact |
|---|---|---|
| `restrict` pointers | All string functions | Enables auto-vectorization |
| `_Generic` dispatch | `print`, `scan`, `log_*` | Zero runtime overhead |
| `static inline` | All functions | Inlined at `-O2` |
| Header caching | `da_push`, `da_insert`, `da_remove` | Eliminates redundant pointer arithmetic |
| Length caching | `da_for` | `da_len()` computed once |
| `memcpy` bulk ops | `da_push_many` | Single grow check + vectorized copy |
| Bitmask modulo | Hash map | `hash & (cap-1)` vs `%` |
| FNV-1a hash | Hash map | 1 XOR + 1 multiply per byte |
| Linear probing | Hash map | Cache-line-friendly |
| Inline key/value storage | Generic hash map | One allocation, no per-entry malloc |
| Intrusive free list | Pool allocator | O(1) alloc/free, zero overhead |
| Bump pointer | Arena allocator | 5-10 instructions per alloc |
| Monotonic clock | Stopwatch / Benchmark | No wall-clock jumps |

### Things to watch

- **Dynamic array macros** evaluate `arr` multiple times — use named variables.
- **`da_for` is read-only** — don't mutate length during iteration.
- **Arena allocations are uninitialised** — use `ec_arena_alloc_zero` for zeros.
- **Pool block size is auto-aligned** to pointer width.
- **Generic hash map does shallow copies** — `hm_set` memcpy's keys and values.
  For `hm(char*, V)`, the caller must keep string keys alive.
- **Backward-compat `ec_hm_set` strdup's keys and values** — 2 mallocs per entry.
- **`print`/`scan`/`log_*` max 8 arguments.**
- **String keys in `hm(char*,V)` must be `char*` lvalues, not literals.**

---

## Platform & Compiler Support

- **C11** or later (`_Generic` is a hard requirement)
- **GCC**, **Clang**, **MSVC** — CI-tested on all three
- **Windows**, **Linux**, **macOS**
- Also compiles as **C++** (wrapped in `extern "C"`)

---

## License

MIT License
