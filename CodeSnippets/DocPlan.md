# Implementation Plan: Comprehensive Subproject Documentation Suite

This plan establishes a documentation framework for the **EzPacker** compiler toolchain by producing exhaustive, production-grade architectural and technical `.md` documentation for all five core subprojects—**EzCore**, **EzMir**, **EzCodeEmitter**, **EzCompiler**, and **EzDsl**—as well as an overarching root repository **`README.md`**.

---

## User Review Required

> [!IMPORTANT]
> - **Existing Documentation Upgrade**: `EzCore/README.md`, `EzMir/README.md`, and `EzCodeEmitter/README.md` currently contain basic summaries (160–175 lines each) that omit several major subsystems (e.g. `IObjectWriter`/ELF/COFF writers in `EzCodeEmitter`, `MirParser` in `EzMir`, `DenseBitSet`/`IntrusiveLinkedList` in `EzCore`). These will be expanded into comprehensive, definitive guides.
> - **Brand New Documentation**: `EzCompiler` currently has **no** `README.md`. A complete architectural document will be authored covering `ezc`, `EzCompilerLib`, pass scheduling, target resolution, and binary emission.
> - **Root Repository Documentation**: Currently, `E:\Repos\EzPacker\README.md` does not exist. A high-level repository `README.md` will be created to tie the subprojects together with end-to-end architecture diagrams and quickstart instructions.
> - **Documentation Integrity**: Documentation will reflect the exact current codebase, including recent updates such as native target register banks in `.tdesc` and `EzDslGenRegisterInfo`.

---

## Subproject Documentation Matrix

```
                     ┌────────────────────────────────────────────────────────┐
                     │                  EzPacker (Root README)                │
                     │          Ecosystem Topology & End-to-End Flow          │
                     └───────────────────────────┬────────────────────────────┘
                                                 │
         ┌───────────────────┬───────────────────┼───────────────────┬───────────────────┐
         ▼                   ▼                   ▼                   ▼                   ▼
┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐
│     EzCore      │ │      EzMir      │ │  EzCodeEmitter  │ │   EzCompiler    │ │      EzDsl      │
│ Foundational    │ │ Multi-Tier IR,  │ │ Binary Emitter, │ │ Driver Engine,  │ │ Declarative DSL │
│ Utilities, Math,│ │ SSA Engine,     │ │ Section Nodes,  │ │ Pass Pipeline,  │ │ Suite, Lexer,   │
│ Diagnostics, PMR│ │ Passes, Parser  │ │ ELF/COFF Writers│ │ Target Resolver │ │ Sema, Codegen   │
└─────────────────┘ └─────────────────┘ └─────────────────┘ └─────────────────┘ └─────────────────┘
```

| Subproject | File Location | Current State | Target Scope & Key Focus Areas |
|:---|:---|:---|:---|
| **EzCore** | [`EzCore/README.md`](file:///E:/Repos/EzPacker/EzCore/README.md) | Basic overview (176 lines) | Diagnostic scopes & listeners, `FlexInt` (LibTomMath) / `FlexFloat` (LibBF), `SourceManager` & line binary search, `DenseBitSet`, `IntrusiveLinkedList`, `NameRegistry`, `StringUtils`, `CliExitCode`, PMR memory models. |
| **EzMir** | [`EzMir/README.md`](file:///E:/Repos/EzPacker/EzMir/README.md) | Mid-level summary (165 lines) | Multi-tier IR, Cytron et al. SSA & Cooper-Harvey-Kennedy dominators, CFG & liveness dataflow equations, TypeTable interning, built-in passes, all operand kinds, intrusive block/instruction linkage, `MirParser` (textual IR ingestion), `MirPrinter`. |
| **EzCodeEmitter** | [`EzCodeEmitter/README.md`](file:///E:/Repos/EzPacker/EzCodeEmitter/README.md) | Mid-level summary (165 lines) | Doubly-linked node section stream (`DataNode`, `LabelNode`, `AlignNode`), multi-pass `finalize()` serialization, post-finalization binary patching, `CodeRelocation` types, `IObjectWriter`, `Elf64Writer`, `CoffWriter`, target emitter seam. |
| **EzCompiler** | [`EzCompiler/README.md`](file:///E:/Repos/EzPacker/EzCompiler/README.md) | **MISSING** (0 lines) | Standalone compiler driver `ezc`, `EzCompilerLib`, `CommandLineParser` & options, `DriverContext` session lifecycle, `TargetTriple` parsing/normalization, `TargetResolver` factory pattern, `CompilationPipeline` (middle-end, legalization, target lowering), `EmissionEngine` object packaging. |
| **EzDsl** | [`EzDsl/README.md`](file:///E:/Repos/EzPacker/EzDsl/README.md) | Comprehensive (458 lines) | 9 DSL sub-languages (`.tdesc`, `.idf`, `.ezcc`/`.ccd`, `.irdf`, `.lad`, `.lrd`, `.isf`, `.tyf`), inline target register banks/classes/specials, Lexy zero-copy parsing, unified Sema passes, 10 C++ code generators, `EzDslCli` driver, CMake integration macros. |
| **Root Repository** | [`README.md`](file:///E:/Repos/EzPacker/README.md) | **MISSING** (0 lines) | Global architecture overview, directory layout, end-to-end pipeline sequence diagram, build prerequisites, CMake build & test instructions, subproject navigation links. |

---

## Detailed Specification per Documentation Artifact

### 1. `EzCore/README.md` (Comprehensive Upgrade)

1. **Executive Summary & Role**: Foundational utility library shared across all compiler layers.
2. **Architecture & Subsystems Diagram**: Topology mapping diagnostics, math, source tracking, and memory to compiler phases.
3. **Diagnostics Management Subsystem**:
   - `DiagnosticCollector`: Thread safety, severity levels (`Diag_Trace` to `Diag_Fatal`), listener registration, scope management.
   - `DiagnosticBuilder`: RAII flush semantics, zero-allocation short-circuiting when disabled, stream chaining (`<<`).
   - `DiagnosticScope` & `DiagnosticScopeAction`: Exact mechanics of `Commit`, `Discard` (speculative parsing/backtracking), and `Propagate`.
   - `DiagnosticMessage` & `DiagnosticNote`: Immutable records with `SourceReference*`.
   - `DiagnosticListener` & `DiagnosticLogger`: Formatted console/file rendering with ANSI color highlights and source line carets (`^~~~~`).
4. **FlexNumber Arbitrary-Precision Math Subsystem**:
   - `FlexInt`: LibTomMath (`mp_int`) integration, configurable bitwidths (1..1024+ bits), signedness, strict two's-complement clamping (`clampToTwosComplement()`), overflow/underflow detection, bitwise/arithmetic operators, target endianness dumping (`dump()`).
   - `FlexFloat`: LibBF (`bf_t`) integration, IEEE-754 precision modes (half, single, double, quad), precision clamping, rounding modes (`BF_RNDN`), binary layout serialization (`dump()`).
   - `LibBFWrapper`: RAII wrapper ensuring deterministic LibBF context/memory cleanup.
5. **Source Management & Tracking Subsystem**:
   - `SourceManager` & `GenericSourceManager`: PMR buffer registry, dense numeric file IDs, canonical path resolution, search paths (`addIncludePath`, `resolveSourcePath`, `loadFile`).
   - `SourceReference`: 16-byte token/span reference (`startOffset`, `endOffset`, `sourceFileId`).
   - Binary-Searched Line Lookup: O(log N) line and column translation, line range slicing (`getReferenceLine`, `getRawLineContent`, `getReferenceContent`).
6. **Helper Classes & Utilities**:
   - `DenseBitSet`: Fast bitset for liveness bitvectors and dataflow sets.
   - `IntrusiveLinkedList`: Zero-allocation intrusive doubly-linked list node container used for blocks and instructions.
   - `NameRegistry`: Thread-safe symbol deduplication and canonical name interning.
   - `StringUtils`: High-speed string manipulation routines (trimming, case transformation, token splitting, hex/decimal parsing).
   - `CliExitCode`: Standardized process exit codes (`kSuccess`, `kError`, `kFatalException`).
7. **Memory Model & Lifecycle**: Polymorphic Memory Resources (`std::pmr`), arena allocators, thread isolation.
8. **Working Code Examples**: Complete C++ snippets demonstrating diagnostics scopes, arbitrary-precision constant folding, and source location queries.
9. **Testing & CMake Integration**: Build commands, linking requirements (`EzCore`), and test fixtures in `EzCore/tests/`.

---

### 2. `EzMir/README.md` (Comprehensive Upgrade)

1. **Executive Summary & Role**: Strongly-typed, multi-tier Machine Intermediate Representation and middle-end compilation framework.
2. **Multi-Tier IR Architecture**:
   - **High-Level IR**: Abstract target-independent SSA operations (`ADD`, `SUB`, `CALL`, `RET`, `PHI`, etc.).
   - **Pass-Internal IR**: Lowering primitives (`PUSH_ARG`, `POP_ARG`, `PUSH_RET`, `POP_RET`).
   - **Target-Low IR**: Post-instruction-selection target instructions bound to physical register classes and encodings.
3. **Core Hierarchy & Containers**:
   - `MirBuilderContext`: Central module container owning the PMR arena, `MirTypeTable`, `DiagnosticCollector`, and entity index maps.
   - Builders: `MirFunctionBuilder`, `MirBlockBuilder`, `MirInstructionBuilder`, `MirGlobalVarBuilder`, `MirClassBuilder`.
   - Intrusive Doubly-Linked List Model: `MirBlock` owning `MirInstruction` nodes via `IntrusiveLinkedList`.
4. **Instruction & Operand Model**:
   - `MirInstruction`: Opcode (`MirInstructionOpCode`), category, tier, flags (`IsCommutative`, `ReadsMemory`, `WritesMemory`, `HasSideEffects`), `SourceReference*`, operands vector.
   - `MirOperand` Hierarchy:
     - `MirRegister`: Virtual registers (`vreg(id)`) vs physical registers (`preg(desc)`).
     - `MirRegisterRef`: Lightweight register reference.
     - `MirRegisterClass` & `MirRegisterBank`: Hardware register organization and sub-register aliasing.
     - `MirInteger` (`FlexInt`) & `MirFloat` (`FlexFloat`): Compile-time numeric literals.
     - `MirMemory`: Base-plus-displacement address calculation (`[baseReg + disp]`).
     - `MirReference`: Symbolic targets for blocks, functions, globals, and stack slots.
     - `MirRuntimeSymbol`: Link-time external runtime symbols.
5. **Type System & Layout**:
   - `MirTypeTable`: Canonical interned primitive types (`i1`..`i128`, `f16`..`f128`, `void`), pointers, arrays, and classes.
   - `IMirTargetTypeLayout`: Interface for target-specific type sizing, alignment, and struct padding.
6. **SSA Infrastructure & Dominance Algorithms**:
   - Automated Non-SSA to SSA conversion (`NonSsaToSsaPass`).
   - **Cooper-Harvey-Kennedy** dominator tree and immediate dominator calculation.
   - Dominance frontier ($\text{DF}$) and iterated dominance frontier ($\text{IDF}$) computation for minimal $\phi$-node placement.
   - Variable version renaming stack algorithm.
7. **Built-in Middle-End Passes**:
   - `MirPassManager`: Automatic topological dependency sorting, on-demand analysis caching, pipeline generation.
   - `CodeFlowAnalysisPass`: CFG construction, predecessor/successor graph calculation, loop detection.
   - `LivenessAnalysisPass`: Backward dataflow analysis, `def`/`use` calculation, `liveIn`/`liveOut` intervals for register allocation.
   - `ClassOffsetResolverPass`: OOP field offset and virtual method table layout.
   - `RelativeReferenceLowererPass`: Lowers symbolic references to concrete base+disp memory.
8. **ABI & Calling Conventions**:
   - `CallingConvDesc`: Abstract ABI definition (stack direction, alignment, shadow space, caller-saved, callee-saved, argument and return location mapping via `ArgumentLocationDesc`).
   - `MirFunctionStackFrame`: Local stack frame layout, spill slots, parameter areas.
9. **Textual MIR Serialization & Parsing**:
   - `MirPrinter`: Configurable formatting (`General` vs `Detailed`), symbol table printing.
   - `MirParser`: Complete Lexy-based parser (`MirParser`, `MirLexer`, `MirParserContext`, `MirAstNodes`), enabling textual `.mir` round-tripping.
10. **Working Code Examples & Testing**: Comprehensive C++ examples building functions, running optimization passes, and disassembling IR; test mapping to `tests/EzMirTestSuite/`.

---

### 3. `EzCodeEmitter/README.md` (Comprehensive Upgrade)

1. **Executive Summary & Role**: Low-level machine code emission, section layout, relocation fixups, and binary object packaging.
2. **Doubly-Linked Section Node Stream**:
   - Non-linear code generation via linked stream nodes:
     - `DataNode`: Raw byte chunks.
     - `LabelNode`: Symbolic label markers (`CodeLabel`).
     - `AlignNode`: Alignment directives with target padding bytes (`0x90` NOP for code, `0x00` for data).
   - Advantages over flat buffers: O(1) arbitrary insertion, out-of-order block layout, late jump table embedding.
3. **Section & Emission Context**:
   - `CodeSection`: Emit primitives (`emit8`, `emit16`, `emit32`, `emit64`, `emitBytesWithEndian`), label binding (`bindLabel`), alignment (`alignTo`), `finalize()` multi-pass linearization, post-finalization patching (`patch32`, `patch64`, `patchBytesWithEndian`).
   - `CodeEmitterContext`: Coordinates multi-section emission across a compilation unit (`.text`, `.rodata`, `.data`, `.bss`), function-local label registry, relocation accumulation.
4. **Relocation Model**:
   - `CodeRelocation`: Relocation types (`Absolute32`, `Absolute64`, `PCRel32`, `BranchRel32`, `GOTPCREL`, `PLTRel32`).
   - Association with source `MirReference` objects and section offsets.
5. **Object Format Writers (`ObjectFormat/`)**:
   - `IObjectWriter`: Unified interface for emitting binary object files to disk.
   - `Elf64Writer`: System V ELF64 object generator (`.text`, `.rodata`, `.data`, `.bss`, symbol tables `.symtab`/`.strtab`, section headers, relocation tables `.rela.text`).
   - `CoffWriter`: Windows PE/COFF object generator (`.text`, `.rdata`, `.data`, `.bss`, COFF symbol table, string table, COFF relocations, optional `.pdata` unwind tables).
   - `ObjectSymbol`: Representation of exported, internal, and external/undefined symbols.
   - Object format section factory presets (`CreateElfSections`, `CreateCoffSections`, `CreateMachoSections`).
6. **Target Integration Seam**:
   - `GenericCodeEmitter`: Target backend emitter interface (`beginFunction`, `bindLabel`, `emitInst`, `endFunction`).
   - `EncodeResult`: Status and byte count descriptor for machine instruction encoding.
7. **Working Code Examples & Testing**: Complete C++ example setting up ELF/COFF sections, emitting machine instructions, binding labels, and writing an object file; test mapping to `tests/EzCodeEmitterTestSuite/`.

---

### 4. `EzCompiler/README.md` (Brand New Comprehensive Documentation)

1. **Executive Summary & Role**: End-to-end compiler driver executable (`ezc`) and reusable compilation library (`EzCompilerLib`).
2. **Architecture & Compilation Pipeline Flow**:
   - Mermaid diagram tracing the pipeline from source/MIR input through middle-end, legalization, target lowering, and binary emission.
3. **Command-Line Interface & Parser (`CommandLineOptions.h`, `CommandLineOptions.cpp`)**:
   - Argparse-based parser (`CommandLineParser`).
   - Compilation flags reference table:
     - Input & output (`-i`, `-o`).
     - Target triple selection (`--target <triple>`).
     - Inspection gates (`--emit-mir`, `--emit-legalized-mir`, `--emit-lowered-mir`, `--emit-asm`, `--emit-obj`).
     - Optimization levels (`-O0`, `-O1`, `-O2`, `-Os`).
     - Diagnostics & logging controls (`-v`, `--verbose`, `--diag-out <path>`, `--diag-threshold <level>`, `--print-passes`, `--time-passes`).
     - Target features (`-mattr=+avx,-sse`).
     - Position-independent code (`-fPIC`).
   - `EmissionStage` and `OptimizationLevel` enums.
4. **Target Triple & Target Resolution**:
   - `TargetTriple`: Canonical `<arch>-<vendor>-<sys>-<abi>` representation, host triple auto-detection (`getHostTriple()`), shorthand parsing, format predicates (`isX86_64`, `isWindows`, `isLinux`, `isElf`, `isCoff`).
   - `TargetResolver`: Pluggable architecture factory registry (`registerTarget`, `resolve`), returning `ResolvedTarget` (`m_targetDesc`, `m_callingConv`, `m_binaryDesc`).
5. **Execution Driver Context (`DriverContext.h`, `DriverContext.cpp`)**:
   - Compilation session lifecycle, 1MB monotonic session arena.
   - Resource ownership: `SourceManager`, `DiagnosticCollector`, `DiagnosticLogger`, `MirTypeTable`, `MirBuilderContext`, and resolved target descriptors.
6. **Frontend Ingestion & Module Loading (`FrontendAdapter.h`, `FrontendAdapter.cpp`)**:
   - `IFrontendAdapter`: Abstraction for frontends translating high-level code to MIR.
   - `MirModuleLoader`: Textual `.mir` file loader and synthetic module generator for tests and compiler pipelines.
7. **Pass Pipeline Orchestrator (`CompilationPipeline.h`, `CompilationPipeline.cpp`)**:
   - Three-phase pass execution per function:
     - **Middle-End Phase**: `CodeFlowAnalysisPass` (CFG), `NonSsaToSsaPass` (SSA), `LivenessAnalysisPass` (live intervals).
     - **Legalization Phase**: Function signature and instruction legalization via `MirLegalizer` driven by target legalization action tables.
     - **Target Lowering Phase**: `MirAbiLowerer` (ABI argument/return lowering), `MirInstructionSelector` (target pattern matching / instruction selection), `MirRegisterAllocator` (physical register allocation), `MirFrameLowerer` (prologue/epilogue, stack slot offsets).
   - Inspection early-exit gates and formatting methods (`dumpCurrentMir()`, `dumpAssembly()`).
8. **Binary Emission Engine (`EmissionEngine.h`, `EmissionEngine.cpp`)**:
   - Translates lowered MIR into machine code via `GenericCodeEmitter`.
   - Populates defined and undefined symbols (`ObjectSymbol`).
   - Resolves branches and fixups via `TargetRelocationResolver`.
   - Delegates binary object creation to `Elf64Writer` or `CoffWriter`.
9. **Executable Entry Point (`src/Main.cpp`)**:
   - Target registration (`EzTargets::X86_64::registerTarget()`), error reporting, CLI exit codes.
10. **Usage Examples & Testing**:
    - CLI usage examples (compiling `.mir` to `.o` / `.obj`, dumping intermediate stages).
    - C++ programmatic API usage examples.
    - Test mapping to `tests/EzCompilerTestSuite/` and `tests/EzCompilerEndToEndTests/`.

---

### 5. `EzDsl/README.md` (Polish & Refinement)

1. **Executive Summary & Role**: Declarative backend description language suite.
2. **Language Specifications**:
   - Complete coverage of all 9 sub-languages:
     - Target Descriptor (`.tdesc`): object formats, pointer/stack sizes, libcalls, components, extensions, register banks, classes, sub-register hierarchies, special registers.
     - Target Instructions (`.idf`): encodings, operand classes, mnemonics, implicit registers, flags.
     - Calling Conventions (`.ezcc` / `.ccd`): stack layout, preservation sets, classification, argument/return lowering, SRET.
     - Generic IR Definitions (`.irdf`): MIR opcodes, categories, tiers, flags.
     - Legalization Actions (`.lad`): type legality tables, promotions, splits.
     - Legalization Rewrite Rules (`.lrd`): IR-to-IR pattern matching and expansion.
     - Instruction Selection Patterns (`.isf`): addressing modes, DAG patterns, costs, emits.
     - Type Definitions (`.tyf`): primitive bitwidths, types.
3. **Lexer, Parser & AST Foundation**: Lexy zero-copy parsers, error recovery, source reference tracking.
4. **Semantic Analysis Pipeline**: `TargetDescPass` (consolidated register and target validation), `TypePass`, `IrInstructionPass`, `CallingConvPass`, `LegalizeActionPass`, `LegalizeRulePass`, `TargetInstPass`, `InstructionSelectPass`.
5. **Code Generators**: 10 C++ generators emitting production tables.
6. **CLI Driver (`EzDslCli`)**: Auto-discovery rules, command-line flags, inspection and dumping tools.
7. **CMake Build System Integration**: Complete reference for all CMake macros (`EzDslGenCallingConv`, `EzDslGenRegisterInfo`, `EzDslGenTargetInstructions`, `EzDslGenLegalizerActionTable`, `EzDslGenInstructionSelector`, `EzDslGenerateTypeTable`, `EzDslGenMirInstructions`).
8. **Memory Architecture & Testing**: PMR memory model and test suite breakdown.

---

### 6. Root `README.md` (Brand New Repository Documentation)

1. **Project Overview**: Introduction to EzPacker as a modern, modular, production-grade compiler backend toolchain.
2. **Architecture Diagram**: End-to-end compiler pipeline and inter-project dependency graph.
3. **Subprojects Index**: Quick-reference table linking directly to `EzCore`, `EzMir`, `EzCodeEmitter`, `EzCompiler`, `EzDsl`, and `EzTargets`.
4. **End-to-End Compilation Workflow**: Walkthrough showing how source code / MIR flows through frontends, middle-end SSA passes, target legalization, instruction selection, register allocation, frame lowering, and binary object emission.
5. **Build Prerequisites & Quickstart**:
   - Supported platforms: Windows (MSVC/Clang), Linux (GCC/Clang).
   - CMake configuration and build commands (`cmake -B cmake-build-debug -G Ninja`, `cmake --build cmake-build-debug`).
   - Running the test suite (`ctest --test-dir cmake-build-debug --output-on-failure`).
6. **Repository Layout**: Directory map explaining the role of each top-level folder.

---

## Verification Plan

### Automated Build & Test Validation
- Ensure all 93 existing tests continue to build and pass cleanly without interference:
  ```powershell
  cmake --build cmake-build-debug
  ctest --test-dir cmake-build-debug --output-on-failure
  ```

### Documentation Quality & Link Integrity Verification
- Verify all relative markdown file links (`file://` and repo-relative paths) resolve correctly.
- Verify code blocks have proper syntax highlighting annotations (`cpp`, `cmake`, `bash`, `dsl`).
- Verify Mermaid diagrams render without syntax errors.
- Ensure all mentioned classes, methods, flags, and headers match the actual codebase symbols.

### Knowledge Graph Update
- In accordance with `.agents/rules/graphify.md`, run:
  ```powershell
  graphify update .
  ```
