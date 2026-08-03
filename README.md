# EaCy

<p align="center">
  <b>Write cleaner, faster, and more expressive C.</b><br>
  A modern, header-only C11 utility library with zero dependencies.
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C-C11-blue.svg">
  <img src="https://img.shields.io/badge/Header-Only-success">
  <img src="https://img.shields.io/badge/License-MIT-green">
  <img src="https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-orange">
  <img src="https://img.shields.io/badge/Compiler-GCC%20%7C%20Clang%20%7C%20MSVC-red">
</p>

---

## Features

* Type-safe `print()` and `scan()` (no format strings)
* Python-inspired loop helpers
* Dynamic arrays
* String builder
* Generic hash maps
* Arena & pool allocators
* File utilities
* Command runner helpers
* Random number helpers
* Terminal colors
* Logging, benchmarking & stopwatch
* Memory debugging
* CLI argument parser
* Optional `EaCyP.h` dynamic-value layer
* Optional `EaCyP.h` simple module helpers

---

## Quick Example

```c
#include "eacy.h"

int main(void)
{
    println("Hello, EaCy!");

    int *nums = NULL;

    da_push(nums, 10);
    da_push(nums, 20);
    da_push(nums, 30);

    da_for(i, nums)
        println(nums[i]);

    da_free(nums);

    return 0;
}
```

---

## EaCyP Example

```c
#include "EaCyP.h"

module(math);
export int add_two(int x) { return x + 2; }

int main(void)
{
    var a, b, c;
    a = b = c = V(12);

    say("answer:", add_two(integer(a)));
    return 0;
}
```

`EaCyP.h` is an optional dynamic-value layer on top of EaCy. It keeps the
project C11-compatible while adding `var`, `V(...)`, `say(...)`, `set(...)`,
`set_all(...)`, truthiness, numeric conversion helpers, and simple module
helpers like `module(math);` and `export`.

---

## Installation

Simply copy `eacy.h` into your project and include it.

```c
#include "eacy.h"
```

For the dynamic layer, copy both headers and include:

```c
#include "EaCyP.h"
```

No build system.

No linking.

No dependencies.

---

## Documentation

Complete documentation is available in the project documentation.

It includes:

* Printing & Input
* Loops
* Math
* Strings
* Random Numbers
* File Utilities
* Command Runner
* Dynamic Arrays
* Hash Maps
* Arena Allocator
* Pool Allocator
* String Builder
* CLI Arguments
* Logging
* Stopwatch
* Benchmarking
* Memory Debugging
* EaCyP Dynamic Values
* EaCyP Simple Modules
* Complete Example Programs

---

## Goals

EaCy aims to make C:

* Easier to write
* Easier to read
* Less repetitive
* Still 100% C

It doesn't replace the language - it removes common boilerplate.

---

## License

Released under the **MIT License**.
