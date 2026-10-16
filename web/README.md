# EzPacker Web Explorer

A web-based interactive compiler explorer and analysis environment for the **EzPacker** compiler (`ezc`).

## Overview

EzPacker Web Explorer provides an intuitive UI for inspecting intermediate representations (MIR), machine lowering stages, control flow graph topology, register liveness sets, stack frame layouts, and final x86-64 assembly generation.

## Features

- **Monaco Code Editor**:
  - Full syntax highlighting for EzPacker Generic MIR (`.mir`) keywords, opcodes, types, registers, and branch labels.
  - Real-time compiler diagnostics with line-and-column navigation.
  - Debounced auto-compilation and `Ctrl+Enter` trigger.
- **Seven Dedicated Workspace Tabs**:
  1. **Assembly (`-S`)**: Clean x86-64 machine assembly code with one-click clipboard copy.
  2. **IR Stages**: Direct toggle between Generic MIR (`--emit-mir`), Legalized MIR (`--emit-legalized-mir`), and Lowered Machine MIR (`--emit-lowered-mir`).
  3. **IR Diff**: Side-by-side Monaco Diff Viewer comparing any two compiler stages.
  4. **Passes & Timing**: Millisecond timing waterfall for each compiler optimization and lowering pass.
  5. **Control Flow Graph (CFG)**: Basic block visualizer featuring instruction listings, predecessor/successor badges, and click-to-jump block navigation.
  6. **Register Liveness**: Color-coded `Def`, `Use`, `LiveIn`, and `LiveOut` intervals with interactive register filtering.
  7. **Stack Frame Analysis**: Stack frame metrics (`Callee-saved`, `Total frame size`, `Locals/spills`, `16-byte alignment`), visual memory hierarchy schematic, and prologue/epilogue previews.
- **Object File (.o/.obj) Inspection**:
  - Direct binary parsing of generated PE/COFF and ELF64 object files.
  - Section tables (`.text`, `.rdata`, `.data`, `.bss`), virtual addresses, raw data sizes, and flags.
  - Full symbol table inspection with linkage (`global`/`local`/`weak`), section bindings, and values.
  - Relocation table decoding with fixup types (e.g. `IMAGE_REL_AMD64_REL32`, `R_X86_64_PLT32`).
  - Formatted byte and ASCII hex dump preview.
- **Automated Doxygen Documentation Integration**:
  - Background indexing of C ABI runtime headers (`EzExceptionRuntime.h`), compiler core headers (`EzMir`, `EzCompiler`, `EzCore`), and project markdown documents.
  - Structured extraction of function signatures, parameter descriptions, return types, and docstrings.
  - Interactive symbol investigation and on-demand documentation generation via `/api/doxygen/generate`.
  - Resilient error handling that surfaces non-crashing diagnostics when Doxygen is missing or misconfigured.
- **Compiler Configuration & Targets**:
  - Target triple selection (`x86_64-pc-windows-msvc`, `x86_64-linux-gnu`, `x86_64-apple-darwin`).
  - Optimization levels: `-O0`, `-O1`, `-O2`, `-Os`.
  - Feature flags: `+avx`, `+avx2`, `+sse2`, etc.
  - Code generation flags: Position Independent Code (`-fPIC`), RTTI toggle.
- **12 Bundled Examples**:
  - Arithmetic, branch control flow, loops, recursive factorial, SIMD vectorization, structured exception handling (`exception_handling.mir`), and memory pointer examples from `examples/`.

## Architecture

- **`web/server`**: Express + TypeScript bridge:
  - `CompilerRunner`: Executes `ezc.exe` with telemetry flags and produces assembly, intermediates, and object files.
  - `ObjectParser`: Binary parser extracting headers, sections, symbols, and relocations from COFF AMD64 and ELF64 buffers.
  - `DoxygenRunner`: Automated Doxygen XML generation, parsing, memory caching, and symbol docstring correlation.
- **`web/client`**: React 18 + Vite + Tailwind CSS + Zustand + Monaco Editor.

## Getting Started

### Prerequisites

- Node.js >= 18
- Built `ezc.exe` located at `cmake-build-debug/bin/ezc.exe` (or in `PATH`).

### Running Locally

```bash
# Terminal 1: Start backend server (Port 3001)
cd web/server
npm install
npm run dev

# Terminal 2: Start frontend client (Port 5173)
cd web/client
npm install
npm run dev
```

Open [http://localhost:5173](http://localhost:5173) in your browser.
