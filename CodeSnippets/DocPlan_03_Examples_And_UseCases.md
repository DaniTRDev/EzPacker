# Plan Part 3: Working Examples and Real-World Use Cases Guide

## 1. Objective
Author a comprehensive **Examples & Use Cases Guide** (`docs/pages/examples_use_cases.md`), featuring in-depth architectural use cases and fully working, copy-paste-ready, compilable C++ and MIR examples covering programmatic IR construction, custom optimization passes, end-to-end pipeline execution, and binary object emission.

---

## 2. Deliverables & File Locations

| File | Purpose |
|:---|:---|
| [`docs/pages/examples_use_cases.md`](file:///E:/Repos/EzPacker/docs/pages/examples_use_cases.md) | Dedicated practical guide integrated into Doxygen via `@page examples_use_cases Examples & Use Cases`. |

---

## 3. Detailed Technical Content Outline

### A. Architectural Use Cases
1. **Ahead-of-Time (AOT) Language Backend**:
   - Compiling frontend ASTs down to EzPacker MIR.
   - Leveraging SSA transformation, architecture-independent optimization, and table-driven legalization.
   - Producing native COFF (`.obj`) or ELF64 (`.o`) binaries ready for the system linker (`link.exe`, `ld.lld`, or `gcc`).
2. **In-Memory & JIT Compilation Engine**:
   - Embedding `EzCompilerLib` within a runtime (database query execution, shader compiler, script engine).
   - Dynamically constructing MIR in memory and linearizing machine code into executable memory buffers without touching disk.
3. **Architecture Prototyping & Custom Accelerators**:
   - Designing new instruction sets, vector extensions, or custom DSP co-processors purely via declarative EzDsl files (`.tdesc`, `.idf`, `.isf`).
   - Automatically generating code selector stubs and instruction tables in minutes.
4. **Binary Instrumentation, Hardening & Transformation**:
   - Using `EzCodeEmitter` to assemble customized binary sections, control flow guards, and binary instrumentation trampolines with relocations.

---

### B. Detailed Working Code Examples

#### Example 1: Programmatic IR Construction (Loop with SSA Phi Nodes)
- **Scenario**: Generating a function `int64_t sum_array(int64_t *ptr, int64_t count)` entirely in C++.
- **Components Used**: `MirBuilderContext`, `MirModule`, `MirFunctionBuilder`, `MirBlockBuilder`, `MirInstructionBuilder`, `MirOperandBuilder`, `MirTypeTable`, `MirPrinter`.
- **Implementation Highlights**:
  - Allocating blocks: `entry`, `loop_header`, `loop_body`, `loop_exit`.
  - Creating SSA virtual registers.
  - Inserting `PHI` instructions in `loop_header` for index and accumulator.
  - Loading from pointer using base-displacement `MirMemory`.
  - Emitting `ADD`, `ICMP_SLT`, and conditional branch `BR_COND`.
  - Printing formatted textual MIR to `std::cout`.

#### Example 2: Authoring a Custom SSA Optimization Pass
- **Scenario**: Implementing a custom `PeepholeOptimizationPass` that folds redundant operations (e.g. `ADD x, 0 -> x`, `MUL x, 1 -> x`, `XOR x, x -> 0`).
- **Components Used**: `MirFunctionPass`, `MirInstructionIterator`, use-def inspection, `replaceUsesWith()`, instruction removal via `IntrusiveLinkedList`.
- **Implementation Highlights**:
  - Subclassing `MirFunctionPass`.
  - Iterating over blocks and instructions safely while mutating.
  - Inspecting `MirInteger` operand values.
  - Rerouting downstream virtual register consumers.
  - Adding the pass to a `MirPassManager`.

#### Example 3: Driving the End-to-End Compilation Pipeline Programmatically
- **Scenario**: Compiling an in-memory `MirModule` directly to a native `.o` or `.obj` object file using the C++ API.
- **Components Used**: `DriverContext`, `TargetResolver`, `TargetTriple`, `CompilationPipeline`, `EmissionEngine`.
- **Implementation Highlights**:
  - Initializing `EzTargets::X86_64::registerTarget()`.
  - Resolving target for `x86_64-pc-windows-msvc` (COFF) and `x86_64-unknown-linux-gnu` (ELF).
  - Executing Middle-End passes (SSA conversion, DCE, CFG simplification).
  - Executing Legalization (widening, narrowing, custom actions).
  - Running Target Instruction Selection (`MirInstructionSelector`).
  - Executing ABI and Frame Lowering (`MirAbiLowerer`, `MirFrameLowerer`).
  - Allocating physical registers via graph-coloring allocator (`MirRegisterAllocator`).
  - Emitting final binary object via `EmissionEngine`.

#### Example 4: Direct Binary Section Emission & Relocation Generation
- **Scenario**: Assembling binary instructions and declaring external symbol relocations without invoking the middle-end IR.
- **Components Used**: `CodeEmitterContext`, `CodeSection`, `DataNode`, `LabelNode`, `AlignNode`, `TargetCodeRelocationType`, `Elf64Writer`, `CoffWriter`.
- **Implementation Highlights**:
  - Creating `.text` and `.data` code sections.
  - Emitting raw machine code bytes (`DataNode`).
  - Defining local labels (`LabelNode`) and section alignment (`AlignNode`).
  - Adding symbol relocations (e.g., `X86_64_RELOC_BRANCH32`, `X86_64_RELOC_RIP_DISP32`).
  - Finalizing sections and exporting directly to a valid ELF64 or COFF object file.

#### Example 5: Textual MIR Specification & Ingestion
- **Scenario**: Reading and writing `.mir` textual files using `MirParser` and `MirPrinter`.
- **Implementation Highlights**:
  - Full syntax walkthrough of a `.mir` module containing globals, functions, attributes, and basic blocks.
  - Ingesting textual MIR via `MirParser::parseModule()`.
  - Handling syntax errors via `DiagnosticCollector` and `DiagnosticListener`.

---

## 4. Verification & Acceptance Criteria
1. All C++ code samples are syntactically and semantically compliant with the EzPacker C++20 API.
2. Code samples are properly fenced with `cpp` syntax highlighting and detailed line-by-line commentaries.
3. Doxygen renders the page with clickable cross-references to all mentioned classes (`MirBuilderContext`, `MirFunction`, `TargetResolver`, etc.).
