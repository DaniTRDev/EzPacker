# YARPGen Differential Testing & Stress Compilation Suites

This document describes the design, execution, and triage workflow of the **Intel YARPGen Test Suites** in **EzPacker**.

---

## 🎯 Purpose and Philosophy

The primary objective of these suites is to mathematically test and verify **EzPacker's compiler pipeline (`ezc`)**:
1. **Miscompilations / Semantic Bugs**: The compiled code generates invalid machine instructions that silently produce incorrect mathematical or logical results. The **Reference Compiler (GCC or Clang)** acts as the source of truth to detect any execution discrepancy.
2. **Compiler Panics & Crashes**: Unhandled opcodes, invalid instruction selections, internal assertion failures, or memory exhaustion during MIR lowering and code emission.
3. **Multi-File Scalability & Throughput**: Benchmarking compilation latency, throughput (files/sec, LOC/sec), and pipeline pass stability across fleets of independent translation units.
4. **Monolithic AST / Stress Scalability**: Straining middle-end and backend algorithms (register allocation, liveness analysis, CFG traversals) on massive translation units (+3,000 LOCs).

To accomplish this with mathematical rigor and zero false positives, the suites utilize **[Intel YARPGen](https://github.com/intel/yarpgen)** to synthesize randomly structured, UB-free C programs that heavily exercise scalar optimizations, nested loops, bitwise logic, and type conversions.

---

## 🏗️ Architecture & Dual-Mode Workflow

```mermaid
flowchart TD
    subgraph Suites["1. Test Workloads"]
        B["Bulk Corpus (25 Small/Med Files)<br/>~3,000 LOC Total"]
        S["mega_stress.c (Single Big File)<br/>3,623 LOCs, 148 Kernels"]
    end

    subgraph RefPipeline["2. Reference Pipeline (Source of Truth)"]
        B & S --> REF_CC["GCC / Clang -O3"]
        REF_CC --> REF_EXE["Reference Executables"]
        REF_EXE --> REF_HASH["Ground Truth Checksums: ref_hash"]
    end

    subgraph EzcPipeline["3. EzPacker Pipeline"]
        B & S --> EZC["ezc -O0, -O1, -O2, -Os"]
        EZC --> EZC_OBJ["Object Files (.obj / .o)"]
        EZC_OBJ --> LINK["Host Linker / Driver"]
        LINK --> EZC_EXE["EzPacker Executables"]
        EZC_EXE --> EZC_HASH["EzPacker Checksums: ezc_hash"]
    end

    subgraph Verif["4. Verification & Metrics"]
        REF_HASH & EZC_HASH --> CMP{"ezc_hash == ref_hash?"}
        CMP -->|Equal| PASS["PASS: Semantically Correct!"]
        CMP -->|Mismatch| FAIL["MISCOMPILATION DETECTED<br/>(Checksum Divergence)"]
        EZC --> PERF["Throughput (LOC/s) & Pass Bottleneck Profiling"]
    end
```

---

## 🚀 The Two Primary Test Suites

EzPacker provides two distinct test suites designed for different testing and benchmarking goals:

### 1. Bulk File Compilation Suite (Multiple Small & Medium Files)

Exercises the compiler on a diverse fleet of independent, small-to-medium translation units (25 files in `tests/YarpgenDifferentialTests/bulk_corpus/`, each ~80–250 LOC, ~3,000 LOC total):
- **Compilation Mode**: Verifies that `ezc` compiles all 25 files without crashes, assertions, or frontend errors across optimization levels (`-O0`, `-O1`, `-O2`, `-Os`). Measures compilation throughput (LOC/sec and files/sec).
- **Differential Verification Mode**: Compiles each test with both GCC/Clang (`-O3`) and `ezc`, links with companion drivers (`driver_001.c` .. `driver_010.c`), executes, and asserts that `ezc_checksum == ref_checksum`.

#### How to Run:
```bash
# 1. Run via CMake Custom Target (Differential Verification if GCC/Clang found, else compilation):
cmake --build build --target bulk-ezc

# 2. Run Pure Compilation Throughput Benchmark across -O0, -O1, -O2, -Os:
cmake --build build --target bulk-ezc-compile

# 3. Run in CTest:
ctest --test-dir build -R "EzCompilerYarpgen_BulkCompilation" --output-on-failure
ctest --test-dir build -R "EzCompilerYarpgen_BulkDifferential" --output-on-failure

# 4. Standalone Python CLI:
python tests/YarpgenDifferentialTests/yarpgen_suite.py bulk --compile-only --opt-levels O0,O2
python tests/YarpgenDifferentialTests/yarpgen_suite.py bulk --ref-compiler gcc --opt-levels O0
```

---

### 2. Single Big File Stress Suite (+3k LOCs)

Stresses the compiler pipeline using a single monolithic translation unit containing **3,623 lines of C code** and **148 distinct computationally intensive kernels**:
- **Monolithic Source File**: [`tests/YarpgenDifferentialTests/mega_stress.c`](mega_stress.c)
- **Companion Verification Driver**: [`tests/YarpgenDifferentialTests/mega_stress_driver.c`](mega_stress_driver.c)
- **Pass Profiling**: Measures middle-end and backend compiler pass execution times, percentage share, and bottleneck passes via `ezc --time-passes`.
- **Differential Verification**: Executes all 148 kernels in sequence, hashes all state variables, and compares the resulting 64-bit checksum against GCC/Clang to prove that the massive translation unit was compiled with zero semantic defects.

#### How to Run:
```bash
# 1. Run Single Big File Stress Benchmark & Pass Profiler across all optimization levels:
cmake --build build --target stress-ezc-passes

# 2. Run Full Stress Compilation with Reference Compiler Verification:
cmake --build build --target stress-ezc

# 3. Run in CTest:
ctest --test-dir build -R "EzCompilerYarpgen_StressCompilation" --output-on-failure
ctest --test-dir build -R "EzCompilerYarpgen_StressDifferential" --output-on-failure

# 4. Standalone Python CLI:
python tests/YarpgenDifferentialTests/yarpgen_suite.py stress --opt-level O0 --compile-only
python tests/YarpgenDifferentialTests/yarpgen_suite.py stress --opt-level all -v
python tests/YarpgenDifferentialTests/yarpgen_suite.py stress --opt-level O0 --ref-compiler gcc
```

---

## 🛠️ YARPGen Vendor Integration

YARPGen is imported into EzPacker using the standard vendor API in [`CMake/Vendor.cmake`](../CMake/Vendor.cmake):
```cmake
EzCMK_AddVendor(https://github.com/intel/yarpgen
        1adb290f453505838f3aa33dc571f292b0c1810f
        yarpgen
)
```

The target `yarpgen` is automatically imported and linked into the CMake dependency graph without requiring custom build overlays.

---

## ⚙️ Unified CLI Reference (`yarpgen_suite.py`)

All testing workflows are consolidated into a single unified runner:

```bash
# Stress monolithic benchmark and pass profiler
python tests/YarpgenDifferentialTests/yarpgen_suite.py stress [--file <path>] [--opt-level O0|O1|O2|Os|all] [--compile-only] [-v]

# Bulk corpus compilation and differential verifier
python tests/YarpgenDifferentialTests/yarpgen_suite.py bulk [--corpus-dir <path>] [--opt-levels O0,O2] [--compile-only]

# Continuous differential fuzzing
python tests/YarpgenDifferentialTests/yarpgen_suite.py fuzz [--iterations N] [--seed S] [--work-dir <path>]

# Curated regression seeds verification
python tests/YarpgenDifferentialTests/yarpgen_suite.py seeds [--seeds-file <path>]
```

---

## 🧪 Freestanding YARP End-to-End Suite (`EzCompilerEndToEndTests/yarp_corpus/`)

In addition to differential fuzzing against GCC/Clang, EzPacker features a dedicated **freestanding end-to-end test suite** in `tests/EzCompilerEndToEndTests/yarp_corpus/` designed to verify standalone C11 compilation without hosted C standard library dependencies:

- **Zero stdlib headers**: Test units compile purely against ISO C11 freestanding headers (`<stdint.h>`, `<stdbool.h>`, `<stddef.h>`, `<limits.h>`).
- **Deterministic 64-bit verification**: Test drivers use `freestanding_driver.h` providing a 64-bit hashing primitive (`hash_step`) and return `0` on successful verification or non-zero exit codes on failure, eliminating `<stdio.h>` and `printf`.
- **Cross-Platform ABI verification**:
  - Compiles to **Windows x86_64 COFF** (`.obj`) and verifies against Microsoft Win64 ABI (`-DABI_WIN64`).
  - Compiles to **Linux x86_64 ELF** (`.o`) and verifies against System V AMD64 ABI (`-DABI_SYSV`).
  - Validates ELF relocatables and section headers using `elf_validator`.
- **Curated test areas**:
  - `yarp_01_arithmetic_logic`: 64-bit/32-bit arithmetic, bitwise masks, sign-extension, and multi-term algebra.
  - `yarp_02_branch_loops`: Nested loops, conditional branching, and early loop breaks.
  - `yarp_03_memory_arrays`: Array indexing, pointer arithmetic, buffer reversal, and memory accumulators.
  - `yarp_04_type_conversions`: Signed/unsigned widening, narrowing truncation, and bitcasts.
  - `yarp_05_structs_globals`: Struct layouts, member offset arithmetic, and file-scope global variables.
  - `yarp_06_nested_calls`: Cross-function parameter passing, deep call chains, and accumulator folding.

---

## 🏷️ CTest Labels Summary

All tests are integrated into CTest and can be filtered by labels:
- `ctest -L Bulk`: Runs all bulk file compilation tests.
- `ctest -L Stress`: Runs all single big file stress tests.
- `ctest -L Compilation`: Runs pure compiler stability and throughput tests.
- `ctest -L Differential`: Runs execution and checksum verification against GCC/Clang.
- `ctest -L Regression`: Runs curated deterministic seed regression tests.
- `ctest -L Yarpgen`: Runs the entire YARPGen test fleet.
- `ctest -L Freestanding`: Runs the freestanding YARP end-to-end tests without stdlib.

---

## 🛡️ Curated Regression Seeds

The test suite tracks deterministic YARPGen seeds in `tests/YarpgenDifferentialTests/regression_seeds.txt`. Each seed targets a specific language construct or lowering edge case discovered during fuzzing:

| Seed | Target Construct / Fix Area |
| :--- | :--- |
| `401871035` | Narrowing integer truncation and implicit conversions (`TRUNC`) |
| `545245296` | Unary operators, sign extensions, and scalar assignment |
| `326683734` | Arithmetic expressions, min/max conditionals, and boolean tests |
| `1045802857` | Multi-operand comparisons, type width legalization |
| `1104` | Unsigned cast zero-extension, logical AND/OR comparison operand width matching |
| `121` | Bitwise operations, nested ternaries, and signed comparisons |

---

## 🐛 Compiler Defects Discovered & Resolved via Differential Testing

1. **`CMP_NE` Operand Bit-Width Mismatch in Logical AND/OR (`ND_LOGAND`, `ND_LOGOR`)**:
   - *Problem*: In `CFrontendAdapter.cpp`, `FlexInt(0)` was constructed without an explicit bit-width, defaulting to 32 bits. When lowering boolean expressions (`i1`) or sub-byte comparisons, `buildConstant` promoted constant operand types to `i32`, producing a `CMP_NE` with operand type mismatch (`i1` vs `i32`) which triggered `MirVerifierPass` aborts.
   - *Fix*: Created constants using explicit bit-widths derived from operand MIR types: `FlexInt(0, static_cast<uint16_t>(rhsType->getTotalSizeInBits()))`.
2. **Unsigned Integer Literal Clamping in `ND_NUM`**:
   - *Problem*: Integer literals with unsigned types (e.g. `(unsigned short)36391`) were treated as signed `int64_t` during AST node conversion, causing bit 15 to sign-extend and clamp to `-29145` via two's complement.
   - *Fix*: Preserved `ty->is_unsigned` and zero-extended literals when constructing `FlexInt`.
3. **Implicit Zero-Extension in `ND_CAST`**:
   - *Problem*: Casting an unsigned integer to a wider type signed-extended the raw value because `imm->getValue().getI64()` was used unconditionally.
   - *Fix*: Applied unsigned masking (`& ((1ULL << srcBits) - 1)`) when `node->lhs->ty->is_unsigned` is true.
4. **64-bit Integer Literal Truncation on Windows**:
   - *Problem*: Upstream `chibicc`'s `tokenize.c` used `strtoul` which returns a 32-bit `unsigned long` under the Windows LLP64 data model.
   - *Fix*: Upgraded to `strtoull` in the overlay, preserving full 64-bit precision for large literal seeds.
