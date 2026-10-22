# EzPacker Web Explorer & Compiler Developer Workbench

[EzPacker Documentation Index](index.md) > **Web Developer Workbench**

---

## 1. Overview & Purpose

The **EzPacker Web Explorer & Compiler Developer Workbench** is a high-density, full-stack compiler engineering environment bundled directly within the EzPacker repository (`web/`). 

Unlike conventional online compiler explorers that only present source code alongside the final emitted assembly, EzPacker's workbench is tailored specifically for **compiler backend developers**. Powered by native JSON telemetry emitted by `ezc`, the workbench exposes the compiler's entire internal state across all 39+ optimization, legalization, and lowering passes.

```
+-----------------------------------------------------------------------------------------+
|                               web/client (React 18 + Vite)                              |
|                                                                                         |
|  [Header: Target Triple | Opt Level (-O0..-O2) | Subsystem | Architecture Extensions]   |
|  +-------------------------------------+---------------------------------------------+  |
|  |       Left Pane: Monaco Editor      |          Right Pane: Developer Workbench    |  |
|  | - C11 & MIR Syntax Highlighting     | 1. Pass Navigator (#01..#39 with Δ tracking)|  |
|  | - Preset Test Cases & Kernels       | 2. Pass Inspector (Snapshot, Diff vs Prev)  |  |
|  | - Diagnostics Inline Markers        | 3. Coarse Stages (MIR, Legal, Lower, Asm)   |  |
|  |                                     | 4. CFG & Liveness Visualizer                |  |
|  |                                     | 5. Stack Frame Schematic                    |  |
|  |                                     | 6. Binary Dissector (COFF & ELF64)          |  |
|  |                                     | 7. Interactive Doxygen Symbol Bridge        |  |
|  +-------------------------------------+---------------------------------------------+  |
|  [Bottom Drawer: Real-Time Compiler Diagnostics & Emission Logs]                        |
+-----------------------------------------------------------------------------------------+
                                              |
                                      POST /api/compile
                                              |
                                              v
+-----------------------------------------------------------------------------------------+
|                             web/server (Express + TypeScript)                           |
|                                                                                         |
|  CompilerRunner (Single Unified Invocations):                                           |
|    ezc -o output.obj --dump-pipeline-json pipeline.json ...                             |
|                                                                                         |
|  +-----------------------------------------+-----------------------------------------+  |
|  | Read pipeline.json                      | Read output.obj                         |  |
|  | - Per-pass snapshots & durations        | - ObjectParser: COFF & ELF64 headers    |  |
|  | - Basic block CFG topologies            | - Section table & byte hex dumps        |  |
|  | - Liveness Def/Use/In/Out sets          | - Symbol table & relocation records     |  |
|  | - Stack frame slot metrics              |                                         |  |
|  +-----------------------------------------+-----------------------------------------+  |
|  | DoxygenRunner: Background XML symbol extraction & docstring correlation           |  |
+-----------------------------------------------------------------------------------------+
```

---

## 2. Architectural Design & Tech Stack

The workbench consists of two tightly coupled applications:

### 2.1 Web Client (`web/client/`)
- **Framework**: React 18, TypeScript, Vite.
- **Styling & UI**: TailwindCSS, Lucide React icons.
- **Editor**: Monaco Editor (`@monaco-editor/react`) with custom language definitions for EzPacker SSA MIR.
- **State Management**: Zustand store (`web/client/src/store/compilerStore.ts`) managing active source files, compiler options, telemetry data, active pass selection, and diff comparisons.

### 2.2 Server Bridge (`web/server/`)
- **Runtime**: Node.js, Express, TypeScript (`tsx`).
- **Compilation Orchestration**: `CompilerRunner` invokes the native `ezc` compiler binary once per compilation request with `--dump-pipeline-json <temp_path>` and relocatable object emission `-o <temp_obj>`.
- **Binary Dissection**: `ObjectParser` parses native PE/COFF (Windows) and ELF64 (Linux) relocatable object files in pure JavaScript/TypeScript without external dependencies.
- **Doxygen Bridge**: `DoxygenRunner` loads generated Doxygen XML documentation from `docs/doxygen/xml` or `docs/xml_web`, caching symbol signatures and docstrings.

---

## 3. Key Features for Compiler Developers

### 3.1 Pass Pipeline Navigator (`PassPipelineNav.tsx`)
The pass navigator renders the compiler's execution history in strict chronological sequence:
- **Sequential Indexing**: Every pass is numbered (`#01`, `#02`, ..., `#39`) matching execution order.
- **Stage Classification Badges**:
  - `Verification` (slate): `MirVerifierPass`.
  - `MiddleEnd` (sky/blue): `CodeFlowAnalysisPass`, `NonSsaToSsaPass`, `MirPeepholePass`, `LivenessAnalysisPass`.
  - `Legalization` (amber): `MirFunctionSignatureLegalizerPass`, `MirLegalizerPass`.
  - `TargetLowering` (purple): `MirAbiLowererPass`, `MirInstructionSelectorPass`, `MirRegisterAllocatorPass`, `MirFrameLowererPass`, `MirTargetPeepholePass`.
- **Target Function Attribution**: Indicates which function the pass was executed on (`@calculate`, `@dot3`, `@main`).
- **Precision Timings**: Displays high-resolution pass execution duration in milliseconds (e.g. `0.045 ms`).
- **Mutation Tracking ($\Delta$ MUTATED)**: Flags passes that modified the instruction stream or CFG topology. Unmutated passes are labeled `No change`, allowing developers to instantly spot where transformations occurred.
- **Developer Filters**:
  - `All`: View complete execution sequence.
  - `Mutations (Δ)`: View only the passes that changed instructions or basic blocks.
  - Stage filters: Quick-toggle `Verify`, `Middle`, `Legalize`, `Lowering`.
  - Search filter: Live search by pass name or function name.

### 3.2 Multi-Mode Pass Inspector (`PassInspector.tsx`)
Clicking any pass opens the inspector with multiple analytical modes:
1. **Snapshot View**: Full Monaco Editor displaying the complete MIR textual dump immediately after the selected pass executed.
2. **Diff vs Prev ($\Delta$)**: Interactive side-by-side or inline diff comparing the immediate predecessor pass against the selected pass. Highlighted additions, deletions, and instruction modifications pinpoint exactly what the pass transformed.
3. **Diff vs Initial**: Compares the current pass against the input frontend MIR to evaluate cumulative optimization effects.
4. **CFG at Pass**: Displays basic blocks, instruction counts, and predecessor/successor connections at this point in the pipeline.
5. **Liveness Sets**: Displays register definition (`Def`), usage (`Use`), `LiveIn`, and `LiveOut` sets per basic block.
6. **Step Controls**: Step backward (`<`) and forward (`>`) through the pipeline to inspect transformations sequentially.

### 3.3 Coarse Pipeline Stages & Global Diffing (`DiffViewer.tsx`)
Developers can toggle between high-level architectural milestones:
- **MIR**: Generic SSA Intermediate Representation post-middle-end.
- **Legalized**: Legalized MIR with expanded 64-bit/128-bit operations and libcall mappings.
- **Lowered**: Machine-instruction MIR post instruction selection, register allocation, and frame lowering.
- **Assembly (`-S`)**: Emitted target assembly code.
- **Object**: Binary relocatable object file overview.
- **Diff Mode**: Compare any two coarse stages side-by-side.

### 3.4 Deep Analysis Visualizers
- **Control Flow Graph (`CfgViewer.tsx`)**: Interactive topological map of basic blocks, branch conditions, entry blocks, and edge weights.
- **Register Liveness Matrix (`LivenessViewer.tsx`)**: Tabular visualization of virtual and physical register lifetimes across basic blocks.
- **Stack Frame Layout (`StackViewer.tsx`)**: Schematic rendering of stack layout:
  - Total frame size and alignment requirements.
  - Return address and frame pointer slots.
  - Callee-saved register save areas.
  - Local variable stack slots and spill slots.
  - Parameter passing and call shadow spaces.
- **Pass Timing Waterfall (`PassViewer.tsx`)**: Visual execution duration waterfall identifying compiler bottlenecks.

### 3.5 Binary Object Dissector
Inspects emitted relocatable object files without needing external tools like `objdump` or `dumpbin`:
- **Section Headers**: `.text`, `.data`, `.rdata`, `.bss`, section virtual/raw sizes, and memory protection flags.
- **Symbol Table**: Symbol names, storage classes, section indices, and binding linkages (`Global`, `Local`, `Weak`).
- **Relocation Table**: Detailed relocation entries with format-specific relocation types (`IMAGE_REL_AMD64_REL32`, `IMAGE_REL_AMD64_ADDR64`, `R_X86_64_PLT32`, `R_RISCV_BRANCH`, etc.).
- **Hex Preview**: Dual-column hexadecimal and ASCII byte preview of raw section data.

### 3.6 Interactive Doxygen Documentation Bridge
- When hovering over compiler symbols, IR instruction mnemonics, or runtime helpers (`@__ez_throw`, `@__ez_try_enter`), the workbench queries the cached Doxygen XML database.
- Displays full docstrings, parameter definitions, and return types inline within the editor.

---

## 4. Native Pipeline Telemetry Protocol

The workbench communicates with `ezc` via the native telemetry flag `--dump-pipeline-json`:

```bash
ezc input.c -o output.obj --dump-pipeline-json pipeline.json
```

### JSON Schema Overview
```json
{
  "version": "1.0",
  "targetTriple": "x86_64-pc-windows-msvc",
  "totalDurationMs": 1.45,
  "passes": [
    {
      "index": 1,
      "name": "MirVerifierPass",
      "stage": "Verification",
      "function": "@main",
      "type": "FunctionPass",
      "durationMs": 0.035,
      "modifiedMir": false,
      "instructionCount": 14,
      "blockCount": 2,
      "mirDump": "fn @main() -> i32 {\nentry:\n  ..."
    },
    {
      "index": 4,
      "name": "MirPeepholePass",
      "stage": "MiddleEnd",
      "function": "@main",
      "type": "FunctionPass",
      "durationMs": 0.082,
      "modifiedMir": true,
      "instructionCount": 11,
      "blockCount": 2,
      "mirDump": "..."
    }
  ],
  "topology": {
    "blocks": [
      {
        "id": 0,
        "label": "entry",
        "isEntry": true,
        "predecessors": [],
        "successors": ["exit"],
        "instructionCount": 8
      }
    ]
  },
  "liveness": [
    {
      "block": "entry",
      "defs": ["%0", "%1"],
      "uses": ["%a", "%b"],
      "liveIn": ["%a", "%b"],
      "liveOut": ["%1"]
    }
  ],
  "frameLayout": {
    "function": "@main",
    "totalFrameSize": 32,
    "calleeSavedRegs": ["rbp", "rbx"],
    "stackSlots": [
      { "offset": -8, "size": 8, "name": "saved_rbp" }
    ]
  },
  "diagnostics": []
}
```

---

## 5. Getting Started & Running Locally

### 5.1 Prerequisites
- **Node.js**: Version 18.0 or newer.
- **npm**: Version 9.0 or newer.
- **Compiled EzPacker**: Build `ezc` via CMake in `build/` (or specify path via environment variable).

### 5.2 Installation & Build
From the repository root:

```bash
# 1. Install dependencies for server and client
cd web/server && npm install
cd ../client && npm install
cd ../..

# 2. Build the production React client
npm run build --prefix web/client

# 3. Start the Express backend server
npm start --prefix web/server
```
The workbench will start and serve the application at **`http://localhost:3001`**.

### 5.3 Configuration via Environment Variables
The server automatically discovers `ezc` in standard build directories (`build/bin/Release/ezc.exe`, `build/bin/ezc`, etc.). You can customize paths using environment variables:

| Variable | Description | Default |
| :--- | :--- | :--- |
| `EZC_PATH` | Explicit path to the `ezc` executable | Auto-detected from `build/bin/` |
| `PORT` | HTTP port for the web server | `3001` |
| `DOXYGEN_XML_DIR` | Path to generated Doxygen XML documentation | `docs/doxygen/xml` |

### 5.4 Running Verification Tests
To verify that the server bridge correctly compiles test cases, parses telemetry JSON, and extracts object file headers:
```bash
npm test --prefix web/server
```
