# Compiler Stage Performance Benchmarks & Evaluation

[EzPacker Documentation Index](index.md) > **Compiler Stage Performance Benchmarks**

This document tracks the end-to-end compilation throughput, memory footprint (peak RSS), per-stage / per-pass timing breakdown, empirical scaling behavior, and comparisons against local GCC and public compiler baselines for `ezc` (Release build, `cmake-build-release/bin/ezc`).

---

## 1. Benchmark Methodology

- **Build configuration**: `CMAKE_BUILD_TYPE=Release` (`gcc 13.3.0`, x86-64 Linux host, 2 vCPUs).
- **Sampling protocol**: 1–2 warmup invocations followed by 2–5 timed iterations; reporting **median** and **p95** wall-clock time (ms), throughput (**KLOC/s**), peak **RSS** (KB via `getrusage`), and emitted `.text` section size (`size`).
- **Instrumentation isolation**: Clean `-o` invocations are measured separately from `--time-passes` runs (`< 1%` instrumentation overhead confirmed on `examples/mega_stress.c`).
- **Targets**: `x86_64-linux-gnu` (C + MIR) and `riscv64-unknown-linux-gnu` (MIR).

---

## 2. Baseline: `ezc` vs Local `gcc 13.3.0` (Pre-Optimization)

| Input (LOC) | Compiler | Opt | Target | Median (ms) | p95 (ms) | KLOC/s | Peak RSS (KB) | `.text` (B) |
|---|---|---:|---|---:|---:|---:|---:|---:|
| `examples/mega_stress.c` (3,623) | `ezc` (`-o`) | `-O0` | `x86_64-linux-gnu` | 1,152.23 | 1,161.49 | 3.14 | 319,836 | 31,827 |
| `examples/mega_stress.c` (3,623) | `ezc` (`-o`) | `-O2` | `x86_64-linux-gnu` | 1,321.08 | 1,383.78 | 2.74 | 331,744 | 27,963 |
| `examples/mega_stress.c` (3,623) | `gcc` (`-ffreestanding -c`) | `-O0` | `x86_64-linux-gnu` | 115.80 | 117.44 | 31.29 | 28,220 | 19,698 |
| `examples/mega_stress.c` (3,623) | `gcc` (`-ffreestanding -c`) | `-O1` | `x86_64-linux-gnu` | 218.79 | 220.06 | 16.56 | 30,648 | 8,606 |
| `examples/mega_stress.c` (3,623) | `gcc` (`-ffreestanding -c`) | `-O2` | `x86_64-linux-gnu` | 307.75 | 313.05 | 11.77 | 33,380 | 9,863 |
| `examples/09_freestanding_kernel.c` (82) | `ezc` | `-O0` | `x86_64-linux-gnu` | 8.61 | 15.67 | 9.53 | 10,324 | 1,046 |
| `examples/09_freestanding_kernel.c` (82) | `ezc` | `-O2` | `x86_64-linux-gnu` | 9.14 | 9.21 | 8.97 | 10,484 | 931 |
| `examples/09_freestanding_kernel.c` (82) | `gcc` | `-O0` | `x86_64-linux-gnu` | 21.42 | 21.47 | 3.83 | 20,868 | 696 |
| `examples/09_freestanding_kernel.c` (82) | `gcc` | `-O2` | `x86_64-linux-gnu` | 30.51 | 30.63 | 2.69 | 22,864 | 483 |
| `examples/branch_control_flow.mir` (66) | `ezc` | `-O2` | `x86_64` / `riscv64` | 9.67 / 9.55 | 9.78 / 10.07 | 6.83 / 6.91 | 9,564 / 9,564 | 260 / 172 |
| `examples/exception_handling.mir` (68) | `ezc` | `-O2` | `x86_64` / `riscv64` | 10.27 / 18.04 | 10.62 / 19.23 | 6.62 / 3.77 | 9,516 / 9,516 | 259 / 244 |
| `examples/vector_simd_avx.mir` (43) | `ezc` | `-O2` | `riscv64-unknown-linux-gnu` | 10.71 | 10.92 | 4.02 | 9,472 | 180 |

---

## 3. Baseline Stage-Gate & Per-Pass Breakdown (`examples/mega_stress.c`, 148 Functions)

### Stage-Gate Wall Time & Peak RSS
| Stage Gate | `-O0` Median (ms) | `-O0` RSS (KB) | `-O2` Median (ms) | `-O2` RSS (KB) |
|---|---:|---:|---:|---:|
| `--emit-mir` (Frontend + Mid-End) | 127.29 | 43,076 | 185.89 | 52,996 |
| `--emit-legalized-mir` (+ Legalization) | 130.59 | 43,260 | 190.66 | 53,036 |
| `--emit-lowered-mir` (+ Target Lowering / RA) | 1,052.98 | 319,812 | 1,332.58 | 331,244 |
| `-S` (+ Assembly Text) | 1,050.87 | 319,088 | 1,299.83 | 330,940 |
| `-o` (+ `EmissionEngine` ELF64) | 1,068.69 | 319,884 | 1,307.91 | 331,748 |

### `--time-passes` Breakdown
| Pass | `-O0` Total (ms) | `-O0` % | `-O2` Total (ms) | `-O2` % | Calls (`-O0` / `-O2`) |
|---|---:|---:|---:|---:|---:|
| `MirRegisterAllocatorPass` | **851.62** | **85.8%** | **1,036.11** | **83.0%** | 148 / 148 |
| `CodeFlowAnalysisPass` | 95.43 | 9.6% | 96.02 | 7.7% | 148 / 148 |
| `LivenessAnalysisPass` | 6.60 | 0.7% | **74.04** | **5.9%** | 148 / 296 |
| `NonSsaToSsaPass` | 16.27 | 1.6% | 16.14 | 1.3% | 148 / 148 |
| `MirInstructionSelectorPass` | 11.35 | 1.1% | 11.50 | 0.9% | 148 / 148 |
| `MirFrameLowererPass` | 4.30 | 0.4% | 4.13 | 0.3% | 148 / 148 |
| `MirAbiLowererPass` | 3.00 | 0.3% | 3.23 | 0.3% | 148 / 148 |
| `MirTargetPeepholePass` | — | — | 2.59 | 0.2% | 0 / 148 |
| `MirLegalizerPass` | 2.27 | 0.2% | 2.32 | 0.2% | 148 / 148 |
| `MirVerifierPass` | 1.07 | 0.1% | 1.14 | 0.1% | 148 / 148 |
| `MirFunctionSignatureLegalizerPass` | 0.66 | 0.1% | 0.79 | 0.1% | 148 / 148 |
| `MirPeepholePass` | — | — | 0.76 | 0.1% | 0 / 148 |

---

## 4. Baseline Empirical Scaling & Public Compiler Comparison

### Synthetic Scaling Exponents (`-O2`)
- **Basic block count** (`synth_bb_N.c`, $N \in \{25, 50, 100, 150\}$, 0 spill iterations):
  - Wall time scales as **$O(N^{2.91})$** (`17,283.53 ms` at $N=150$ at `-O2` vs `150.33 ms` at `-O0`).
  - `MirRegisterAllocatorPass` scales as **$O(N^{2.99})$**; `LivenessAnalysisPass` scales as **$O(N^{1.65})$**.
- **Virtual register / op count** (`synth_vreg_N.c`, $N \in \{25..200\}$, 2 spill iterations):
  - Wall time scales as **$O(N^{2.05})$** (`13,908.45 ms` and **704 MB RSS** at $N=200$ at `-O2` vs `71.08 ms` and `23 MB` at `-O0`).

### Comparison vs Public Compilers
- **GCC 13.3.0 (`-ftime-report` on `mega_stress.c`)**: `integrated RA + LRA` consumes ~15–31% of total compile time (`33.4 MB` peak RSS at `-O2`, `11.77 KLOC/s`).
- **Clang / LLVM (`-ftime-report` at `-O2`)**: Typically allocates 30–50% to Frontend, 15–30% to IR Mid-End, and 20–40% to Backend (ISel 5–15%, RegAlloc 5–12% of total).
- **Lightweight compilers**: **TCC** reaches ~100–130+ KLOC/s (single-pass); **chibicc** standalone reaches ~7–15 KLOC/s. `ezc`'s frontend+midend (`--emit-mir`) already runs at ~19.5–28.5 KLOC/s, but `MirRegisterAllocatorPass` (83–86% of pass time) pulls end-to-end throughput down to **2.74 KLOC/s**.

---

## 5. Identified Baseline Bottlenecks

1. **`MirRegisterAllocatorPass` ($O(\text{BB}^3)$ / $O(\text{VReg}^2)$)**: `std::pmr::unordered_map<MirRegisterRef, std::pmr::set<MirRegisterRef>>` interference graph + `coalesce()` restarting from candidate 0 on every merge (`break;`) + per-candidate `unordered_set` allocations in Briggs checks.
2. **Monotonic Arena Retention (331–704 MB RSS)**: `RegisterAllocatorCtx` and `LivenessAnalysisPass::computeGlobalLiveness()` allocate temporary structures into `MirBuilderContext::getGlobalAllocator()` (`monotonic_buffer_resource`), retaining dead graph/liveness allocations across spill iterations.
3. **Module-Wide Liveness/CFG Invalidation**: `MirRegisterAllocatorPass::run()` calls `passManager->invalidateAnalysis()` on every spill iteration, forcing `CodeFlowAnalysisPass` and `LivenessAnalysisPass` to recompute across all 148 functions.
4. **Eager Spill-Cost Scan & `-O0` Copy Churn**: `simplify()` eagerly computes spill costs even when zero spills occur, and `-O0` skips lightweight `MOV` affinity hints.
