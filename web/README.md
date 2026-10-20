# EzPacker Web Explorer & Compiler Developer Workbench

A modern, high-density compiler engineering workbench and interactive explorer for the **EzPacker** compiler suite (`ezc`).

## Overview

EzPacker Web Explorer is designed specifically for **compiler developers** to inspect, debug, and optimize each phase of the compilation pipeline. Powered by native JSON pipeline telemetry from `ezc`, the workbench provides deterministic pass-by-pass snapshots, side-by-side delta diffing, control flow graph (CFG) topology, register liveness analysis, stack frame schematics, object binary analysis (COFF & ELF64), and target machine code generation (x86-64 and RISC-V 64).

---

## Key Features for Compiler Developers

### 1. Fine-Grained Pass-by-Pass Inspection
- **Deterministic Native Telemetry**: Rather than scraping terminal logs or invoking `ezc` multiple times, the server runs `ezc` once with `--dump-pipeline-json <file> -o <obj>`. Every executed pass is recorded natively inside `CompilationPipeline`.
- **Pass Pipeline Navigator (`PassPipelineNav`)**:
  - Displays every pass in exact sequential execution order (`#01`, `#02`, ..., `#39`).
  - Stage classification badges:
    - `Verification` (slate): `MirVerifierPass`, etc.
    - `MiddleEnd` (sky/blue): `NonSsaToSsaPass`, `MirPeepholePass`, etc.
    - `Legalization` (amber): `MirFunctionSignatureLegalizerPass`, `MirLegalizerPass`, etc.
    - `TargetLowering` (purple): `MirInstructionSelectorPass`, `MirRegisterAllocatorPass`, `MirFrameLowererPass`, etc.
  - Target function attribution for function-level passes (`@hash_step32`, `@bit_blend32`, `@main`).
  - High-precision execution duration per pass (`0.045 ms`) and pass sequence waterfall.
  - **$\Delta$ Mutation Tracking**: Clearly flags passes that actively mutated the MIR (`Δ MUTATED`) vs passes that preserved the IR.
  - **Developer Filters**:
    - `All`: View the complete pipeline sequence.
    - `Mutations (Δ)`: Instantly filter to only the passes that changed IR instructions or structure.
    - Stage filters: `Verify`, `Middle`, `Legalize`, `Lowering`.
    - Live search filter: Quickly locate passes by name or target function.

### 2. Multi-Mode Pass Inspector (`PassInspector`)
When inspecting any pass in the pipeline, switch seamlessly between:
- **Snapshot View**: Full Monaco Editor displaying the exact MIR state immediately after the selected pass ran, with custom EzPacker MIR syntax highlighting.
- **Diff vs Prev ($\Delta$)**: Side-by-side or inline Monaco Diff Viewer comparing the immediate predecessor pass (or initial MIR) against the current pass. Added, removed, or rewritten instructions are highlighted.
- **Diff vs Initial**: Compare the current pass against the original input frontend MIR to evaluate end-to-end optimization effects.
- **CFG at Pass**: Inspect basic block structures, instruction counts, and predecessor/successor branches.
- **Liveness Analysis**: Verify register definition sets (`Def`), uses (`Use`), `LiveIn`, and `LiveOut` intervals.
- **Step Navigation**: Single-click `<` and `>` arrow keys to walk forward and backward through compiler passes.

### 3. Coarse Pipeline Stages & Global Diffing
- Direct 1-click toggling between coarse architectural stages:
  1. **MIR**: Generic SSA Intermediate Representation.
  2. **Legalized**: Target type and opcode legalized MIR.
  3. **Lowered**: Target-lowered machine MIR post register allocation and frame layout.
  4. **Assembly (`-S`)**: Emitted assembly code for x86-64 or RISC-V 64.
  5. **Object**: Compiled binary relocatable object file.
- **Global IR Diff Viewer (`DiffViewer`)**: Compare any two coarse stages or any two arbitrary passes across the pipeline.

### 4. Deep Analysis Suite
- **Control Flow Graph (`CfgViewer`)**: Interactive visualization of basic block control flow with entry points, edge counts, and branch targets.
- **Register Liveness (`LivenessViewer`)**: Tabular register lifetime intervals with per-register filtering and highlight tracking.
- **Stack Frame Layout (`StackViewer`)**: Architecture-aware stack frame schematic displaying total frame size, callee-saved registers, stack slots, and alignment boundaries.
- **Pass Timing Waterfall (`PassViewer`)**: Visual waterfall chart highlighting the slowest optimization and lowering passes with 1-click jump to the Pass Inspector.

### 5. Relocatable Object File (.o/.obj) Inspection
- Native binary inspection of emitted PE/COFF and ELF64 object files:
  - Section headers (`.text`, `.rdata`, `.data`, `.bss`) with offsets, sizes, and flags.
  - Symbol table with linkage (`global`, `local`, `weak`), types, and values.
  - Relocation records with target-specific relocation types (`IMAGE_REL_AMD64_REL32`, `R_X86_64_PLT32`, `R_RISCV_BRANCH`, etc.).
  - Formatted byte and ASCII hex preview.

### 6. Interactive Symbol & Doxygen Documentation
- Automatic background correlation between compiler symbols, runtime functions (e.g. `@__ez_throw`, `@__ez_try_enter`), and Doxygen docstrings.
- Searchable symbol catalog with function signatures, parameter descriptions, and return types.

---

## Architectural Design

```
+-------------------------------------------------------------+
|                      web/client (React 18 + Vite)           |
|                                                             |
|  [Header: Target Triple | Opt Level | Features | Examples]  |
|  +---------------------------+---------------------------+  |
|  |     Left: Monaco Editor   |  Right: Developer Workbench|  |
|  |     (MIR / C11 Sources)   |  - Passes (Nav + Inspector)|  |
|  |                           |  - Stages (Coarse + Diff)  |  |
|  |                           |  - Analysis (CFG/Liveness) |  |
|  |                           |  - Symbols & Doxygen Docs  |  |
|  +---------------------------+---------------------------+  |
|  [Bottom: Real-Time Compiler Diagnostics Drawer]            |
+-------------------------------------------------------------+
                              |
                     POST /api/compile
                              |
                              v
+-------------------------------------------------------------+
|                   web/server (Express + TypeScript)         |
|                                                             |
|  CompilerRunner (Single Unified Invocations):               |
|    ezc -o output.obj --dump-pipeline-json pipeline.json ... |
|                                                             |
|  +----------------------------+---------------------------+ |
|  | Read pipeline.json         | Read output.obj           | |
|  | (Passes, Timings, Dumps,   | (ObjectParser: COFF/ELF64 | |
|  |  CFG, Liveness, Frame)     |  sections, symbols, reloc)| |
|  +----------------------------+---------------------------+ |
|                                                             |
|  DoxygenRunner: Symbol correlation & docstring matching     |
+-------------------------------------------------------------+
                              |
                              v
+-------------------------------------------------------------+
|               EzCompiler Native Engine (ezc.exe)            |
|                                                             |
|  PipelineTelemetry: Records snapshots before/after passes,  |
|  tracks IR mutations, captures CFG topologies, stack frames,|
|  and diagnostics even on partial failure or syntax errors.  |
+-------------------------------------------------------------+
```

---

## Getting Started

### Prerequisites
- Node.js >= 18
- Built `ezc.exe` located at `build/bin/ezc.exe` (or specified via `EZC_BIN` environment variable).

### Running Locally

```bash
# 1. Start backend server (Port 3001)
cd web/server
npm install
npm run build
npm run dev

# 2. In another terminal, start frontend client (Port 5173)
cd web/client
npm install
npm run dev
```

Open [http://localhost:5173](http://localhost:5173) in your browser.
