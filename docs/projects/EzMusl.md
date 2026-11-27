# EzMusl Subproject Documentation

[EzPacker Documentation Index](../index.md) > [Subprojects](EzMusl.md) > **EzMusl** | [Doxygen API Reference](../doxygen/index.html)

---

## 1. Overview & Architectural Role

`EzMusl` integrates the official **musl 1.2.5** C standard library (vendored under `third_party/musl/`) into EzPacker via `EzCMK` (`EzCMK_ConfigureLib(NAME "EzMusl" BUILD_TYPE STATIC ...)`). It replaces the legacy custom `EzLibStdC` runtime and `include/c11std` headers with a complete, standards-compliant ISO C11 / POSIX.1-2008 libc for both **Linux (ELF64)** and **Windows (PE/COFF)**.

```
                    +------------------------------------------+
                    |           C11 Source (.c, .i)            |
                    |   #include <stdio.h>, <stdlib.h>, ...    |
                    +------------------------------------------+
                                         |
                                         v
                    +------------------------------------------+
                    |          ezc -fhosted -O2 file.c         |
                    | 1. include/musl-compat (chibicc stdarg)  |
                    | 2. include/musl (musl 1.2.5 + bits/*)    |
                    +------------------------------------------+
                                         |
                                         v
                    +------------------------------------------+
                    |        Object File (.o / .obj)           |
                    +------------------------------------------+
                                         |
                                         v
                    +------------------------------------------+
                    |          ez-ld --hosted file.o           |
                    | - Linux ELF64: crt1.o + libEzMusl.a      |
                    | - Windows COFF: libEzMusl.a + kernel32   |
                    +------------------------------------------+
                                         |
                                         v
                    +------------------------------------------+
                    |        Native Executable Binary          |
                    |   (Linux ELF64 or Windows PE/COFF .exe)  |
                    +------------------------------------------+
```

### Key Design Goals
- **Permissive MIT Licensing**: `musl` 1.2.5 is MIT-licensed, keeping EzPacker's end-to-end toolchain completely free of GPL/LGPL runtime restrictions.
- **Zero External Build Tool Dependencies**: Arch/internal headers (`bits/alltypes.h`, `bits/syscall.h`, `version.h`) are generated in pure CMake (`EzMusl/cmake/GenerateMuslHeaders.cmake`) without requiring `sed`, `sh`, or `awk` on Windows.
- **Cross-Platform Linux & Windows Support**:
  - **Linux (`x86_64` / `riscv64` ELF64)**: Compiles upstream `musl` C + architecture assembly overrides (`.s`/`.S`) and stages standalone `crt1.o` (`_start` -> `__libc_start_main`) alongside `libEzMusl.a`.
  - **Windows (`x86_64` PE/COFF)**: Compiles portable C `musl` sources with a self-contained Win32 syscall translation layer (`win32_syscall.c`), `VirtualAlloc`/`VirtualFree` memory mapping (`win32_mman.c`), PE/COFF CRT startup (`win32_crt.c` defining `mainCRTStartup`), and COFF-compatible alias handling (`win32/include/features.h`).
- **Seamless `chibicc` (`ezc`) Frontend Compatibility**: Provides a minimal `compat_include/stdarg.h` overlay and `__chibicc__` / LLP64 guards in generated `bits/alltypes.h` and staged `inttypes.h` so `ezc -fhosted` parses and lowers musl headers cleanly.

---

## 2. Directory & Module Structure

```
third_party/musl/               # Unmodified upstream musl 1.2.5 source tree
├── arch/                       # Upstream arch definitions (x86_64, riscv64, generic, ...)
├── crt/                        # Upstream CRT startup sources (crt1.c, ...)
├── include/                    # Upstream ISO C11 & POSIX public headers
└── src/                        # Upstream libc implementation (stdio, stdlib, mallocng, math, ...)

EzMusl/                         # EzPacker EzCMK integration & platform abstraction
├── CMakeLists.txt              # EzCMK_ConfigureLib(NAME "EzMusl" BUILD_TYPE STATIC ...)
├── cmake/
│   └── GenerateMuslHeaders.cmake  # Pure-CMake generator for bits/alltypes.h, bits/syscall.h, version.h
├── compat_include/
│   └── stdarg.h                # chibicc __va_area__ / __builtin_reg_class compatibility header
└── win32/                      # Windows PE/COFF platform abstraction layer
    ├── include/
    │   ├── features.h          # COFF weak/strong alias & visibility handling
    │   ├── pthread_arch.h      # Win32 thread pointer accessor (__ez_win32_get_tp)
    │   └── syscall_arch.h      # 64-bit syscall_arg_t & __ez_win32_syscall dispatch
    └── src/
        ├── win32_crt.c         # mainCRTStartup / WinMainCRTStartup, GetCommandLineA/envp/auxv setup
        ├── win32_mman.c        # VirtualAlloc / VirtualFree / VirtualProtect mmap/munmap/mremap
        ├── win32_stubs.c       # Single-process cancellation stubs, COFF entry aliases, ELF linker symbol stubs
        └── win32_syscall.c     # Linux SYS_* -> kernel32.dll Win32 API syscall translation
```

---

## 3. Header Generation & Staging

At CMake configure time, `EzMusl/cmake/GenerateMuslHeaders.cmake` generates:
1. `bits/alltypes.h` from `third_party/musl/arch/<ARCH>/bits/alltypes.h.in` + `include/alltypes.h.in`:
   - Includes a `__chibicc__` guard defining `__va_elem` and `__builtin_va_list[1]` for `ezc`.
   - Includes a `#if defined(_WIN32) && !defined(__LP64__)` guard mapping `_Addr`, `_Int64`, `_Reg` to `long long` and `__LONG_MAX` to `0x7fffffffL` for Windows LLP64 compilers (MinGW/MSVC), while preserving LP64 (`long` = 64-bit) for Linux and `chibicc` (`__LP64__`).
2. `bits/syscall.h` from `arch/<ARCH>/bits/syscall.h.in` (duplicating `__NR_*` as `SYS_*`).
3. `src/internal/version.h` from `third_party/musl/VERSION` (`1.2.5`).

Public headers are staged to `<build>/bin/include/musl/` and `<build>/bin/include/musl-compat/` by the `EzMusl_StageHeaders` target, which `EzCompilerLib` and `ezc` depend on automatically.

---

## 4. Platform Architecture: Linux vs. Windows

### 4.1 Linux (ELF64)
- Compiles all `src/*/*.c` and `src/malloc/mallocng/*.c` with architecture-specific `.c`/`.s`/`.S` overrides under `src/*/<ARCH>/`.
- Compiles `third_party/musl/crt/crt1.c` (with `-DCRT`) and stages it to `<build>/lib/crt1.o`.
- Uses native Linux `syscall` instructions (`arch/x86_64/syscall_arch.h`) and `%fs:0` TLS (`arch/x86_64/pthread_arch.h`).

### 4.2 Windows (PE/COFF)
- **Syscall Translation (`win32/src/win32_syscall.c`)**: Maps `SYS_read`, `SYS_write`, `SYS_readv`, `SYS_writev`, `SYS_open`, `SYS_openat`, `SYS_close`, `SYS_lseek`, `SYS_ioctl` (`TIOCGWINSZ`), `SYS_stat`/`SYS_fstat`/`SYS_newfstatat`, `SYS_access`, `SYS_unlink`, `SYS_rename`, `SYS_mkdir`/`SYS_rmdir`, `SYS_getcwd`/`SYS_chdir`, `SYS_dup`/`dup2`/`dup3`, `SYS_clock_gettime`/`SYS_gettimeofday`, `SYS_nanosleep`, `SYS_getpid`/`SYS_gettid`, `SYS_getrandom`, and `SYS_exit`/`SYS_exit_group` onto `kernel32.dll` APIs.
- **64-bit Syscall Arguments**: `win32/include/syscall_arch.h` overrides `syscall_arg_t` and `__scc(X)` to `long long` so 64-bit pointers are never truncated to 32-bit `long` under Windows LLP64.
- **Memory Management (`win32/src/win32_mman.c`)**: Implements `mmap`, `munmap`, `mremap`, `mprotect`, and `madvise` directly over `VirtualAlloc`, `VirtualFree`, and `VirtualProtect` so `mallocng` operates with native 64-bit addresses.
- **CRT Startup (`win32/src/win32_crt.c`)**: Implements `mainCRTStartup` and `WinMainCRTStartup`, builds `argc`/`argv`/`envp`/`auxv` from `GetCommandLineA()` and `GetEnvironmentStringsA()`, sets up the thread pointer for `__pthread_self()`, and invokes `__libc_start_main`.
- **COFF Archive Symbol Resolution (`win32/include/features.h`)**: Public aliases (`weak_alias(__foo, foo)`) emit strong COFF aliases so `ar` indexes them in `libEzMusl.a`, while ~26 internal optional hook files compile with `EZ_MUSL_WEAK_HOOKS=1` to retain weak overridability.
- **Zero `libgcc` Dependency**: Compiled on MinGW with `-ffreestanding -nostdinc -fno-stack-protector -mno-stack-arg-probe`, allowing `libEzMusl.a` to link cleanly under MSVC `link.exe`, `lld-link`, MinGW `ld.exe`, and `gcc`/`clang`.

---

## 5. Toolchain & Linker Usage

### Compiling with `ezc -fhosted`
```bash
# Host target (Windows PE/COFF or Linux ELF64)
ezc -fhosted -O2 app.c -o app.o

# Cross-targeting Linux ELF64 from Windows
ezc -fhosted -O2 --target x86_64-linux-gnu app.c -o app.o
```
Override discovery paths if needed:
- `EZPACKER_MUSL_DIR`: Overrides `<build>/bin/include/musl`.
- `EZPACKER_MUSL_COMPAT_DIR`: Overrides `<build>/bin/include/musl-compat`.

### Linking with `ez-ld --hosted`
```bash
# Auto-discovers libEzMusl.a / EzMusl.lib (and crt1.o on ELF targets)
ez-ld --hosted app.o -o app.exe

# Explicit musl archive override
ez-ld --hosted --musl-path build/lib/libEzMusl.a app.o -o app.exe
```
Override discovery paths if needed:
- `EZPACKER_MUSL_PATH`: Explicit path to `libEzMusl.a` / `EzMusl.lib`.
- `EZPACKER_MUSL_CRT1_PATH`: Explicit path to `crt1.o` (ELF targets).
