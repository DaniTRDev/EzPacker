# EzPacker: Technical TODOs, Unimplemented Features & Optimization Roadmap

[EzPacker Documentation Index](index.md) > **TODOs & Optimization Roadmap**

---

## 1. Overview

This document catalogs identified technical debt, incomplete or stubbed implementations, language and runtime limitations, and concrete performance optimization opportunities across all EzPacker subprojects.

Items are organized by architectural subsystem and categorized by priority:
- **P1 (High)**: Core functionality required for complete language / runtime compliance or correctness.
- **P2 (Medium)**: Architectural enhancements, performance optimizations, and feature expansions.
- **P3 (Low / Future)**: Long-term architectural explorations, new hardware targets, and developer tooling.

---

## 2. Standard Runtime & CRT (`EzMusl`)

> **Migration Note**: Legacy `EzLibStdC` (`STD-01`..`STD-03`: OS file descriptors, `mallocng` heap allocator, and floating-point `vsnprintf` formatting) has been replaced by vendored **musl 1.2.5** (`EzMusl`). Remaining items below track Windows Win32 platform parity extensions for `EzMusl`.

### 2.1 Windows Win32 Platform Parity Extensions (`EzMusl/win32/`)
- **[P2] Win64 `setjmp`/`longjmp` Register Context (`win32_stubs.c`)**:
  - Replace the stub `__sigsetjmp` / C-fallback `setjmp` with Win64 callee-saved register preservation (`rbx`, `rbp`, `rdi`, `rsi`, `r12`–`r15`, `rsp`, `rip`, `xmm6`–`xmm15`) for hosted C `setjmp`/`longjmp` on Windows.
- **[P2] Windows Multithreading Runtime (`<threads.h>` / `<pthread.h>`)**:
  - Back `pthread_create`, `pthread_join`, `thrd_create`, `mtx_*`, and `cnd_*` on Windows with Win32 `CreateThread` / `WaitOnAddress` / `WakeByAddressAll` and per-thread TLS slots for `__ez_win32_get_tp()`.
- **[P3] Windows Directory & Signal Support**:
  - Implement `SYS_getdents64` over `FindFirstFileA` / `FindNextFileA` (`<dirent.h>`) and Win32 console signal handlers (`SetConsoleCtrlHandler`).

---

## 3. C Frontend & Semantic Lowering (`EzCompiler/CFrontend`)

### 3.1 Type System & Aggregate Layout
- **[P1] Native Aggregate Representation in MIR**:
  - In `CFrontendTypeMapper.cpp`, aggregate structs and unions with non-power-of-two or large sizes are flattened into byte arrays (`MirType::getArray(i8, size)`) or clamped to scalar integers.
  - Struct member accesses calculate raw byte offsets (`ND_MEMBER`).
  - **Optimization**: Retain structured aggregate types in MIR (`MirType::getStruct(...)`) with member offset lists. This enables Scalar Replacement of Aggregates (SROA) and field-sensitive alias analysis in middle-end optimization passes.
- **[P2] IEEE 80-Bit Extended / 128-Bit Quad Precision `long double`**:
  - `TY_LDOUBLE` currently aliases directly to 64-bit `f64`.
  - On x86-64, implement true 80-bit x87 extended precision or 128-bit IEEE quad-precision float support in `FlexFloat`, type mapper, and register lowerer.
- **[P2] Full Designated Initializers in Nested Aggregates**:
  - Extend nested struct and array constant emission in `ND_INIT_LIST` to support complex member path overwrites and sparse array index designated initializers.

### 3.2 Advanced C11 Language Features
- **[P2] Dynamic Stack Frame Cleanup for VLAs and `alloca`**:
  - `ND_VLA_PTR` lowers to `MirInstructionOpCode::DALLOC`. Ensure function epilogues accurately restore the stack pointer (`rsp` / `sp`) from the frame pointer (`rbp` / `s0`) even in functions with dynamic stack allocations and multiple return blocks.
  - Provide direct builtin lowering for `__builtin_alloca()`.
- **[P3] ISO C11 `_Atomic` Type Qualifiers & Operations**:
  - Provide complete frontend mapping for `_Atomic` type qualifiers, lowering atomic loads and stores to acquire/release memory barriers or target-specific atomic instructions (`LOCK` prefix on x86, `.aq` / `.rl` bits on RISC-V).

---

## 4. Intermediate Representation & Middle-End Optimizations (`EzMir`)

### 4.1 Optimization Passes
- **[P1] Global Value Numbering (GVN) & Common Subexpression Elimination (CSE)**:
  - Current `MirPeepholePass` operates strictly at the local instruction or basic-block window.
  - Implement a dominator-tree-based GVN pass to eliminate redundant computations, common subexpressions, and redundant loads across basic blocks.
- **[P2] Loop-Invariant Code Motion (LICM)**:
  - Implement loop detection and natural loop construction in `CodeFlowAnalysisPass`.
  - Hoist invariant instructions (calculations with operands defined outside the loop body) into a loop preheader block.
- **[P2] Dead Block Elimination & CFG Simplification**:
  - Remove unreachable blocks identified post dominator tree construction.
  - Merge linear basic block chains where block $A$ has an unconditional branch to $B$ and $B$ has only predecessor $A$.
  - Thread jumps when a basic block consists solely of an unconditional branch to another label.
- **[P2] Memory Disambiguation & Alias Analysis**:
  - Implement a basic alias analysis pass (TBAA or flow-insensitive points-to analysis) to identify whether memory loads and stores can alias.
  - Enable load-folding and dead store elimination across basic block boundaries when pointers are proven disjoint.

---

## 5. Target Lowering & Backend Optimization (`EzTriple` & `EzTargets`)

### 5.1 Register Allocator (`MirRegisterAllocatorPass`)
- **[P1] Loop-Depth Weighted Spill Costs**:
  - Currently, spill costs in Chaitin-Briggs graph coloring are uniform or estimated based on definition/use counts.
  - Incorporate loop nesting depth: multiply spill cost by $10^{\text{depth}}$ so values used inside tight inner loops are prioritized for physical registers.
- **[P2] Live Range Splitting**:
  - When a virtual register interferes with all available physical registers across a large function, split the live range across control-flow edges instead of spilling the variable throughout the entire function.
- **[P2] Cross-Block Rematerialization**:
  - Detect constants and trivial expressions (e.g. `MOV %r, 0` or address of global variables) that can be recomputed at low cost rather than spilled to stack memory and reloaded.

### 5.2 Instruction Selection & Target Emitter
- **[P2] x86-64 Complex Memory Addressing Synthesis**:
  - Enhance ISel pattern trees to synthesize full `[base + index*scale + disp32]` addressing modes in a single maximal munch pass across shifted arithmetic patterns.
- **[P2] RISC-V Bitmanip & Vector Extensions**:
  - Add declarative IDF/ISF definitions for RISC-V Bitmanip (Zba, Zbb, Zbc, Zbs) extensions (e.g. `SH1ADD`, `SH2ADD`, `ANDN`, `ORN`, `XNOR`, `ROL`, `ROR`).
  - Expand RVV (Vector) lowering for 128-bit and 256-bit SIMD operations.
- **[P3] New Target Architectures**:
  - **AArch64 (ARM64)**: Implement 64-bit ARM target descriptors, calling conventions (AAPCS64), instruction encodings, and Mach-O/ELF64 object writers.
  - **AVX-512**: Support 512-bit `ZMM` registers and EVEX prefix encoding on x86-64.

---

## 6. Linking, Debug Information & Object Emitter (`EzLinker` & `EzCodeEmitter`)

### 6.1 Debug Information Generation ✅ Implemented
- **[Implemented] DWARF v5 Emission (Linux ELF64)**:
  - `Dwarf5Emitter` generates `.debug_info`, `.debug_abbrev`, `.debug_line`, `.debug_str`, `.debug_line_str`, and `.debug_frame` sections for x86-64 and RISC-V 64 (`-g` / `-g1` / `-g2`, `--debug-format=dwarf`).
- **[Implemented] CodeView / PDB Information Emission (Windows PE/COFF)**:
  - `CodeViewEmitter` generates `.debug$T` and `.debug$S` (CV8 C13) sections with `SECREL`/`SECTION` relocations, and `ez-ld -g` passes `/DEBUG` for PDB generation.
  - See the **[Debugging Information Guide](debug_information.md)** for architecture, CLI flags, and workflows.

### 6.2 Native Linker Engine
- **[P2] Direct Internal Object Linking**:
  - `EzLinkerDriver` currently invokes external system linkers (`lld-link`, `link.exe`, `ld.lld`, `ld`).
  - Implement a direct, built-in binary linker in `EzLinker` capable of resolving relocations, laying out sections, and emitting standalone ELF64 executables and PE/COFF `.exe` files directly without relying on external toolchain installations.
- **[P3] Link-Time Optimization (LTO)**:
  - Allow emitting serialized MIR bytecode into dedicated object sections (`.ez_mir`), enabling cross-module inlining and dead code elimination at link time.

---

## 7. Web Explorer & Developer Workbench (`web/`)

### 7.1 Features & Interactivity
- **[P2] WebAssembly Compilation of `ezc`**:
  - Compile the `ezc` core and target backends to WebAssembly (Wasm) via Emscripten.
  - Enable running the complete compilation pipeline client-side in the browser without requiring a running Node.js backend.
- **[P2] Interactive CFG Editing & Block Highlighting**:
  - Enable bidirectional selection: clicking an instruction in Monaco highlights its corresponding basic block in `CfgViewer` and its register lifetime in `LivenessViewer`.
- **[P3] Pass Performance Flame Graph**:
  - Render an interactive flame graph of pass durations, highlighting which passes consume the largest fraction of total compilation time across large test cases.
- **[P3] Multi-File Project Workspace**:
  - Support multi-file C/MIR projects with `#include` and cross-module symbol resolution in the browser workbench.

---

## 8. Summary Table of Action Items

| ID | Subsystem | Action Item | Priority | Target Milestone |
| :--- | :--- | :--- | :--- | :--- |
| `STD-01` | `EzMusl` | Win64 callee-saved `setjmp`/`longjmp` register save/restore on Windows | P2 | v1.1 |
| `STD-02` | `EzMusl` | Win32 `<threads.h>` / `pthread` runtime (`CreateThread` + `WaitOnAddress`) | P2 | v1.2 |
| `STD-03` | `EzMusl` | Win32 `<dirent.h>` (`SYS_getdents64` via `FindFirstFileA`) | P3 | v1.2 |
| `FE-01` | `EzCompiler` | Model native aggregate struct/union types in MIR | P1 | v1.1 |
| `FE-02` | `EzCompiler` | True 80-bit/128-bit `long double` support | P2 | v1.2 |
| `OPT-01` | `EzMir` | Implement Global Value Numbering (GVN) pass | P1 | v1.1 |
| `OPT-02` | `EzMir` | Implement Loop-Invariant Code Motion (LICM) | P2 | v1.2 |
| `OPT-03` | `EzTriple` | Incorporate loop-depth weighting in register allocation | P1 | v1.1 |
| `DBG-01` | `EzCodeEmitter` | Emit DWARF v5 line and symbol tables for Linux ELF64 | ✅ Done | v1.1 |
| `DBG-02` | `EzCodeEmitter` | Emit CodeView / PDB debug sections for Windows | ✅ Done | v1.2 |
| `TGT-01` | `EzTargets` | Declarative RISC-V Bitmanip (Zba/Zbb/Zbc/Zbs) extension support | P2 | v1.2 |
| `TGT-02` | `EzTargets` | AArch64 (ARM64) Target Architecture Backend | P3 | v2.0 |
| `WEB-01` | `web` | Compile `ezc` to WebAssembly for offline in-browser execution | P2 | v1.2 |
