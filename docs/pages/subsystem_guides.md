# Subsystems Architectural Overview {#subsystem_guides}

This guide provides an in-depth architectural breakdown of the 7 core subsystems comprising **EzPacker**.

---

## 🗺️ Subsystem Architecture & Dependency Hierarchy

```
                      ┌──────────────────────────────────────────────┐
                      │              EzCompiler (ezc)                │
                      │  Compiler Driver, Pipeline Engine & Registry │
                      └──────────────────────┬───────────────────────┘
                                             │
                      ┌──────────────────────┼───────────────────────┐
                      ▼                                              ▼
       ┌──────────────────────────────┐               ┌──────────────────────────────┐
       │          EzTargets           │               │           EzTriple           │
       │ Concrete Target Backends     │               │ Platform & Triple Normalizer │
       │ (x86_64, RISC-V, ARM64)      │               └──────────────────────────────┘
       └──────────────┬───────────────┘
                      │
       ┌──────────────┴───────────────┬──────────────────────────────┐
       ▼                              ▼                              ▼
┌──────────────┐               ┌──────────────┐               ┌──────────────┐
│    EzMir     │               │    EzDsl     │               │EzCodeEmitter │
│ Multi-Tier   │               │ Declarative  │               │ Stream Nodes │
│ SSA IR &     │               │ Languages &  │               │ & ELF/COFF   │
│ Optimizers   │               │ Generators   │               │ Writers      │
└──────┬───────┘               └──────┬───────┘               └──────┬───────┘
       │                              │                              │
       └──────────────────────────────┼──────────────────────────────┘
                                      ▼
                       ┌──────────────────────────────┐
                       │            EzCore            │
                       │ Utilities, Arbitrary Math,   │
                       │ Diagnostics & Source Tracking│
                       └──────────────────────────────┘
```

---

## 1. EzCore: Foundational Infrastructure

The **EzCore** subsystem ([`EzCore`](@ref EzCore)) provides zero-dependency foundational data structures, diagnostic handling, and arbitrary-precision arithmetic shared across all compiler layers.

### Subsystem Highlights
- **Diagnostics Subsystem**:
  - `DiagnosticCollector`: Thread-safe diagnostic aggregator with support for custom listeners (`DiagnosticListener`, `ConsoleDiagnosticListener`).
  - `DiagnosticBuilder`: Zero-allocation RAII stream builder (`diags.report(level) << "msg"`).
  - `DiagnosticScope`: Transactional diagnostic control with `Commit`, `Discard` (for speculative syntax exploration), and `Propagate`.
- **Arbitrary-Precision Arithmetic**:
  - `FlexInt`: Multi-precision integer arithmetic engine backed by LibTomMath (`mp_int`), supporting arbitrary bitwidths (1..1024+ bits), signed/unsigned arithmetic, strict two's-complement clamping (`clampToTwosComplement()`), and target endianness binary dumping.
  - `FlexFloat`: Arbitrary-precision floating-point arithmetic backed by LibBF (`bf_t`), supporting IEEE-754 half (16-bit), single (32-bit), double (64-bit), and quad (128-bit) precision modes with deterministic precision clamping.
- **Source Management & Tracking**:
  - `SourceManager`: Registry of loaded file buffers backed by polymorphic memory resources.
  - `SourceReference`: 16-byte packed token span (`startOffset`, `endOffset`, `sourceFileId`).
  - Fast O(log N) line and column translation via binary-searched line table.
- **Utility Containers**:
  - `DenseBitSet`: Fast bitset for liveness bitvectors and dataflow analysis.
  - `IntrusiveLinkedList`: Zero-allocation intrusive doubly-linked list container used for instruction and block sequences.
  - `NameRegistry`: Thread-safe symbol deduplication and canonical string interning.

---

## 2. EzMir: Intermediate Representation & SSA

The **EzMir** subsystem ([`EzMir`](@ref EzMir)) implements a strongly typed, multi-tier Machine Intermediate Representation and optimization framework.

### Subsystem Highlights
- **Multi-Tier Classification**:
  - **High-Level IR**: Abstract target-independent SSA operations (`ADD`, `SUB`, `CALL`, `RET`, `PHI`, `LOAD`, `STORE`).
  - **Pass-Internal IR**: Lowering primitives (`PUSH_ARG`, `POP_ARG`, `PUSH_RET`, `POP_RET`).
  - **Target-Low IR**: Post-instruction-selection instructions bound to hardware registers and encodings.
- **Containers & Builders**:
  - `MirBuilderContext`: Central module context owning the PMR memory arena, `MirTypeTable`, and `DiagnosticCollector`.
  - `MirFunctionBuilder`, `MirBlockBuilder`, `MirInstructionBuilder`, `MirOperandBuilder`.
  - Intrusive doubly-linked list containment: `MirBlock` owns `MirInstruction` nodes with zero heap overhead during instruction mutation.
- **SSA Construction & Optimizations**:
  - `NonSsaToSsaPass`: Cytron et al. SSA transformation with Cooper-Harvey-Kennedy dominator tree computation and iterated dominance frontier calculation.
  - `CodeFlowAnalysisPass`: Control flow graph analysis and predecessor/successor tracking.
  - `LivenessAnalysisPass`: Backward dataflow analysis using `DenseBitSet` liveness bitvectors.
- **Textual Serialization**:
  - `MirPrinter`: Formats in-memory MIR into readable `.mir` text with operand typing.
  - `MirParser`: Ingests `.mir` text into fully validated in-memory modules.

---

## 3. EzDsl: Declarative Target Suite

The **EzDsl** subsystem ([`EzDsl`](@ref EzDsl)) provides a declarative modeling ecosystem consisting of 9 domain-specific sub-languages:

| Sub-Language | Extension | Description |
|:---|:---|:---|
| **Target Descriptors** | `.tdesc` | Target metadata, pointer size, stack slot size, inline register banks, register classes, sub-register hierarchies, and special registers. |
| **Instruction Definitions** | `.idf` | Target instruction mnemonics, operands, directions (`IN`, `OUT`), behavioral flags, and encoding templates. |
| **Calling Conventions** | `.ezcc` | Stack layout, shadow space, red zones, callee-saved preservation sets, parameter passing and return rules. |
| **Generic IR Definitions** | `.irdf` | Generic intermediate opcode declarations, type masks, categories, and tiers. |
| **Legalization Actions** | `.lad` | Target legality declarations (`LEGAL`, `WIDENS`, `NARROWS`, `LIBCALL`, `CUSTOM`, `CLAMP_SCALAR`). |
| **Legalization Rules** | `.lrd` | Multi-word pattern rewrite rules (e.g. 128-bit arithmetic splitting). |
| **Instruction Selection** | `.isf` | Tree-pattern matching rules, addressing modes, cost heuristics, and target selector stubs. |
| **Type Definitions** | `.tyf` | Declarative primitive and composite type definitions. |

### Subsystem Highlights
- **Lexy Zero-Copy Parser**: High-performance parser combinator engine producing structured AST nodes.
- **Multi-Pass Semantic Analysis (Sema)**: Symbol resolution, register aliasing checks, and legality verification.
- **10 C++ Code Generators**: Emits C++ headers for instruction tables, pattern selectors, calling conventions, and register info.

---

## 4. EzCodeEmitter: Binary Section & Object Packaging

The **EzCodeEmitter** subsystem ([`EzCodeEmitter`](@ref EzCodeEmitter)) linearizes post-allocation target instructions into binary sections and exports standard relocatable object files.

### Subsystem Highlights
- **Doubly-Linked Section Stream**:
  - `CodeSection`: Manages ordered streams of nodes with section flags (`Alloc`, `Exec`, `Write`).
  - `DataNode`: Raw byte arrays of encoded instructions and immediate data.
  - `LabelNode`: Symbolic anchors for local and global branches.
  - `AlignNode`: Hardware padding nodes enforcing alignment (4, 8, 16 bytes).
- **Two-Pass Section Finalization**:
  - Pass 1: Computes absolute and relative byte offsets for all nodes, resolving label positions.
  - Pass 2: Evaluates PC-relative displacements and branch relaxation.
- **Binary Relocations**:
  - `TargetCodeRelocationType`: Architecture-neutral relocation kinds (`Branch32`, `Call32`, `Absolute64`, `RipDisp32`).
- **Object Format Serialization**:
  - `Elf64Writer`: Generates 64-bit ELF relocatable objects with symbol tables (`.symtab`), string tables (`.strtab`), and relocation tables (`.rela.text`).
  - `CoffWriter`: Generates Windows x64 COFF object files (`.obj`) with section headers and relocation entries.

---

## 5. EzTriple: Target Environment Normalization

The **EzTriple** subsystem ([`EzTriple`](@ref EzTriple)) parses and canonicalizes target triples:

```
<architecture>-<vendor>-<operating_system>-<environment>
```

### Subsystem Highlights
- **Normalization**: Translates synonyms (e.g. `amd64`, `x86_64`, `x64` => `x86_64`).
- **Predicate Queries**:
  - `isWindows()`, `isLinux()`, `isDarwin()`.
  - `isCoff()`, `isElf()`, `isMachO()`.
  - `getArch()`, `getVendor()`, `getOS()`, `getEnvironment()`.

---

## 6. EzCompiler: Driver & Pipeline Engine

The **EzCompiler** subsystem ([`EzCompiler`](@ref EzCompiler)) coordinates the compilation process from invocation to binary emission.

### Subsystem Highlights
- **Compiler CLI (`ezc`)**: Standalone binary driver supporting optimization flags, target selection, and pass dumping.
- **Session Lifecycle**:
  - `DriverContext`: Holds driver lifetime state, options, diagnostic collector, and builder contexts.
  - `CommandLineParser`: Flexible command-line argument tokenizer and validator.
- **Target Resolver Registry**:
  - `TargetResolver`: Pluggable architecture factory registry (`registerTarget`, `resolve`).
- **Multi-Phase Compilation Pipeline**:
  - `CompilationPipeline`: Coordinates the phased compilation sequence:
    1. **Middle-End Optimization**: SSA transformation, dead code elimination, CFG simplification.
    2. **Target Legalization**: Table-driven type widening, narrowing, and libcall lowering.
    3. **Target Instruction Selection**: Pattern matching generic MIR to target machine instructions.
    4. **ABI & Frame Lowering**: Argument placement, return handling, stack frame layout, prologue/epilogue emission.
    5. **Register Allocation**: Graph-coloring physical register allocation and spill handling.
    6. **Object Emission**: `EmissionEngine` linearizes sections and invokes `Elf64Writer` or `CoffWriter`.

---

## 7. EzTargets: Architecture Implementations

The **EzTargets** subsystem ([`EzTargets`](@ref EzTargets)) houses concrete hardware backend implementations.

### Subsystem Highlights (Reference `x86_64` Backend)
- `X86_64TargetDesc`: Concrete target descriptor managing GPR and FPR register banks.
- `X86_64FrameLowerer`: System V AMD64 and Windows x64 frame layout, stack alignment, and callee-saved preservation.
- `X86_64TargetInstructionSelector`: Tree-pattern selection combined with handwritten fallbacks for control flow, floating point, and vector math.
- `X86_64InstructionEncoder`: Machine code byte encoder for ModR/M, SIB, REX, and VEX prefixes.
- `X86_64RelocationResolver`: Branch and RIP-relative displacement fixups.
- `X86_64ElfBinaryDesc` & `X86_64CoffBinaryDesc`: Relocation type translation for ELF64 and Windows COFF.

---

> [!NOTE]
> Explore class-level APIs and namespaces directly in the **[Modules Reference](topics.html)** or the **[Alphabetical Class Index](annotated.html)**.
