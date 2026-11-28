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

## 5. Post-Optimization Results (Milestones 1–4)

Following the 4 optimization milestones (dense-indexed interference graph & linear-scan coalescing, split persistent/iteration-scoped PMR scratch arenas, per-function liveness recomputation, and lazy spill-cost evaluation):

### Before vs After: Real Corpus (`x86_64-linux-gnu` & `riscv64-unknown-linux-gnu`)
| Input (LOC) | Target | Opt | Before Med (ms) | After Med (ms) | Speedup | After KLOC/s | Before RSS (KB) | After RSS (KB) |
|---|---|---|---:|---:|---:|---:|---:|---:|
| `examples/mega_stress.c` (3,623) | `x86_64` | `-O0` | 1,065.54 | **873.28** | **1.22x** | 4.15 | 319,836 | **213,928** (-33.1%) |
| `examples/mega_stress.c` (3,623) | `x86_64` | `-O2` | 1,315.87 | **919.09** | **1.43x** | 3.94 | 331,744 | **221,012** (-33.4%) |
| `examples/09_freestanding_kernel.c` (82) | `x86_64` | `-O2` | 9.14 | **8.56** | 1.07x | 9.58 | 10,484 | 9,716 |
| `examples/c_kernel_queue.c` (107) | `x86_64` | `-O2` | 11.03 | **10.04** | 1.10x | 10.65 | 11,012 | 9,988 |
| `examples/branch_control_flow.mir` (66) | `riscv64` | `-O2` | 9.55 | **9.28** | 1.03x | 7.12 | 9,564 | 8,712 |
| `examples/exception_handling.mir` (68) | `riscv64` | `-O2` | 18.04 | **10.21** | **1.77x** | 6.66 | 9,516 | 9,064 |

On `examples/mega_stress.c -O2` (`--time-passes`), `MirRegisterAllocatorPass` total time dropped from `1,036.11 ms` to **`635.81 ms`** (**1.63x**), and max single-function RA time dropped from `73.28 ms` to **`6.36 ms`** (**11.5x**).

### Before vs After: Synthetic Scaling (`-O2`)
| Stress Workload | Before Wall (ms) | After Wall (ms) | Speedup | Before RSS (KB) | After RSS (KB) | Memory Reduction |
|---|---:|---:|---:|---:|---:|---:|
| `synth_vreg_200.c` (200 vars, 1 BB) | 13,908.45 | **36.15** | **384.7x** | 704,096 | **14,112** | **49.9x** |
| `synth_vreg_800.c` (800 vars, 1 BB) | *(infeasible)* | **188.12** | — | — | **35,060** | — |
| `synth_bb_150.c` (150 BBs) | 17,283.53 | **238.22** | **72.5x** | 160,464 | **109,404** | 1.47x |
| `synth_bb_600.c` (600 BBs) | *(>1,000s proj.)* | **4,045.33** | — | — | 1,510,540 | — |

- **Empirical `-O2` Scaling Exponents ($\alpha$)**:
  - Virtual register pressure (`25..200`): **$O(N^{2.05}) \to O(N^{0.72})$** (near-linear up to 800 vars at $\alpha = 0.87$).
  - Basic block count (`25..150`): **$O(N^{2.91}) \to O(N^{1.54})$**.

---

## 6. Remaining Optimization Roadmap

1. **Fast `-O0` Linear-Scan Register Allocator**: On `mega_stress.c -O0` (148 functions), Chaitin-Briggs graph coloring still accounts for `683.48 ms` (~4.6 ms/function). Adding a dedicated `-O0` linear-scan allocator would bring `-O0` throughput closer to `gcc -O0` (31.29 KLOC/s) and `chibicc` (7–15 KLOC/s).
2. **Sparse Liveness & Per-Function `MirPassManager` Scoping**: `synth_bb_600` still exhibits $O(\text{BB}^{1.66})$ in `LivenessAnalysisPass` and $O(\text{BB}^{2.08})$ in RA; `CodeFlowAnalysisPass` + `LivenessAnalysisPass` still consume `150.5 ms` combined on `mega_stress.c -O2`. Replacing dense `m_adjBits` matrix allocation in large-BB functions with sparse bitvectors/adjacency lists and caching `LivenessResult` directly as `DenseBitSet` would further cut both wall time and RSS.
3. **Frontend & `.text` Size**: `ezc -O2` `.text` (`27,766 B` on `mega_stress.c`) remains 2.81x larger than `gcc -O2` (`9,863 B`); middle-end dead-code elimination, constant folding, and GVN/CSE passes would close the code-size gap.
