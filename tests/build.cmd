@echo off
REM build.cmd - compile all EaCy stress tests (Windows / MSVC or GCC)
REM Usage: build          (uses gcc by default)
REM        build msvc     (uses cl.exe)
REM        build clean    (remove .exe files)

setlocal enabledelayedexpansion

if "%1"=="clean" (
    del /q *.exe 2>nul
    echo Cleaned.
    exit /b 0
)

set CC=gcc
set CFLAGS=-std=c11 -O2 -Wall -Wextra -pedantic
set LDFLAGS=

if "%1"=="msvc" (
    set CC=cl
    set CFLAGS=/nologo /std:c11 /O2 /W4
    set LDFLAGS=
)

set FAILED=0

for %%f in (
    test_print_scan
    test_loops_math
    test_strings
    test_da
    test_hashmap
    test_string_builder
    test_arena
    test_pool
    test_time_stopwatch
    test_random_files
    test_command_runner
    test_eacyp
    test_logging_benchmark
    test_cli
    test_stress
) do (
    echo.
    echo === %%f ===
    %CC% %CFLAGS% %%f.c -o %%f.exe %LDFLAGS%
    if !errorlevel! neq 0 (
        echo BUILD FAILED: %%f
        set FAILED=1
    ) else (
        echo Running %%f.exe...
        %%f.exe
        if !errorlevel! neq 0 (
            echo TEST FAILED: %%f
            set FAILED=1
        )
    )
)

echo.
if %FAILED%==1 (
    echo SOME TESTS FAILED
    exit /b 1
) else (
    echo ALL TESTS PASSED
)
