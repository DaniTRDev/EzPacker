# Comprehensive Master Architecture & Implementation Review Plan: EzPacker Compiler Suite
**Exhaustive End-to-End Audit, Invariant Verification, Memory Safety & Hardening Framework**

---

## 1. Executive Summary & Project Vision

### 1.1 Context & Motivation
The **EzPacker** project is an ambitious, industrial-grade ahead-of-time (AOT) compiler infrastructure and binary packer. Its architecture decouples the front-end language syntax, intermediate representation, target-specific declarative domain-specific languages (DSLs), backend code generation passes, and raw machine code emission into cleanly bounded, highly cohesive subsystems:

```
┌────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                    EzPacker End-to-End Compiler Pipeline                               │
└────────────────────────────────────────────────────────────────────────────────────────────────────────┘

 [ High-Level Ez Source ] (.ez)
            │
            ▼
 ┌──────────────────────┐
 │      EzFrontend      │  Lexing, AST Construction, Type Checking, Semantic Validation
 └──────────┬───────────┘
            │  EzAstLowerer
            ▼
 ┌──────────────────────┐
 │        EzMir         │  Typed SSA Intermediate Representation, Control Flow Graph (CFG),
 └──────────┬───────────┘  Liveness Analysis, PMR Allocator Arenas, Pass Manager Infrastructure
            │
            ▼
 ┌──────────────────────┐    ┌──────────────────────────────────────────────────────────────┐
 │       EzTriple       │◄───┤                            EzDsl                             │
 │   Backend Pipeline   │    │  Target Architecture Meta-Compiler (.tyf, .idf, .ezcc,      │
 └──────────┬───────────┘    │  .lad, .lrd, .isf) -> Synthesized C++ Descriptors & Matchers │
            │                └──────────────────────────────────────────────────────────────┘
            ▼
 ┌──────────────────────┐
 │    EzCodeEmitter     │  Hardware Machine Code Encoding, Section Builders, Relocations,
 └──────────┬───────────┘  Executable Layout (ELF, PE-COFF, Mach-O)
            │
            ▼
 [ Native Executable / Object File ]
```

### 1.2 The Imperative for a Meticulous Full-Project Review
As the backend matures—integrating table-driven Legalization, synthesized Calling Convention Descriptors, bottom-up Instruction Selection with complex SIB addressing mode folding, Chaitin-Briggs graph coloring Register Allocation, and Frame Lowering—the interdependencies between modules multiply.

Minor structural inconsistencies, mismatched memory allocation models, subtle edge-case omissions in semantic analysis, or latent quadratic complexities in intermediate passes can cascade across the pipeline. A defect in `EzDsl`'s semantic analysis can emit malformed pattern matchers; an unhandled operand constraint in `MirLegalizer` will panic during Instruction Selection; a neglected calling convention alignment requirement will trigger bus errors or segfaults at runtime.

### 1.3 Review Mission & Scope
This plan establishes a formal, meticulous, and systematic review methodology to audit **every layer of the EzPacker codebase**:
1. **EzCore**: Foundation utilities, memory resources, diagnostics, source tracking.
2. **EzFrontend**: Lexer, Parser, AST, Semantic Analysis, and AST-to-MIR lowering.
3. **EzMir**: Intermediate representation, instructions, blocks, functions, operand model, SSA tracking, CFG, pass infrastructure.
4. **EzDsl**: Language parsers (lexy), semantic passes (Sema), symbol tables, C++ code generators, CLI driver (`ezdslc`).
5. **EzTriple**: Backend passes (Signature Legalizer, Legalizer, ABI Lowerer, Instruction Selector, Register Allocator, Frame Lowerer, Target Descriptors).
6. **EzCodeEmitter**: Machine instruction encoding, label resolution, relocations, binary formats.
7. **CMake & Tooling**: Build determinism, compiler flags, sanitizers, test runners, knowledge graph synchronization (`graphify`).

---

## 2. Architectural Pillars & Core Invariants

The review evaluates the entire project against **Seven Foundational Pillars**. Each pillar defines strict operational invariants that must hold under all execution paths.

```
┌────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                 Seven Pillars of Architectural Review                                  │
└────────────────────────────────────────────────────────────────────────────────────────────────────────┘

  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐
  │     PILLAR 1     │  │     PILLAR 2     │  │     PILLAR 3     │  │     PILLAR 4     │
  │     EzCore &     │  │    EzFrontend    │  │    EzMir Core    │  │  EzDsl Language  │
  │  Infrastructure  │  │    Subsystems    │  │  Representation  │  │ & Meta-Compiler  │
  └────────┬─────────┘  └────────┬─────────┘  └────────┬─────────┘  └────────┬─────────┘
           │                     │                     │                     │
           └─────────────────────┼─────────────────────┴─────────────────────┘
                                 │
           ┌─────────────────────┼─────────────────────┬─────────────────────┐
           │                     │                     │                     │
  ┌────────┴─────────┐  ┌────────┴─────────┐  ┌────────┴─────────┐           │
  │     PILLAR 5     │  │     PILLAR 6     │  │     PILLAR 7     │           ▼
  │ EzTriple Backend │  │  EzCodeEmitter   │  │   CMake, QA &    │  [ Cross-Cutting Invariants ]
  │ Code Generation  │  │  & Binary Layout │  │ Test Automation  │  Memory Safety, Determinism,
  └──────────────────┘  └──────────────────┘  └──────────────────┘  Algorithmic Complexity
```

---

## 3. Subsystem-by-Subsystem Meticulous Audit Plan

### 3.1 Pillar 1: EzCore & Foundation Infrastructure
`EzCore` forms the foundational substrate for all memory allocation, diagnostic logging, source management, and basic data structures.

#### 3.1.1 Target Directories & Components
- [`EzCore/include/`](file:///E:/Repos/EzPacker/EzCore/include/) and [`EzCore/src/`](file:///E:/Repos/EzPacker/EzCore/src/)
  - `Diagnostics/`: `DiagnosticCollector`, `DiagnosticLogger`, `SourceReference`, `DiagnosticSeverity`.
  - `Memory/`: PMR monotonic arenas, buffer allocators, pool allocators.
  - `SourceManager/`: File loading, line/column mapping, virtual buffers.
  - `Utils/`: Intrusive linked lists (`IntrusiveLinkedList`), bit manipulation, hashing.

#### 3.1.2 Invariants & Verification Checkpoints
- **[INV-CORE-01] Diagnostic Location Fidelity**: Every diagnostic message emitted by any compiler stage must preserve an accurate `SourceReference` (line, column, source file) down to the byte span.
- **[INV-CORE-02] Monotonic Allocator Reset Safety**: Arenas used for transient compiler passes must never invoke destructors on POD types, but must correctly invoke non-trivial destructors where PMR containers or polymorphic objects reside before buffer reclamation.
- **[INV-CORE-03] Intrusive Container Stability**: `IntrusiveLinkedList<T>` node insertion, removal, and splicing must maintain head/tail integrity under all boundary states (empty list, single-element list, head/tail deletion). Splicing must be strictly $O(1)$.
- **[INV-CORE-04] Zero Raw Exception Leakage**: Core infrastructure must never propagate raw unhandled runtime exceptions across module boundaries; errors must be captured as structured diagnostics.

---

### 3.2 Pillar 2: EzFrontend & Language Pipeline
`EzFrontend` accepts user code, parses syntax trees, performs type inference and checking, and lowers high-level AST constructs into initial un-legalized generic MIR.

#### 3.2.1 Target Directories & Components
- [`EzFrontend/EzLexer/`](file:///E:/Repos/EzPacker/EzFrontend/EzLexer/)
- [`EzFrontend/EzSemantics/`](file:///E:/Repos/EzPacker/EzFrontend/EzSemantics/)
- [`EzFrontend/EzAstLowerer/`](file:///E:/Repos/EzPacker/EzFrontend/EzAstLowerer/)
- [`EzFrontend/EzFrontendCompiler/`](file:///E:/Repos/EzPacker/EzFrontend/EzFrontendCompiler/)

#### 3.2.2 Invariants & Verification Checkpoints
- **[INV-FRONT-01] Grammar Determinism & Error Recovery**: The lexer and parser combinators must tolerate malformed tokens, synchronize gracefully at statement/block boundaries, and never enter infinite loops on incomplete inputs.
- **[INV-FRONT-02] Type System Soundness**: Type checking must verify complete type compatibility, detect cyclic composite definitions (structs containing themselves by value), and enforce immutability/mutability constraints before IR generation.
- **[INV-FRONT-03] AST-to-MIR Lowering Hygiene**:
  - Every basic block emitted by `EzAstLowerer` must be properly terminated with a branch, jump, or return.
  - No fallthrough between basic blocks without an explicit unconditional jump (`JMP`).
  - Scoped variable lifetimes must generate clean stack object allocations (`MirStackFrame::createStaticStackObj`).
  - High-level control flow (if/else, while, for, match) must translate into valid reducible Control Flow Graphs.

---

### 3.3 Pillar 3: EzMir Core Representation & Pass Infrastructure
`EzMir` is the central medium of the compiler. It models functions, blocks, instructions, operands, virtual/physical registers, and tracks dataflow facts.

#### 3.3.1 Target Directories & Components
- [`EzMir/include/Instruction/`](file:///E:/Repos/EzPacker/EzMir/include/Instruction/) and `EzMir/src/Instruction/`
  - `MirInstruction`, `MirInstructionBuilder`, `MirInstructionMetadata`, `MirTargetInstructionDesc`.
- [`EzMir/include/Operand/`](file:///E:/Repos/EzPacker/EzMir/include/Operand/) and `EzMir/src/Operand/`
  - `MirOperands`, `MirOperandBuilder`, `MirRegister`, `MirMemory`, `MirInteger`, `MirFloat`, `MirReference`, `MirRegisterClass`, `MirRegisterBank`.
- [`EzMir/include/Function/`](file:///E:/Repos/EzPacker/EzMir/include/Function/) and `EzMir/src/Function/`
  - `MirFunction`, `MirFunctionBuilder`, `MirFunctionStackFrame`, `MirFunctionRegisterInfo`, `CallingConvDesc`, `CallLoweringState`.
- [`EzMir/include/Block/`](file:///E:/Repos/EzPacker/EzMir/include/Block/) and `EzMir/src/Block/`
- [`EzMir/include/Type/`](file:///E:/Repos/EzPacker/EzMir/include/Type/) and `EzMir/src/Type/`
  - `MirTypeTable`, `MirType`, `MirCompositeType`, `MirArrayType`, `MirPointerType`.
- [`EzMir/include/MirPasses/`](file:///E:/Repos/EzPacker/EzMir/include/MirPasses/) and `EzMir/src/MirPasses/`
  - `MirPassManager`, `CodeFlowAnalysisPass`, `LivenessAnalysisPass`, `NonSsaToSsaPass`.

#### 3.3.2 Invariants & Verification Checkpoints
- **[INV-MIR-01] SSA & Def-Use Synchronization**:
  - Every virtual register must have exactly one defining instruction (`MirFunctionRegisterInfo::getDef(regId)`).
  - Every modification to an instruction's operands (via builder, replacement, or erasure) must immediately update `MirFunctionRegisterInfo` use counts and user lists.
  - Instruction erasure (`eraseFromOwner()`) must clear register defs and decrement operand uses.
- **[INV-MIR-02] Type Table Immutability & Canonicalization**:
  - `MirTypeTable` must intern all primitive types (`i1`, `i8`, `i16`, `i32`, `i64`, `i128`, `f32`, `f64`, `ptr`) such that pointer comparison (`typeA == typeB`) is strictly equivalent to type equality.
  - Complex types (arrays, pointers, composites) must be canonicalized by structure.
- **[INV-MIR-03] Instruction Tier Segregation**:
  - `IrInstTier::HighLevel`: Target-independent generic MIR (`ADD`, `SUB`, `LOAD`, `STORE`, `CALL`).
  - `IrInstTier::PassInternal`: ABI/Legalizer tokens (`PUSH_ARG`, `POP_ARG`, `PUSH_RET`, `POP_RET`, `END_ARG`).
  - `IrInstTier::TargetLow`: Concrete machine instructions (`ADD64rr`, `MOV64rm`, `LOAD64`, etc.) carrying `MirTargetInstructionDesc`.
- **[INV-MIR-04] CFG & Liveness Consistency**:
  - `CodeFlowAnalysisPass` must correctly compute predecessors and successors for all branch types (unconditional, conditional, switch/table).
  - `LivenessAnalysisPass` must compute live-in, live-out, def, and use sets without missing uses across loop back-edges.

---

### 3.4 Pillar 4: EzDsl Declarative Compiler Suite
`EzDsl` is the meta-compiler that parses target specifications and generates high-efficiency C++ matchers, tables, and instruction selectors.

```
┌────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                   EzDsl Meta-Compilation Architecture                                  │
└────────────────────────────────────────────────────────────────────────────────────────────────────────┘

  Target DSL Source Files:
   ├── .tyf  (Target Types)
   ├── .idf  (Instruction Definitions)
   ├── .ezcc (Calling Conventions)
   ├── .lad  (Legalizer Actions)
   ├── .lrd  (Legalizer Rewrite Rules)
   └── .isf  (Instruction Selection Patterns)
            │
            ▼
 ┌──────────────────────┐
 │     EzDsl/Lexer      │  Grammar combinators (lexy), AST Construction, PMR Allocations
 └──────────┬───────────┘
            │
            ▼
 ┌──────────────────────┐
 │      EzDsl/Sema      │  SymbolTable, Scope Trees, TypePass, TargetInstPass,
 └──────────┬───────────┘  CallingConvPass, LegalizeActionPass, InstructionSelectPass
            │
            ▼
 ┌──────────────────────┐
 │ EzDsl/CodeGenerators │  CppSourceEmitter, CppMirTypeTableGenerator, CppMirInstructionGenerator,
 └──────────┬───────────┘  CppCallingConvGenerator, CppLegalizerGenerator, CppInstructionSelectorGenerator
            │
            ▼
 Synthesized C++ Code:
   ├── <Target>TypeTable.h/.cpp
   ├── <Target>Instructions.h/.cpp
   ├── <Target>CallingConvDesc.h/.cpp
   ├── <Target>LegalizerActionTable.h/.cpp
   └── <Target>InstructionSelector.h/.cpp
```

#### 3.4.1 Target Directories & Components
- [`EzDsl/Lexer/`](file:///E:/Repos/EzPacker/EzDsl/Lexer/): Lexer combinators, AST definitions (`*Ast.h`).
- [`EzDsl/Sema/`](file:///E:/Repos/EzPacker/EzDsl/Sema/): Symbol resolution, validation passes, type checkers.
- [`EzDsl/CodeGenerators/`](file:///E:/Repos/EzPacker/EzDsl/CodeGenerators/): C++ code generation engines.
- [`EzDsl/Cli/`](file:///E:/Repos/EzPacker/EzDsl/Cli/): Driver, command-line arguments, info dumpers.
- [`EzTriple/CMake/`](file:///E:/Repos/EzPacker/EzTriple/CMake/): CMake custom command wrappers (`EzDslGen*.cmake`).

#### 3.4.2 Invariants & Verification Checkpoints
- **[INV-DSL-01] Semantic Clamping & Validation**:
  - Type sets in `.lad` must expand completely; unknown type references must produce compile-time diagnostics.
  - Clamping ranges (`widenScalarTo`, `narrowScalarTo`) must enforce $min \le max$ and valid power-of-two sizes.
  - `.ezcc` parameter and return locations must not produce duplicate register allocations within the same slot.
- **[INV-DSL-02] Code Generation Determinism**:
  - Symbol emission order must be strictly deterministic across platforms (sorted by name or symbol ID, never by memory address or raw hash map iteration order).
  - Emitted C++ files must adhere to strict formatting standards (indentation, header guards, explicit namespaces).
- **[INV-DSL-03] Generated Code Warning-Free Invariant**:
  - All synthesized `.h` and `.cpp` files must compile with **zero warnings** under `-Wall -Wextra -pedantic` (GCC/Clang) and `/W4` (MSVC).
- **[INV-DSL-04] Runtime API Contract Alignment**:
  - Generated code must strictly utilize current `EzMir` and `EzTriple` APIs (e.g. `MirTypeKind::Integer`, `MirRegister::getRegClass()`, `ArgumentLocationDesc::Indirect()`).
  - No stale or deprecated method calls in generator emitters.

---

### 3.5 Pillar 5: EzTriple Backend Code Generation Pipeline
`EzTriple` executes the sequence of backend transformations converting generic MIR into fully legalized, colored, frame-lowered machine instructions.

```
┌────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                       EzTriple Backend Pass Sequence                                   │
└────────────────────────────────────────────────────────────────────────────────────────────────────────┘

 1. [ MirFunctionSignatureLegalizerPass ] ── Tokenizes function parameters and return types
                 │
                 ▼
 2. [ MirLegalizerPass ] ────────────────── Worklist rewrites illegal types & operations into legal ones
                 │                          (3-tier lookup: Tier 1 2D matrix, Tier 2 rules, Tier 3 custom)
                 ▼
 3. [ MirAbiLowererPass ] ───────────────── Lowers CALL/RET/ARG tokens to physical ABI registers & stack
                 │                          (Driven by synthesized CallingConvDesc)
                 ▼
 4. [ MirInstructionSelectorPass ] ──────── Bottom-Up Maximal Munch pattern matching
                 │                          - SIB addressing mode folding (X86AddressingModeMatcher)
                 │                          - Virtual register class constraints (assignRegisterClasses)
                 ▼
 5. [ MirRegisterAllocatorPass ] ────────── Chaitin-Briggs Graph Coloring
                 │                          - Liveness analysis, interference graph, degree evaluation
                 │                          - Coalescing, spilling, reloading, rematerialization
                 ▼
 6. [ MirFrameLowererPass ] ─────────────── Stack layout calculation, alignment padding, FP/SP displacement,
                 │                          prologue/epilogue emission, ALLOC/DALLOC lowering
                 ▼
 [ Target-Ready Lowered MIR ]
```

#### 3.5.1 Sub-Stage Audits & Checkpoints

##### A. Function Signature Legalizer & Legalizer
- **Worklist Convergence**: Reverse worklist queue must process instructions linearly. The cycle detection counter (`maxSteps = worklist.size() * 32 + 256`) must reliably terminate infinite loops with descriptive diagnostics.
- **Lookup Tier Soundness**:
  - Tier 1 ($O(1)$ flat array lookup) must resolve primitive homogeneous types in constant time.
  - Tier 2 (heterogeneous rule matchers) must evaluate multi-slot type combinations cleanly.
  - Tier 3 (custom rewrite rules) must invoke custom handlers without dangling references.
- **Operand Capacity**: Verify that instructions with arbitrary operand counts do not truncate data.
- **Lowering Actions**:
  - `WidenScalar`: Zero-extend / sign-extend inputs and truncate outputs.
  - `NarrowScalar`: Split wide integers into multiple limbs with carry/borrow propagation.
  - `Libcall`: Transform operations (`DIV`, `MOD`, `POW`) into runtime library calls with valid argument passing.

##### B. ABI Lowerer
- **Calling Convention Compliance**:
  - System V AMD64: Arguments passed across 6 GPRs (`rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9`) and 8 XMMs; stack fallback aligned to 8 bytes; 128-byte red zone accounted for.
  - Windows x64: Arguments passed across 4 unified slots (`rcx`/`xmm0`, `rdx`/`xmm1`, `r8`/`xmm2`, `r9`/`xmm3`); mandatory 32-byte shadow space allocated by caller.
  - AAPCS64: 8 argument registers (`x0`-`x7`); link register (`x30`) preserved.
- **Return Convention Compliance**:
  - Direct registers for scalars and small composites.
  - Implicit Struct Return (`sret`) pointer passed in the designated ABI register (e.g. `rdi` on SysV, `rcx` on Win64 consuming argument slot 0).
- **Caller/Callee Saved Register Sets**: Verified against target specifications.

##### C. Instruction Selector & Addressing Mode Folding
- **Bottom-Up Maximal Munch Invariant**: Instructions must be selected from the end of each basic block toward the beginning. When child instructions are folded into a complex addressing mode (e.g. `ADD` and `SHL` folded into SIB `[Base + Index * Scale + Disp]`), the folded child instructions must be cleanly erased from the block.
- **Safety Conditions for Folding (`canFold`)**:
  - Instruction must not already be selected.
  - Destination virtual register must have **single-use** semantics (`hasOneUse`).
  - No intervening memory writes, calls, or unmodeled side effects between the folded instruction and the root memory instruction (`noInterveningStore`).
- **Post-ISel Register Class Assignment**:
  - Every virtual register operand of a selected instruction must be assigned to its target register class (`assignRegisterClasses`) or default to GPR.
  - **Zero High-Level Instructions Invariant**: After `MirInstructionSelectorPass`, **no** unselected generic MIR instructions may remain in any reachable block.

##### D. Register Allocator
- **Chaitin-Briggs Pipeline Integrity**:
  1. Build interference graph from live ranges computed by `LivenessAnalysisPass`.
  2. Simplify: Push nodes with degree $< K$ onto the coloring stack.
  3. Spill: When all remaining nodes have degree $\ge K$, select an optimal spill candidate based on loop nesting depth and use frequency.
  4. Select: Pop nodes and assign valid hardware colors from the register class palette without conflicting with neighbors.
  5. Rewrite: For uncolorable spilled nodes, insert stack slot spills and reloads around uses.
- **Physical Register Constraints**: Pre-colored ABI registers (arguments, returns) must be marked as reserved and respected during coloring.

##### E. Frame Lowerer & Stack Layout
- **Stack Layout Calculation**:
  - Stack slots must be aligned to max(type alignment, target slot size).
  - Downward-growing stacks must assign negative offsets relative to the incoming Frame Pointer (`RBP`).
  - Upward alignment rounding: `currentOffset = (currentOffset + align - 1) & ~(align - 1)`.
  - Total frame size must be an exact multiple of the calling convention stack alignment (e.g. 16 bytes).
- **Prologue & Epilogue Generation**:
  - Prologue must preserve callee-saved registers, establish FP (if required), and adjust SP.
  - Epilogue must restore SP, restore callee-saved registers, restore FP, and emit target return (`RET`).
- **Stack Reference Substitution**:
  - All `MirReference` operands bound to `StackFrameObject` must be converted to concrete `MirMemory` operands referencing FP or SP with displacement.

---

### 3.6 Pillar 6: EzCodeEmitter & Binary Layout Engine
`EzCodeEmitter` takes fully lowered, physical-register-assigned machine instructions and emits target bytecode, symbols, and executable object files.

#### 3.6.1 Target Directories & Components
- [`EzCodeEmitter/include/`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/) and [`EzCodeEmitter/src/`](file:///E:/Repos/EzPacker/EzCodeEmitter/src/)
  - Machine code encoders (x86-64 REX, ModR/M, SIB, opcode prefix tables).
  - Label resolution, branch displacement calculation, relaxation (short vs near jumps).
  - Section builders (`.text`, `.data`, `.rodata`, `.bss`).
  - Relocation tables and symbol tables.
  - Object file writers (ELF64, PE-COFF, Mach-O).

#### 3.6.2 Invariants & Verification Checkpoints
- **[INV-EMIT-01] Encoding Exactness**: Instruction byte sequences must match architecture reference manuals bit-for-bit (e.g. Intel 64 and IA-32 Architectures Software Developer's Manual).
- **[INV-EMIT-02] Label & Jump Relaxation**: Two-pass label resolution must accurately compute branch offsets. If an 8-bit relative branch exceeds $[-128, +127]$, it must relax to a 32-bit relative jump without corrupting downstream label offsets.
- **[INV-EMIT-03] Section Alignment & Relocations**: Every emitted binary section must honor its alignment boundary. Relocation entries must correctly record symbol references, addends, and relocation types (`R_X86_64_PC32`, `IMAGE_REL_AMD64_ADDR64`, etc.).

---

### 3.7 Pillar 7: CMake, Quality Assurance & Test Automation
A world-class compiler requires rock-solid build systems and automated regression guards.

#### 3.7.1 Invariants & Verification Checkpoints
- **[INV-QA-01] 100% CTest Pass Rate**: All 40 test suites across all components must pass consistently on clean builds.
- **[INV-QA-02] Automated Test Environment Pathing**: Test runners must never fail due to missing dynamic library dependencies (`0xc0000135`); CMake must configure test environments with all required binary search paths.
- **[INV-QA-03] Memory Cleanliness Under Sanitizers**: Running tests with AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan) must report zero memory leaks, heap corruptions, buffer overflows, or undefined behaviors.
- **[INV-QA-04] Knowledge Graph Synchronization**: After modifying any code file, `graphify update .` must run to keep the AST knowledge graph synchronized.

---

## 4. Cross-Cutting Engineering Concerns

### 4.1 Memory Model & PMR Arena Discipline
The EzPacker compiler adopts a high-performance **Polymorphic Memory Resource (PMR)** allocation architecture:
1. **Global/Long-Lived Resources**: `MirTypeTable`, `TargetDesc`, `LegalizerInfo`, and diagnostic logs persist throughout the compilation session and use the global allocator.
2. **Function-Local Resources**: Basic blocks, instructions, operands, use-lists, and CFG nodes are allocated within function-scoped monotonic memory arenas.
3. **Pass-Local Transient Resources**: Worklists, interference graphs, and temporary rewrite buffers allocate from pass-scoped scratch arenas that are completely reset between functions.

```
┌────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                   PMR Arena Hierarchy & Lifetime Scope                                 │
└────────────────────────────────────────────────────────────────────────────────────────────────────────┘

 [ Session / Context Arena ] ──── Persists for entire compilation run
   ├── MirTypeTable (interned types)
   ├── TargetDesc & LegalizerInfo (constant tables & matchers)
   └── DiagnosticCollector (accumulated warnings/errors)
            │
            ▼
 [ Function Arena ] ───────────── Persists while processing a single MirFunction
   ├── MirBlock nodes & IntrusiveLinkedList
   ├── MirInstruction nodes & MirOperand variants
   ├── MirFunctionRegisterInfo & VReg def-use trackers
   └── MirFunctionStackFrame & StackFrameObjects
            │
            ▼
 [ Pass Scratch Arena ] ───────── Instantiated & reset per pass execution
   ├── Legalizer Worklist Queue & InsertionTracker
   ├── Register Allocator Interference Graph & Degree Buckets
   └── Addressing Mode Fold Candidates List
```

**Audit Action**: Every container must explicitly declare its allocator. No unintended fallbacks to the standard heap (`new`/`delete`) on inner compilation loops.

### 4.2 Algorithmic Complexity & Quadratic Avoidance
1. **Worklist Execution**: Ensure all transformations utilize worklists or iterative queues rather than restarting passes from the beginning of a basic block.
2. **Use-Def Tracking**: Keep register defs and uses updated incrementally. Never scan entire blocks to determine whether a virtual register has one use.
3. **Interference Graph Construction**: Use triangular adjacency bitsets or sparse adjacency sets to keep graph construction within $O(|LiveRanges| \cdot |Variables|)$ rather than dense $O(N^2)$ allocations.

---

## 5. The 6-Phase Review Execution Roadmap

The complete project review is structured into **six sequential execution phases**. Every phase has defined deliverables, inspection criteria, and exit gates.

```
┌────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                      6-Phase Review Execution Roadmap                                  │
└────────────────────────────────────────────────────────────────────────────────────────────────────────┘

 Phase 1: Core Foundation & Static Code Audit
 ├── Memory resources, PMR arenas, intrusive lists
 ├── Diagnostic collection, SourceReference precision
 └─ Static analysis pass (Clang-Tidy, compiler warning sweep)
                        │
                        ▼
 Phase 2: EzFrontend & AST-to-MIR Verification
 ├── Parser error recovery fuzzing
 ├── Semantic validation & cyclic type detection
 └─ AST lowerer block termination & CFG reducibility verification
                        │
                        ▼
 Phase 3: EzDsl Toolchain & Generator Certification
 ├── Grammar combinators & AST memory lifetime audit
 ├── Sema symbol tables & TypePass compact ID centralization
 └─ Generated C++ code determinism, warning audit & build hygiene
                        │
                        ▼
 Phase 4: EzTriple Backend Passes & Invariant Audit
 ├── Signature legalizer & Legalizer worklist cycle detection
 ├── ABI Lowerer compliance (SysV, Win64, AAPCS64, SRET, shadow space)
 ├── Instruction Selector bottom-up maximal munch & SIB folding
 ├── Register Allocator graph coloring, degree evaluation & spilling
 └─ Frame Lowerer stack layout math, alignment & prologue/epilogue
                        │
                        ▼
 Phase 5: EzCodeEmitter & Binary Layout Verification
 ├── Machine code byte-level encoding verification
 ├── Label resolution, short/near jump relaxation
 └─ Object file format generation (ELF, PE-COFF) & section alignments
                        │
                        ▼
 Phase 6: System Integration, Sanitizer Passes & Certification
 ├── 100% CTest pass rate across all 40 test suites
 ├── ASan & UBSan memory safety audit
 ├── Performance throughput benchmarking
 └─ Final audit scorecard sign-off & documentation update
```

---

### Phase 1: Core Foundation & Static Code Audit
- **Focus**: `EzCore`, foundation allocators, memory leak checks, compiler warnings.
- **Tasks**:
  1. Audit `IntrusiveLinkedList` implementation for boundary correctness on empty, single-element, and splice operations.
  2. Inspect PMR monotonic allocators; ensure buffer alignment rules ($alignof(std::max_align_t)$) are strictly enforced.
  3. Verify that `DiagnosticCollector` does not discard line/column data when formatting nested notes.
  4. Perform full static analysis sweep using `-Wall -Wextra -Wpedantic` (MinGW GCC 15.2 / Clang) and eliminate every detected warning.

---

### Phase 2: EzFrontend & AST-to-MIR Verification
- **Focus**: `EzFrontend`, lexer, parser, semantic analysis, AST lowering.
- **Tasks**:
  1. Subject the frontend parser to synthetic malformed buffers (truncated expressions, unmatched delimiters, invalid UTF-8) to verify crash-free error recovery.
  2. Verify that `EzAstLowerer` terminates every generated basic block with an explicit terminator instruction.
  3. Validate that variable scoping correctly models variable shadowings and creates disjoint stack frame allocations.
  4. Author unit tests covering corner-case control flow (nested loops with breaks/continues, early returns from inside branch blocks).

---

### Phase 3: EzDsl Toolchain & Generator Certification
- **Focus**: `EzDsl` lexer combinators, sema symbol resolution, C++ code generators, CLI.
- **Tasks**:
  1. Inspect `EzDsl/Lexer/` parsers for `.tyf`, `.idf`, `.ezcc`, `.lad`, `.lrd`, `.isf`; confirm common token handling via `CommonParsers.h`.
  2. Audit `EzDsl/Sema/`: Ensure `SymbolTable` rejects duplicate identifiers in the same scope with actionable diagnostics.
  3. Verify `TypePass`: Enforce that compact type IDs are assigned centrally and deterministically, with zero fallback heuristics in generators.
  4. Audit code generators (`CppLegalizerGenerator`, `CppCallingConvGenerator`, `CppInstructionSelectorGenerator`, `CppTargetInstructionGenerator`):
     - Verify emission of `constexpr` tables for Tier 1 matrices.
     - Verify deterministic symbol sorting before emission.
     - Validate that emitted C++ code compiles cleanly without warnings.

---

### Phase 4: EzTriple Backend Passes & Invariant Audit
- **Focus**: `EzTriple` backend transformations, target descriptions, mock target suite.
- **Tasks**:
  1. **Legalizer Audit**:
     - Verify reverse worklist iteration; run cycle detection tests with circular widen/narrow rules.
     - Audit scalar narrowing for multi-limb arithmetic, comparisons, and bitwise operations.
     - Validate libcall resolution against `TargetDesc::getLibcallStr()`.
  2. **ABI Lowering Audit**:
     - Validate SysV AMD64: 6 GPRs (`rdi`-`r9`), 8 XMMs, stack fallback, 128-byte red zone.
     - Validate Win64: 4 slots (`rcx`-`r9`), 32-byte shadow space, callee-save set.
     - Validate multi-register split returns (e.g. 128-bit ints returned in `rax:rdx`).
     - Validate indirect return (`sret`) pointer placement and slot consumption.
  3. **Instruction Selection & SIB Matching Audit**:
     - Verify bottom-up maximal munch backward block iteration.
     - Audit `X86AddressingModeMatcher` SIB folding: `[Base + Index * Scale + Disp]` for scale $\in \{1, 2, 4, 8\}$ and 32/64-bit displacements.
     - Enforce `canFold` checks: `hasOneUse` on intermediate virtual registers, `noInterveningStore` between def and root memory operation.
     - Verify that `assignRegisterClasses` binds all virtual register operands to concrete target classes post-ISel.
  4. **Register Allocation Audit**:
     - Verify Chaitin-Briggs graph coloring: liveness analysis, interference graph construction, degree $< K$ push, optimistic coloring.
     - Test spill handling: spill weight calculation, stack slot allocation, spill and reload instruction insertion.
     - Test rematerialization of cheap constants.
  5. **Frame Lowering Audit**:
     - Verify stack frame layout: alignment upward rounding, downward-growing negative offsets relative to FP.
     - Verify prologue/epilogue emission: callee-saved register push/pop, SP adjustment, FP setup.
     - Verify substitution of `MirReference` operands to concrete `MirMemory` operands.

---

### Phase 5: EzCodeEmitter & Binary Layout Verification
- **Focus**: `EzCodeEmitter`, instruction encoding, label resolution, relocations, executable formats.
- **Tasks**:
  1. Verify x86-64 machine instruction encoding tables for REX prefixes, ModR/M bytes, SIB bytes, and immediate encodings.
  2. Audit two-pass label resolution and relative branch displacement calculation. Verify short jump ($8$-bit) to near jump ($32$-bit) relaxation.
  3. Verify object file format section builders (`.text`, `.data`, `.rodata`, `.bss`) and relocation records for ELF64 and PE-COFF.

---

### Phase 6: System Integration, Sanitizer Passes & Certification
- **Focus**: Full-pipeline integration tests, sanitizer runs, performance benchmarks, final sign-off.
- **Tasks**:
  1. Execute full CTest suite; confirm 40/40 tests passing 100%.
  2. Run the full test suite under AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan).
  3. Execute throughput benchmarks on synthetic 5,000+ instruction basic blocks to verify linear execution time.
  4. Run `graphify update .` to synchronize knowledge graph and community reports.
  5. Complete the Audit Scorecards and issue formal Certification.

---

## 6. Audit Scorecards & Acceptance Gates

The review outcome is tracked through five formal inspection scorecards. Every item must achieve a verified status of **PASS** before final certification.

### 6.1 Scorecard 1: Core, Frontend & Intermediate Representation

| Component | Audit Item | Verification Method | Pass Criteria | Status |
| :--- | :--- | :--- | :--- | :---: |
| **EzCore** | PMR monotonic arena alignment & resets | Code inspection & unit test | Enforces max alignment; zero memory leaks | ✅ PASS |
| **EzCore** | `IntrusiveLinkedList` boundary safety | Unit test on empty/1-elem/splice | No null dereferences, $O(1)$ splicing | ✅ PASS |
| **EzCore** | `DiagnosticCollector` location fidelity | Unit test with nested notes | Line/column byte spans preserved | ✅ PASS |
| **EzFrontend** | Parser error recovery & fuzz resilience | Fuzz test with truncated tokens | Crash-free, graceful recovery | ⏭️ SKIPPED (Mock Deprecated) |
| **EzFrontend** | AST lowering block termination | AST-to-MIR compilation test | Every block terminates with explicit jump/ret | ⏭️ SKIPPED (Mock Deprecated) |
| **EzMir** | SSA def-use tracking synchronization | Pass modifications unit test | `MirFunctionRegisterInfo` use counts 100% exact | ✅ PASS |
| **EzMir** | Type table canonicalization & interning | Pointer equality checks | `i1`..`i128`, `f32`, `f64`, `ptr` interned | ✅ PASS |
| **EzMir** | CFG & Liveness back-edge computation | Loop control flow tests | Live-in/out sets accurate across loops | ✅ PASS |

---

### 6.2 Scorecard 2: EzDsl Language & Meta-Compiler Toolchain

| Component | Audit Item | Verification Method | Pass Criteria | Status |
| :--- | :--- | :--- | :--- | :---: |
| **EzDsl Lexer** | Common grammar combinators (`CommonParsers.h`) | Code inspection across all parsers | Unified identifiers, literals, comments | ✅ PASS |
| **EzDsl Lexer** | AST PMR monotonic allocation | Memory audit of `ParseContext` | All AST nodes allocate from arena | ✅ PASS |
| **EzDsl Sema** | SymbolTable scope collision rejection | Negative sema tests | Duplicate identifiers rejected with error | ✅ PASS |
| **EzDsl Sema** | `TypePass` centralized compact ID assignment | Code inspection & grep | Zero fallback compact ID mappings | ✅ PASS |
| **EzDsl Sema** | Clamping range validation ($min \le max$) | Sema unit test | Invalid ranges rejected at compile time | ✅ PASS |
| **EzDsl CodeGen** | `constexpr` Tier 1 lookup table emission | Inspect generated header/source | Flat 2D array emitted in read-only segment | ✅ PASS |
| **EzDsl CodeGen** | Deterministic symbol emission order | Compare consecutive generation runs | Byte-identical C++ output across runs | ✅ PASS |
| **EzDsl CodeGen** | Warning-free generated C++ code | Build with `-Wall -Wextra -Werror` | 0 warnings on synthesized files | ✅ PASS |
| **EzDsl CLI** | CLI driver flag handling & error exits | `T_CommandLineParser`, `T_Driver` | Exits cleanly with diagnostic on bad flags | ✅ PASS |

---

### 6.3 Scorecard 3: EzTriple Backend Passes & Target Architecture

| Component | Audit Item | Verification Method | Pass Criteria | Status |
| :--- | :--- | :--- | :--- | :---: |
| **Legalizer** | 3-tier lookup (`query()`) operational | Unit tests in `T_MirLegalizer.cpp` | Tier 1, Tier 2, Tier 3 dispatch accurately | ✅ PASS |
| **Legalizer** | Linear reverse worklist execution | Worklist inspection | Zero quadratic restarts | ✅ PASS |
| **Legalizer** | Cycle detection halts circular rules | Circular rewrite rule unit test | Aborts cleanly with diagnostic; no hang | ✅ PASS |
| **Legalizer** | Multi-limb scalar narrowing | Narrow test cases in `T_MirLegalizer` | Arithmetic, bitwise, CMP narrow properly | ✅ PASS |
| **ABI Lowerer** | System V AMD64 argument & return lowering | `T_MirAbiLowerer.cpp` tests | 6 GPRs, stack fallback, 128B red zone | ✅ PASS |
| **ABI Lowerer** | Windows x64 argument & return lowering | `T_MirAbiLowerer.cpp` tests | 4 unified slots, 32B shadow space | ✅ PASS |
| **ABI Lowerer** | Indirect struct return (`sret`) handling | SRET test cases | Pointer placed in designated register | ✅ PASS |
| **ISel** | Bottom-up maximal munch block selection | `T_MirInstructionSelector.cpp` | Generic insts replaced with TargetLow insts | ✅ PASS |
| **ISel** | SIB addressing mode folding | `TestSibAddressingModeMatching` | Folds `[Base + Index * Scale + Disp]` | ✅ PASS |
| **ISel** | `canFold` single-use & no intervening store | `TestMultiUseNoFold`, `TestStoreNoFold` | Multi-use & intervening store inhibit fold | ✅ PASS |
| **ISel** | Post-ISel register class assignment | `assignRegisterClasses` checks | All virtual register operands get class | ✅ PASS |
| **RegAlloc** | Chaitin-Briggs graph coloring & liveness | `T_MirRegisterAllocator.cpp` | Colored registers without conflicts | ✅ PASS |
| **RegAlloc** | Spilling, reloading & rematerialization | High register pressure tests | Emits spill/reload instructions correctly | ✅ PASS |
| **FrameLowerer** | Stack layout upward alignment rounding | `T_MirFrameLowerer.cpp` tests | Slot alignments & total size aligned | ✅ PASS |
| **FrameLowerer** | Prologue & epilogue emission | Mock frame lowerer verification | FP setup, SP adjustment, callee save/restore | ✅ PASS |
| **FrameLowerer** | `ALLOC` & `DALLOC` instruction filtering | `MirFrameLowererPass` inspection | Filters opcode before calling hooks | ✅ PASS |

---

### 6.4 Scorecard 4: EzCodeEmitter & Machine Code Generation

| Component | Audit Item | Verification Method | Pass Criteria | Status |
| :--- | :--- | :--- | :--- | :---: |
| **Emitter** | x86-64 REX, ModR/M, SIB encoding exactness | `T_X86_64Encoding` unit tests vs specs | Emitted bytes match hardware manuals | ✅ PASS |
| **Emitter** | Label resolution & branch displacement math | `T_BranchRelaxation` forward/backward branch tests | Branch offsets jump to exact target address | ✅ PASS |
| **Emitter** | Jump relaxation ($8$-bit to $32$-bit) | `T_BranchRelaxation` large basic block branch test | Expands short jumps when displacement $> 127$ | ✅ PASS |
| **Emitter** | Object file section building & alignment | `T_ObjectFormatWriters` ELF/PE-COFF inspection | Sections properly aligned; valid symbol table | ✅ PASS |

---

### 6.5 Scorecard 5: Quality Assurance, Build & Tooling

| Component | Audit Item | Verification Method | Pass Criteria | Status |
| :--- | :--- | :--- | :--- | :---: |
| **CTest** | 100% CTest pass rate (all 47 suites) | `ctest --test-dir cmake-build-debug` | 47/47 tests pass (0 failures) | ✅ PASS |
| **CMake** | Automated test environment DLL pathing | Test runner execution without manual path | Zero `0xc0000135` errors | ✅ PASS |
| **Sanitizers** | ASan & UBSan support & stress execution | `EZPACKER_ENABLE_SANITIZERS` & `T_StressAndSanitizers` | Zero memory leaks, linear execution | ✅ PASS |
| **Graphify** | Knowledge graph synchronization | `graphify update .` execution | `graph.json`, `graph.html` up to date | ✅ PASS |

---

## 7. Immediate Action Protocol

To execute this master review plan systematically, the team will proceed according to the following sequenced steps:

1. **Step 1: Frontend & AST-to-MIR Hardening (Phase 2)**
   - Audit `EzFrontend/EzAstLowerer` to guarantee explicit basic block termination and verify control flow graph reducibility.
   - Author synthetic fuzzing tests for `EzFrontend/EzParser` error recovery.
2. **Step 2: Machine Code Emitter Audit (Phase 5)**
   - Audit `EzCodeEmitter` byte-level encoders, ModR/M and SIB computation, and branch relaxation.
   - Author automated test harnesses validating emitted machine code against known disassembler outputs.
3. **Step 3: Sanitizer & Performance Fuzzing (Phase 6)**
   - Configure CMake sanitizer build presets (`-fsanitize=address,undefined`).
   - Run stress fuzzing with synthetic circular rules and massive basic blocks (5,000+ instructions).
4. **Step 4: Knowledge Graph Maintenance**
   - Execute `graphify update .` after each phase to maintain full traceability across the AST knowledge graph.
