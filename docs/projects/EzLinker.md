# EzLinker Subproject Documentation

[EzPacker Documentation Index](../index.md) > [Subprojects](EzLinker.md) > **EzLinker** | [Doxygen API Reference](../doxygen/index.html)

---

## 1. Overview & Architectural Role

`EzLinker` is the automated linker driver, system toolchain detector, and exception runtime library of EzPacker. It bridges the gap between binary object files produced by `EzCompiler` (`EzCodeEmitter`) and final native executables or dynamic libraries.

### Core Responsibilities
1. **Automated Linker Detection**: Discovers the preferred system linker (`lld-link`, `link.exe`, `ld.lld`, `ld.bfd`, `ld.gold`, or compiler drivers `clang`/`gcc`) across Windows and Linux.
2. **Freestanding Linker Driver**: Produces freestanding executables by default, decoupling from C standard runtime libraries (`-lc`, `msvcrt.lib`, `ucrt.lib`, etc.) while cleanly supporting user-specified library linking.
3. **Freestanding Exception Runtime (`EzExceptionRuntime`)**: A fully standalone, C-free exception management runtime. Completely eliminates `<setjmp.h>`, `msvcrt.dll`, libc `longjmp`, `<cstdlib>`, and `<iostream>`, providing custom architecture-specific `__ez_setjmp` and `__ez_longjmp` implementations, thread-local exception frame unwinding, RTTI-based multicatch dispatch, and diagnostic reporting.
4. **Standalone Standard Library (`EzStd.mir`)**: Provides freestanding implementations of fundamental memory operations (`@memcpy`, `@memset`, `@bzero`, `@memmove`, `@memcmp`) and process termination (`@abort`, `@exit`) written directly in generic MIR.
5. **Unified Command-Line Interface (`ez-ld`)**: A cross-platform CLI supporting standard flags (`-o`, `-L`, `-l`, `--shared`, `--static`, `--target`, `--entry`, `--subsystem`) with full dry-run introspection (`--dry-run`).

```
+-------------------------------------------------------------------------------+
|                            EzLinker Pipeline                                  |
|                                                                               |
|  Object Files (.obj / .o) + User Libs                                         |
|        |                                                                      |
|        v                                                                      |
|  +-------------------------------------+                                      |
|  |       SystemLinkerDetector          | <-- Target Triple & Host System      |
|  | - Classify Linker Executable        |                                      |
|  | - Scan PATH, MSVC, SDK, LLVM Paths  |                                      |
|  +-------------------------------------+                                      |
|        |                                                                      |
|        v                                                                      |
|  +-------------------------------------+                                      |
|  |         EzLinkerDriver              |                                      |
|  | - Freestanding by default           |                                      |
|  | - Inject compiler-rt / libgcc       |                                      |
|  | - Inject EzExceptionRuntime         |                                      |
|  | - Synthesize Native Linker Command  |                                      |
|  +-------------------------------------+                                      |
|        |                                                                      |
|        v                                                                      |
|  System Linker Invocation (or --dry-run introspection)                        |
|        |                                                                      |
|        v                                                                      |
|  Native Executable (.exe / ELF binary) or Shared Library (.dll / .so)         |
+-------------------------------------------------------------------------------+
```

---

## 2. Subsystems

### 2.1 System Linker Detection (`include/SystemLinkerDetector.h`)

`SystemLinkerDetector` automates the discovery and classification of native toolchains:

- **Supported Linker Kinds (`SystemLinkerKind`)**:
  - `LldLink`: LLVM's MSVC-compatible linker (`lld-link.exe`)
  - `MsvcLink`: Microsoft Visual C++ linker (`link.exe`)
  - `LldElf`: LLVM ELF linker (`ld.lld`)
  - `GnuBfd`: GNU BFD linker (`ld.bfd`, `ld`)
  - `GnuGold`: GNU Gold linker (`ld.gold`)
  - `ClangDriver`: Clang compiler driver (`clang`, `clang++`)
  - `GccDriver`: GCC compiler driver (`gcc`, `g++`)

- **Detection Strategy**:
  1. **User Override**: If `--linker <path>` is supplied, validates existence and classifies the binary.
  2. **Windows Targets**: Prioritizes `lld-link` -> `link.exe` (via `VSINSTALLDIR`, `vswhere.exe`, and standard Visual Studio 2022 paths) -> `ld.lld` -> MinGW `ld.exe` -> `clang.exe` -> `gcc.exe`.
  3. **Linux/Unix Targets**: Prioritizes `ld.lld` -> `ld.gold` -> `ld.bfd` / `ld` -> `clang` -> `gcc`.
  4. **Library Path Resolution**: Automatically locates Windows Kits SDK paths (`ucrt`, `um`) and MSVC runtime lib directories when invoking MSVC-compatible linkers.

---

### 2.2 Linker Driver Engine (`include/EzLinkerDriver.h`)

`EzLinkerDriver` handles command synthesis and subprocess execution:

- **MSVC / LLD-Link Synthesis**:
  - Generates `/nologo`, `/out:<file>`, `/libpath:<dir>`, `<lib>.lib`
  - Injects `kernel32.lib`, `user32.lib` by default (C runtime libraries like `msvcrt.lib`, `libcmt.lib`, `ucrt.lib` are decoupled and not linked unless requested)
  - Configures subsystem (`/subsystem:console` or `/subsystem:windows`) and entry point
- **ELF / GNU Synthesis**:
  - Generates `-o <file>`, `-L<dir>`, `-l<lib>`
  - Decoupled from default C libraries (`-lc`, `-lm`, `-lpthread`, `-ldl` are not injected unless explicitly requested)
  - Injects legalizer helpers (`-lgcc` / `-lcompiler_rt`) when needed
  - Configures `-shared`, `-static`, and `-e <entry>`
- **EzExceptionRuntime Automatic Linking**:
  - Locates `libEzExceptionRuntime.a` or `EzExceptionRuntime.lib` in the local build or install tree
  - Automatically appends search directory and library flags unless `--no-exception-rt` is specified
- **Freestanding Standard Library (`EzStd.mir`)**:
  - Provides freestanding implementations of `@memcpy`, `@memset`, `@bzero`, `@memmove`, `@memcmp`, `@abort`, `@exit` in native MIR, compiled directly to target object code without any external libc requirement.

---

### 2.3 Freestanding Exception Handling Runtime (`runtime/EzExceptionRuntime.h`)

The runtime provides a completely custom, freestanding setjmp/longjmp exception substrate independent of host C runtimes:

```cpp
extern "C" {
    struct EzJmpBuf {
        uint64_t regs[32];
    };

    struct EzExceptionFrame {
        struct EzJmpBuf jmpBuf;
        struct EzExceptionFrame *prev;
        void *currentPayload;
        const void *currentRtti;
        int isCaught;
    };

    int __ez_setjmp(struct EzJmpBuf *buf);
    void __ez_longjmp(struct EzJmpBuf *buf, int val) __attribute__((noreturn));

    int __ez_try_enter(struct EzExceptionFrame *frame);
    void __ez_try_leave(struct EzExceptionFrame *frame);
    void __ez_throw(void *payload, const void *rtti);
    int __ez_catch_matches(const void *thrownRtti, const void *filterRtti);
    void *__ez_get_current_exception(void);
    const void *__ez_get_current_rtti(void);
    const void *__ez_get_default_rtti(void);
    const void *__ez_get_default_payload(void);
    const char *__ez_get_rtti_type_name(const void *rtti);
    uint64_t __ez_get_rtti_type_id(const void *rtti);
    struct EzExceptionFrame *__ez_get_top_frame(void);
}
```

- **Freestanding `__ez_setjmp` & `__ez_longjmp`**:
  - Implemented in naked inline assembly for Microsoft Win64 (`x86_64`), System V AMD64 (`x86_64`), and RISC-V 64 (`riscv64`).
  - Preserves all callee-saved registers (including `rbx`, `rsp`, `rbp`, `r12`-`r15`, and Win64 non-volatile XMM registers `xmm6`-`xmm15`; or RISC-V `ra`, `sp`, `s0`-`s11`, `fs0`-`fs11`) into `EzJmpBuf`.
  - Completely eliminates `<setjmp.h>`, `msvcrt.dll`, libc `longjmp`, `<cstdlib>`, and `<iostream>`.
- **Guaranteed RTTI Attachment & Default Fallback**:
  - Whenever `__ez_throw(payload, rtti)` is invoked, an RTTI descriptor is guaranteed to be attached. If `rtti == NULL`, the runtime automatically attaches the canonical default RTTI descriptor `__ez_default_rtti` (`typeId = computeTypeId("EzDefaultException")`, `typeName = "EzDefaultException"`).
  - If `payload == NULL`, the runtime provides the canonical default exception payload `__ez_default_payload` (`RichExceptionPayload`).
  - Accessors `__ez_get_rtti_type_name(rtti)` and `__ez_get_rtti_type_id(rtti)` allow safe, C-compatible introspection without requiring C++ `std::string_view` layouts.
- **Thread-Local Stack Unwinding**:
  - Each `TRY` block registers an `EzExceptionFrame` on the thread-local stack via `__ez_try_enter()`.
  - `__ez_throw()` saves payload and RTTI pointers, pops the active frame, and performs `__ez_longjmp()` back to the handler.
  - Normal exit or catch block completion unlinks the frame via `__ez_try_leave()`.
- **Multicatch & Catch-All Filtering**:
  - `__ez_catch_matches(thrownRtti, filterRtti)` evaluates type compatibility using `RttiTypeDescriptor::isA()`.
  - A `nullptr` filter acts as a catch-all (`catch (...)`).
  - Thrown default exceptions match either catch-all filters or specific `__ez_default_rtti` filters.
- **Uncaught Exception Handling**:
  - If `__ez_throw()` is called with an empty exception stack, it safely invokes `__builtin_trap()` / infinite trap loop without relying on hosted `std::abort()`.


---

## 3. Command-Line Reference (`ez-ld`)

```bash
ez-ld [options] <inputs...>
```

### Options

| Flag | Description |
| :--- | :--- |
| `-o <path>`, `--output <path>` | Destination binary executable or library path. |
| `--linker <path>` | Explicit override for system linker executable. |
| `--target <triple>` | Target architecture and OS triple (e.g. `x86_64-pc-windows-msvc`). |
| `-L<dir>` | Appends a library search directory (repeatable). |
| `-l<name>` | Links the specified library (repeatable). |
| `--entry <sym>` | Sets a custom entry point symbol name. |
| `--subsystem <sub>` | Windows subsystem (`console`, `windows`). |
| `--shared` | Generates a shared object (`.so`) or dynamic link library (`.dll`). |
| `--static` | Directs static linking against runtime libraries. |
| `-v`, `--verbose` | Emits detailed detection and command synthesis logs. |
| `--dry-run` | Prints the synthesized linker command line without executing it. |
| `--nodefaultlibs` | Disables automatic injection of C standard libraries. |
| `--no-compiler-rt` | Disables automatic injection of compiler-rt / libgcc helpers. |
| `--no-exception-rt` | Disables automatic linking of `EzExceptionRuntime`. |
| `-Xlinker <arg>` | Passes raw arguments verbatim to the underlying system linker. |

---

## 4. CMake Targets

| Target | Type | Description |
| :--- | :--- | :--- |
| `EzLinkerLib` | Shared Library | Linker detection and command synthesis library. |
| `EzExceptionRuntime` | Static Library | Low-level SjLj exception unwinding runtime. |
| `ez-ld` / `EzLinker` | Executable | Linker driver executable. |
