# EzLibStdC Subproject Documentation

[EzPacker Documentation Index](../index.md) > [Subprojects](EzLibStdC.md) > **EzLibStdC** | [Doxygen API Reference](../doxygen/index.html)

---

## 1. Overview & Architectural Role

`EzLibStdC` is the native ISO C11 standard runtime library and C run-time (CRT) startup facility for EzPacker. It provides a permissive, MIT-licensed standard C library implementation designed to compile cleanly under EzPacker's embedded C frontend (`ezc`) and link seamlessly via the automated linker driver (`ez-ld`).

```
                    +------------------------------------+
                    |        C11 Source (.c, .i)         |
                    |    #include <stdio.h>, <stdlib.h>  |
                    +------------------------------------+
                                      |
                                      v
                    +------------------------------------+
                    |       ezc -fhosted -O2 file.c      |
                    | Resolves headers in include/c11std |
                    +------------------------------------+
                                      |
                                      v
                    +------------------------------------+
                    |   Object File (.o / .obj)          |
                    +------------------------------------+
                                      |
                                      v
                    +------------------------------------+
                    |       ez-ld --hosted file.o        |
                    | - Injects libEzLibStdC.a / .lib    |
                    | - Links CRT startup (crt_start.o)  |
                    | - Detects host/target system linker|
                    +------------------------------------+
                                      |
                                      v
                    +------------------------------------+
                    |      Native Executable Binary      |
                    | (Linux ELF64 or Windows PE/COFF)   |
                    +------------------------------------+
```

### Key Design Goals
- **Permissive MIT Licensing**: Zero GPL or LGPL copyleft viral restrictions. Applications compiled with EzPacker and linked against `EzLibStdC` can be distributed commercially or under any open-source license without legal encumbrance.
- **Cross-Platform Host Abstraction**: Unified support for Microsoft Windows (`Win32` / `PE-COFF`) and Linux (`POSIX` / `ELF64`) through a clean OS abstraction layer (`os/os_support.h`).
- **Complete CRT Startup Lifecycle**: Implements `_start` (Linux ELF64) and `mainCRTStartup` (Windows PE/COFF) to initialize the process environment, invoke standard `main(int argc, char **argv)`, and exit cleanly via `exit()`.
- **Decoupled from Host System Libc**: Compiles independently without requiring host GCC/glibc, Clang, or MSVC UCRT headers, ensuring deterministic, reproducible builds across development environments.

---

## 2. Directory & Module Structure

The subproject is organized under `EzLibStdC/`:

```
EzLibStdC/
├── CMakeLists.txt              # Static library target definition (C11)
└── src/
    ├── assert/
    │   └── assert.c            # Assertion failure handler (__assert_fail)
    ├── ctype/
    │   └── ctype.c             # Character classification and conversion
    ├── errno/
    │   └── errno.c             # Error code definition (errno)
    ├── math/
    │   └── math.c              # Basic mathematical functions (abs, sqrt, sin, cos, pow)
    ├── os/
    │   ├── crt_start.c         # C runtime entry points (_start, mainCRTStartup)
    │   └── os_support.h        # OS-level abstraction (Win32 API vs POSIX syscalls)
    ├── stdio/
    │   └── stdio.c             # Formatted I/O, string formatting, file streams
    ├── stdlib/
    │   └── stdlib.c            # Memory allocation, sort, search, conversions, exit
    ├── string/
    │   └── string.c            # Memory and string manipulation functions
    └── time/
        └── time.c              # Basic time and clock measurement
```

---

## 3. Bundled ISO C11 Headers (`include/c11std/`)

`EzLibStdC` provides all 28 standard C11 headers in `include/c11std`. When `-fhosted` is specified (or when compiling in hosted mode), `ezc` injects this directory into the preprocessor include path:

| Header | Description | Core Types & Functions |
| :--- | :--- | :--- |
| `<assert.h>` | Diagnostic assertions | `assert()`, `__assert_fail()` |
| `<complex.h>` | Complex number arithmetic | `complex`, `_Complex_I`, `I` |
| `<ctype.h>` | Character classification & conversion | `isalpha()`, `isdigit()`, `isalnum()`, `isspace()`, `isupper()`, `islower()`, `toupper()`, `tolower()` |
| `<errno.h>` | Error number constants | `errno`, `EDOM`, `ERANGE`, `EINVAL`, `ENOMEM`, etc. |
| `<fenv.h>` | Floating-point environment | `fenv_t`, `fexcept_t`, `FE_ALL_EXCEPT` |
| `<float.h>` | Floating-point limits & characteristics | `FLT_RADIX`, `FLT_MANT_DIG`, `DBL_MAX`, `DBL_MIN`, `DBL_EPSILON` |
| `<inttypes.h>` | Format specifiers for fixed-width integers | `PRId64`, `PRIu64`, `PRIx64`, `SCNd32`, `intmax_t`, `uintmax_t` |
| `<iso646.h>` | Alternative operator spellings | `and`, `or`, `not`, `xor`, `bitand`, `bitor` |
| `<limits.h>` | Fundamental integer ranges | `CHAR_BIT`, `INT_MIN`, `INT_MAX`, `LLONG_MIN`, `LLONG_MAX` |
| `<locale.h>` | Localization definitions | `struct lconv`, `setlocale()`, `localeconv()` |
| `<math.h>` | Mathematical functions | `fabs()`, `sqrt()`, `sin()`, `cos()`, `pow()`, `ceil()`, `floor()`, `exp()`, `log()` |
| `<setjmp.h>` | Non-local jumps | `jmp_buf`, `setjmp()`, `longjmp()` (interfaced with EzExceptionRuntime) |
| `<signal.h>` | Signal handling | `sig_atomic_t`, `signal()`, `raise()`, `SIGINT`, `SIGTERM`, `SIGSEGV` |
| `<stdalign.h>` | Alignment macros | `alignas`, `alignof`, `__alignas_is_defined` |
| `<stdarg.h>` | Variable argument lists | `va_list`, `va_start()`, `va_arg()`, `va_copy()`, `va_end()` |
| `<stdatomic.h>` | Atomic operations | Interfaced with frontend lowering (`ND_CAS`, `ND_EXCH`) |
| `<stdbool.h>` | Boolean type and constants | `bool`, `true`, `false`, `__bool_true_false_are_defined` |
| `<stddef.h>` | Standard type definitions | `size_t`, `ptrdiff_t`, `nullptr_t`, `offsetof`, `NULL` |
| `<stdint.h>` | Exact-width integer types | `int8_t`..`int64_t`, `uint8_t`..`uint64_t`, `uintptr_t`, `intmax_t` |
| `<stdio.h>` | Input/output facilities | `printf()`, `sprintf()`, `snprintf()`, `vsnprintf()`, `puts()`, `putchar()`, `fopen()`, `fclose()` |
| `<stdlib.h>` | General utilities | `malloc()`, `free()`, `realloc()`, `calloc()`, `atoi()`, `strtol()`, `qsort()`, `bsearch()`, `exit()`, `abort()` |
| `<stdnoreturn.h>` | Non-returning function specifier | `noreturn` |
| `<string.h>` | String & memory manipulation | `strlen()`, `strcpy()`, `strncpy()`, `strcmp()`, `strstr()`, `memcpy()`, `memset()`, `memmove()` |
| `<tgmath.h>` | Type-generic math macros | Generic wrappers for `<math.h>` and `<complex.h>` |
| `<threads.h>` | ISO C11 multithreading | `thrd_t`, `mtx_t`, `cnd_t`, `tss_t`, `thrd_success` |
| `<time.h>` | Date and time utilities | `time_t`, `clock_t`, `struct tm`, `time()`, `clock()`, `difftime()`, `CLOCKS_PER_SEC` |
| `<uchar.h>` | Unicode character utilities | `char16_t`, `char32_t` |
| `<wchar.h>` | Wide character facilities | `wchar_t`, `wint_t`, `struct tm` |
| `<wctype.h>` | Wide character classification | `wctype_t`, `wint_t`, `iswalpha()`, `iswdigit()` |

---

## 4. Implemented Subsystems & APIs

### 4.1 Input / Output (`stdio/stdio.c`)
- **Formatted String Output**:
  - `vsnprintf(char *str, size_t size, const char *format, va_list ap)`: Full format specifier parser supporting `%d`, `%i`, `%u`, `%x`, `%X`, `%p`, `%s`, `%c`, `%%`, width padding, zero padding (`%08x`), left alignment (`%-10s`), precision (`%.4f`, `%.10s`), and length modifiers (`%lld`, `%ld`, `%zu`, `%h`).
  - `snprintf`, `sprintf`, `printf`, `fprintf`, `vfprintf`.
- **Character & String Output**:
  - `puts(const char *s)`: Emits string followed by newline to `stdout`.
  - `putchar(int c)`, `fputc(int c, FILE *stream)`, `fputs(const char *s, FILE *stream)`.
- **Standard Streams**:
  - Static predefined streams `stdin` (fd 0), `stdout` (fd 1), `stderr` (fd 2).
- **Buffered & File Operations (Stubs & Baselines)**:
  - `fopen`, `fclose`, `fflush`, `fread`, `fwrite`, `fseek`, `ftell`, `rewind`, `feof`, `ferror`, `clearerr`, `perror`.

### 4.2 General Utilities & Memory Management (`stdlib/stdlib.c`)
- **Dynamic Memory Allocation**:
  - `malloc(size_t size)`: Allocates blocks from the OS heap (delegates to `HeapAlloc(GetProcessHeap())` on Windows or `mmap` on Linux).
  - `calloc(size_t num, size_t size)`: Allocates zero-initialized memory blocks.
  - `realloc(void *ptr, size_t new_size)`: Resizes memory blocks preserving existing content.
  - `free(void *ptr)`: Returns memory blocks to the OS heap allocator.
- **Integer Conversion**:
  - `atoi(const char *s)`, `atol(const char *s)`: Parses base-10 signed integers with leading whitespace handling.
  - `strtol(const char *nptr, char **endptr, int base)`: General multi-base integer parser (supports base 2 through 36, auto-detection of `0x` hex and `0` octal).
  - `strtoul(const char *nptr, char **endptr, int base)`: Unsigned integer variant.
- **Algorithms**:
  - `qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *))`: In-place Hoare quicksort implementation with median-of-three partitioning.
  - `bsearch(const void *key, const void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *))`: $O(\log N)$ binary search algorithm.
- **Process Lifecycle**:
  - `exit(int status)`: Terminates process returning status code.
  - `abort(void)`: Triggers immediate process termination via trap / `ExitProcess`.

### 4.3 String Manipulation (`string/string.c`)
- **Memory Operations**:
  - `memcpy(void *dest, const void *src, size_t n)`: Byte-level memory copy.
  - `memset(void *s, int c, size_t n)`: Byte fill operation.
  - `memmove(void *dest, const void *src, size_t n)`: Overlap-safe memory copy.
  - `memcmp(const void *s1, const void *s2, size_t n)`: Lexicographical memory comparison.
  - `memchr(const void *s, int c, size_t n)`: Locates byte within memory buffer.
- **Null-Terminated String Functions**:
  - `strlen(const char *s)`: String length measurement.
  - `strcpy`, `strncpy`, `strcat`, `strncat`: Copy and concatenate functions.
  - `strcmp`, `strncmp`: Lexicographical string comparisons.
  - `strchr`, `strrchr`, `strstr`: Substring and character search functions.

### 4.4 Mathematics (`math/math.c`)
- **Absolute Values**: `abs(int x)`, `labs(long x)`, `llabs(long long x)`, `fabs(double x)`.
- **Power & Roots**: `sqrt(double x)` (Newton-Raphson approximation), `pow(double base, double exp)`.
- **Trigonometry**: `sin(double x)`, `cos(double x)` (Taylor series approximations).
- **Rounding**: `ceil(double x)`, `floor(double x)`, `fmod(double x, double y)`.

### 4.5 Character Classification (`ctype/ctype.c`)
- Fast ASCII character classification: `isalpha`, `isdigit`, `isalnum`, `isspace`, `isupper`, `islower`, `isxdigit`, `ispunct`, `isprint`, `isgraph`, `iscntrl`.
- Case conversion: `toupper(int c)`, `tolower(int c)`.

### 4.6 Diagnostics (`assert/assert.c`)
- `__assert_fail(const char *assertion, const char *file, unsigned int line, const char *function)`:
  Emits formatted assertion failure messages directly to `stderr`:
  ```
  Assertion failed: (rec != NULL), function analyze_sensor, file 10_hosted_c11_pipeline.c, line 48.
  ```
  Followed by immediate process termination via `abort()`.

### 4.7 Timing (`time/time.c`)
- `time(time_t *timer)`: Retrieves elapsed seconds since Epoch (1970-01-01 00:00:00 UTC).
- `clock(void)`: Returns CPU clock ticks elapsed since process start.
- `difftime(time_t time1, time_t time0)`: Returns difference in seconds as `double`.

---

## 5. OS Abstraction Layer (`os/os_support.h`)

EzLibStdC encapsulates all OS interactions behind an inline abstraction layer:

```c
#if defined(_WIN32) || defined(_WIN64) || defined(__WIN64__)
#define EZ_OS_WINDOWS 1
// Direct Win32 API imports
__declspec(dllimport) void __stdcall ExitProcess(unsigned int uExitCode);
__declspec(dllimport) EZ_HANDLE __stdcall GetStdHandle(unsigned long nStdHandle);
__declspec(dllimport) int __stdcall WriteFile(EZ_HANDLE hFile, const void *lpBuffer,
                                             unsigned long nNumberOfBytesToWrite,
                                             unsigned long *lpNumberOfBytesWritten, void *lpOverlapped);
__declspec(dllimport) EZ_HANDLE __stdcall GetProcessHeap(void);
__declspec(dllimport) void *__stdcall HeapAlloc(EZ_HANDLE hHeap, unsigned long dwFlags, size_t dwBytes);
__declspec(dllimport) void *__stdcall HeapReAlloc(EZ_HANDLE hHeap, unsigned long dwFlags, void *lpMem, size_t dwBytes);
__declspec(dllimport) int __stdcall HeapFree(EZ_HANDLE hHeap, unsigned long dwFlags, void *lpMem);
#else
#define EZ_OS_LINUX 1
#include <sys/syscall.h>
#include <unistd.h>
#endif
```

### Abstraction Primitives
- `ez_os_exit(int code)`: Invokes `ExitProcess` (Windows) or `SYS_exit` (Linux).
- `ez_os_write_stdout(const void *buf, size_t len)`: Writes to standard output handle or `SYS_write(1, ...)`.
- `ez_os_write_stderr(const void *buf, size_t len)`: Writes to standard error handle or `SYS_write(2, ...)`.
- `ez_os_alloc(size_t size)`: Heap allocation via `HeapAlloc` or `mmap`.
- `ez_os_realloc(void *ptr, size_t new_size)`: Reallocation via `HeapReAlloc`.
- `ez_os_free(void *ptr)`: Release via `HeapFree`.

---

## 6. C Runtime Startup Entry (`os/crt_start.c`)

When building a standalone hosted executable, the linker requires a defined entry point that bootstraps the C execution environment before entering `main()`:

```c
// Forward declaration of user main
extern int main(int argc, char **argv);

#if defined(_WIN32) || defined(_WIN64) || defined(__WIN64__)
// Microsoft PE/COFF Console Entry Point
void mainCRTStartup(void) {
    char *dummy_argv[] = { "program", 0 };
    int ret = main(1, dummy_argv);
    exit(ret);
}
#else
// System V ELF64 Entry Point
void _start(void) {
    char *dummy_argv[] = { "program", 0 };
    int ret = main(1, dummy_argv);
    exit(ret);
}
#endif
```

---

## 7. Toolchain & Linker Integration (`ezc` & `ez-ld`)

### Compilation with Hosted Environment
```bash
# Ingest ISO C11 code with full hosted headers enabled
ezc -fhosted -O2 my_program.c -o my_program.o
```

### Automated Linking with `ez-ld`
```bash
# Automatically detects system linker and links EzLibStdC + CRT startup
ez-ld --hosted my_program.o -o my_program.exe

# Or explicitly pass the path to custom EzLibStdC build:
ez-ld --hosted --libstdc-path build/lib/EzLibStdC.lib my_program.o -o my_program.exe
```

When `--hosted` is specified:
1. `EzLinkerDriver` searches the build or installation library directory for `libEzLibStdC.a` (Linux) or `EzLibStdC.lib` (Windows).
2. It automatically appends the library path `-L<dir>` and links `-lEzLibStdC` (or `EzLibStdC.lib`).
3. For Windows MSVC linkers, default system imports (`kernel32.lib`) are linked to resolve `ExitProcess`, `GetStdHandle`, and heap functions.
4. For Linux ELF linkers, raw syscalls provide standalone operation without requiring host `glibc`.

---

## 8. Test Suite & End-to-End Verification

The hosted C11 runtime is thoroughly validated by dedicated tests located in `tests/EzCompilerEndToEndTests/c11_hosted/`:

| Test File | Verified Standard Capabilities |
| :--- | :--- |
| `test_malloc.c` | Dynamic heap allocation (`malloc`, `free`), pointer verification, data integrity |
| `test_qsort.c` | In-place array sorting (`qsort`), callback comparison dispatch, ascending order check |
| `test_debug.c` | Assertion failure tracking, string formatting via `sprintf`, struct member manipulation |
| `test_div.c` | Signed and unsigned division, modulo operations, boundary conditions |
| `test_arr.c` | Multi-dimensional array access, pointer decay, arithmetic indexing |
| `test_field.c` | Struct field offset calculations, nested struct alignments, value preservation |
| `test_sz.c` | `sizeof` evaluation on standard types, structs, and arrays |
| `test_exec.c` | CRT startup execution, `main()` return code forwarding, clean process exit |

Run all hosted tests via CMake/CTest:
```bash
ctest --test-dir build -R "EzCompilerEndToEndTests" --output-on-failure
```
