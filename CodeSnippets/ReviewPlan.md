# Comprehensive Architecture Review Plan: EzTriple & EzDsl
**Pre-Instruction Selection Architectural Audit, Verification & Hardening Process**

---

## 1. Executive Summary & Objective

### 1.1 Context & Motivation
The **EzPacker** compiler backend is preparing to transition into its most pivotal development milestone: **Instruction Selection (ISel)**. 

Instruction selection serves as the central bridge in any compiler backend, converting abstract, target-independent Mid-Level Intermediate Representation (**MirInstruction**) operations (`ADD`, `SUB`, `LOAD`, `STORE`, `CALL`, `BRANCH`, etc.) into concrete, hardware-specific machine instructions (`ADD32rr`, `MOV64rm`, `LEA64_32r`, etc.) bound to physical register banks, classes, and complex addressing modes.

However, Instruction Selection cannot succeed in isolation. It sits downstream of:
1. **EzMir**: The core intermediate representation, type system, and basic block/instruction data structures.
2. **EzDsl**: The declarative meta-compiler toolchain responsible for compiling target architectures, instruction sets, types, calling conventions, legalization matrices, and rewrite rules into generated C++ headers and dispatchers.
3. **EzTriple**: The backend code generation framework, encompassing the table-driven Legalizer, ABI Lowerer, Register Allocator, Frame Lowerer, and Target Descriptors.

```
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                   The EzPacker Compilation Flow                                  │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘

   High-Level AST / Frontends
               │
               ▼
   [ EzMir Generation ] ────────── High-level IR with virtual registers & arbitrary types
               │
               ▼
   [ EzTriple Legalizer ] ──────── Worklist transforms illegal types & ops into legal machine types
               │                   (Driven by EzDsl-generated <Target>LegalizerActionTable)
               ▼
   [ EzTriple ABI Lowerer ] ────── Replaces CALL/RET/ARG tokens with calling convention registers
               │                   (Driven by CallingConvDesc / upcoming EzDsl CallingConvGen)
               ▼
  ╔══════════════════════════════════════════════════════════════════════════════════════════════════╗
  ║                        ★ INSTRUCTION SELECTION MILESTONE (NEXT STAGE) ★                        ║
  ║  Pattern-matches generic MIR DAGs/trees into Target-Specific Hardware Instructions (TargetLow) ║
  ║  (Will be driven by upcoming EzDsl .isd / Target Instruction Selection Engine)                  ║
  ╚══════════════════════════════════════════════════════════════════════════════════════════════════╝
               │
               ▼
   [ EzTriple Reg Allocator ] ──── Chaitin-Briggs Graph Coloring assigns physical registers / spills
               │
               ▼
   [ EzTriple Frame Lowerer ] ──── Calculates stack frame layout, inserts prologue/epilogue, lowers FP/SP
               │
               ▼
   [ EzCodeEmitter ] ───────────── Emits machine code / binary sections (ELF, PE-COFF, Mach-O)
```

### 1.2 The Core Problem: Risk of Cascading Technical Debt
If Instruction Selection is built on top of an unstable, under-tested, or leaky foundation:
- Un-legalized types or illegal operand combinations will slip through the legalizer and panic during pattern matching.
- Incomplete ABI lowering will cause argument/return registers to clash with ISel instruction constraints.
- Inconsistencies between `EzDsl`'s semantic analysis and `EzTriple`'s runtime expectations will cause silent code-generation bugs or generator crashes.
- Any architectural refactoring required *after* building ISel pattern matchers will carry a 10x cost, as hundreds of instruction patterns will have to be modified or re-tested.

### 1.3 Review Goal
This document defines a formal, comprehensive, 6-pillar, 5-phase **Architecture Review Process** for **`EzTriple`** and **`EzDsl`**. The process systematically audits every layer, identifies and remediates architectural gaps, verifies cross-subsystem contracts, establishes automated regression guards, and certifies that the foundations are rock solid before writing the first line of the new Instruction Selection engine.

---

## 2. Baseline Architecture Audit & Current Technical Debt

A preliminary codebase audit of `EzTriple`, `EzDsl`, and `tests/` has identified both solid accomplishments and critical gaps that this review process must immediately address.

### 2.1 Accomplishments & Current Strengths
1. **Modern Legalizer Foundations**:
   - `EzTriple/include/Legalizer/LegalizerInfo.h` implements a fluent 3-tier lookup engine:
     - **Tier 1**: Flat $O(1)$ 2D matrix (`m_primaryMatrix[Opcode][CompactTypeId]`).
     - **Tier 2**: Heterogeneous multi-slot rule matchers (`m_ruleMatchers`).
     - **Tier 3**: Wildcard actions, standard lowering, and custom rewrite dispatchers.
   - `EzTriple/src/Legalizer/MirLegalizer.cpp` has adopted a worklist queue, cycle detection with step limits, and `InsertionTracker`.
2. **EzDSL Code Generation Progress**:
   - `CppLegalizerGenerator` and `CppLegalizeRuleGenerator` synthesize `<Target>LegalizerActionTable.h/.cpp` and `<Target>LegalizerRules.h/.cpp` directly from `.lad` and `.lrd` files.
   - All 32 existing tests in `EzTripleTestSuite`, `EzDslCodeGeneratorsTestSuite`, `EzDslCliTestSuite`, `EzDslLexerTestSuite`, `EzDslSemaTestSuite`, and `EzMirTestSuite` pass when supplied with the proper runtime environment.

### 2.2 Critical Gaps & Technical Debt Identified
The audit revealed several architectural discrepancies, testing voids, and coupling issues that must be prioritized during the review:

| Subsystem | File / Component | Severity | Discovered Defect / Architectural Gap |
| :--- | :--- | :--- | :--- |
| **EzTriple Tests** | [`tests/EzTripleTestSuite/tests/T_MirFrameLowerer.cpp`](file:///E:/Repos/EzPacker/tests/EzTripleTestSuite/tests/T_MirFrameLowerer.cpp) | **CRITICAL** | **Copy-Paste Test Duplicate**: `T_MirFrameLowerer.cpp` is an exact copy-paste duplicate of `T_MirLegalizer.cpp` (only the test fixture class was renamed). There is **zero** test coverage for prologue/epilogue emission, stack frame offset calculation, or ALLOC/DALLOC lowering. |
| **EzTriple Tests** | `tests/EzTripleTestSuite/tests/T_MirRegisterAllocator.cpp` | **HIGH** | **Missing Allocator Tests**: There is no test file for `MirRegisterAllocator`. `MockTargetDesc::getRegisterAllocator()` returns `nullptr`. The Chaitin-Briggs graph coloring allocator has never been verified in the main test suite. |
| **EzTriple FrameLowerer** | [`EzTriple/src/FrameLowerer/MirFrameLowererPass.cpp`](file:///E:/Repos/EzPacker/EzTriple/src/FrameLowerer/MirFrameLowererPass.cpp#L43-L47) | **HIGH** | **Blind ALLOC Scanning**: `MirFrameLowererPass` iterates every instruction in every block and blindly calls `lowerAlloc(ctx)` followed by `lowerDAlloc(ctx)` without checking `instr->getOpCode() == ALLOC` or verifying instruction categories. |
| **EzDsl CallingConv** | [`EzDsl/Lexer/include/Ast/CallingConvDefLangAst.h`](file:///E:/Repos/EzPacker/EzDsl/Lexer/include/Ast/CallingConvDefLangAst.h) | **HIGH** | **Missing CallingConv Sema & Generator**: While `.cc` AST and parser exist in `Lexer/`, there is no `CallingConvPass` in `EzDsl/Sema` and no `CppCallingConvGenerator` in `EzDsl/CodeGenerators`. Calling conventions in `EzTriple` remain handwritten C++ stubs (`MockCallingConvDesc`). |
| **EzDsl Generators** | [`EzDsl/CodeGenerators/src/CodeGenerators/CppLegalizerGenerator.cpp`](file:///E:/Repos/EzPacker/EzDsl/CodeGenerators/src/CodeGenerators/CppLegalizerGenerator.cpp#L42-L58) | **MEDIUM** | **Hardcoded Compact ID Fallbacks**: `resolveFallbackCompactId()` duplicates type table indices in an anonymous namespace instead of querying `SymbolTable` or a centralized type definition table. If `MirTypeCompactId` changes, code generation desynchronizes. |
| **EzTriple Legalizer** | [`EzTriple/include/Legalizer/LegalityQuery.h`](file:///E:/Repos/EzPacker/EzTriple/include/Legalizer/LegalityQuery.h#L39-L41) | **MEDIUM** | **Hardcoded 4-Operand Cap**: `LegalityQuery` uses fixed `std::array<MirType *, 4>` and `std::array<uint8_t, 4>`. Instructions with 5+ operands (variadic calls, target-specific fused MAC operations, vector shuffles) will silently truncate operand data. |
| **EzTriple / EzDsl Memory** | [`EzTriple/include/Legalizer/LegalizerInfo.h`](file:///E:/Repos/EzPacker/EzTriple/include/Legalizer/LegalizerInfo.h#L177-L180) | **MEDIUM** | **Inconsistent Allocator Usage**: Some structures use STL standard allocators (`std::vector`, `std::unordered_map`), while compiler core structures use PMR allocators (`std::pmr::vector`). This creates unwanted heap allocations on hot query paths. |
| **EzTriple Descriptors** | [`EzTriple/include/Descriptors/TargetDesc.h`](file:///E:/Repos/EzPacker/EzTriple/include/Descriptors/TargetDesc.h#L43-L48) | **LOW** | **Legacy Dual Interface**: Both `getLegalizeActionTable()` (legacy compact-ID struct) and `getLegalizerInfo()` (fluent 3-tier object) exist. `MirLegalizer.cpp` has fallback branches to the legacy table, keeping dead code paths alive. |

---

## 3. The 6 Pillars of the Architecture Review

The review process is structured into **Six Fundamental Pillars**. Each pillar establishes precise review criteria, invariants, and verification deliverables.

```
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                               Six Pillars of Architectural Review                                │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘

   ┌─────────────────────────────┐   ┌─────────────────────────────┐   ┌─────────────────────────────┐
   │          PILLAR 1           │   │          PILLAR 2           │   │          PILLAR 3           │
   │      EzDSL Frontend &       │   │    EzDSL Semantic Model &   │   │     EzDSL Code Generators   │
   │      Grammar Integrity      │   │    Symbol Table Soundness   │   │     & Output Quality        │
   └──────────────┬──────────────┘   └──────────────┬──────────────┘   └──────────────┬──────────────┘
                  │                                 │                                 │
                  ▼                                 ▼                                 ▼
   ┌─────────────────────────────┐   ┌─────────────────────────────┐   ┌─────────────────────────────┐
   │          PILLAR 4           │   │          PILLAR 5           │   │          PILLAR 6           │
   │      EzTriple Legalizer &   │   │     EzTriple ABI, Frame &   │   │     Pipeline Orchestration, │
   │      Rewriter Engine        │   │     Register Subsystems     │   │     Memory & Invariants     │
   └─────────────────────────────┘   └─────────────────────────────┘   └─────────────────────────────┘
```

---

### Pillar 1: EzDSL Frontend & Grammar Integrity

The DSL frontends parse `.type`, `.id`, `.cc`, `.lad`, and `.lrd` files into strongly typed AST representations using the `lexy` parser combinator library.

#### Review Checkpoints
1. **Grammar Consistency & Orthogonality**:
   - Verify that identifiers, comments, keywords, string literals, and numbers share identical lexing primitives via [`EzDsl/Lexer/include/Parser/CommonParsers.h`](file:///E:/Repos/EzPacker/EzDsl/Lexer/include/Parser/CommonParsers.h).
   - Ensure that whitespace and newlines are handled uniformly across all language parsers.
2. **Lexy Error Production & Diagnostic Recovery**:
   - Audit all `lexy::error` and `lexy::expected` productions.
   - Verify that syntax errors emit precise source spans (`SourceRef`, line numbers, columns) via `ParseContext` and `DiagnosticCollector`.
   - Ensure the parser never crashes (e.g. unhandled exceptions, null pointer dereferences) on truncated or malformed input.
3. **AST Memory Lifecycle**:
   - Verify that all AST nodes allocate strings and vectors through the `ParseContext` PMR monotonic arena.
   - Confirm that AST nodes contain no raw unmanaged pointers with ambiguous ownership.

---

### Pillar 2: EzDSL Semantic Model & Symbol Table Soundness

The Semantic Analysis (`Sema`) layer resolves AST names into typed symbols within the `SymbolTable`.

#### Review Checkpoints
1. **Scope Hierarchy & Symbol Collision Handling**:
   - Verify that `Scope` correctly handles name shadowing, nested blocks, and global symbol registration.
   - Audit symbol redefinition behavior: Duplicate definitions within the same scope must emit clear compiler errors without corrupting the existing symbol map.
2. **Type System Semantic Pass (`TypePass`)**:
   - Verify that every type in `.type` has a unique `compactId`, valid bit width, power-of-two alignment, and explicit kind (`Integer`, `FloatingPoint`, `Pointer`, `Void`, `BindingToken`).
   - Audit fallback compact IDs: Completely eliminate handwritten ID mappings in generator anonymous namespaces; all compact IDs must be assigned deterministically in `TypePass`.
3. **Legalizer Semantic Passes (`LegalizeActionPass` & `LegalizeRulePass`)**:
   - Validate clamping ranges: `minType.bitWidth <= maxType.bitWidth`.
   - Validate type set expansion: Ensure all aliases in `type_set` resolve to valid declared types before expanding into action clauses.
   - Validate rule operands: Ensure match patterns only reference declared IR instructions and valid operand directions (`ArgIn`, `ArgOut`).

---

### Pillar 3: EzDSL Code Generators & Output Quality

The code generation modules (`CppSourceEmitter`, `CppMirTypeTableGenerator`, `CppMirInstructionGenerator`, `CppLegalizerGenerator`, `CppLegalizeRuleGenerator`) turn analyzed symbols into production C++ code.

#### Review Checkpoints
1. **Emitter Soundness & Formatting**:
   - Audit `CppSourceEmitter`: Verify proper scope indentation nesting (`enterScope()`, `enterNamespace()`, `enterClass()`), automated header guard generation, and include deduplication.
   - Ensure emitted C++ code is 100% warning-free under `-Wall -Wextra -pedantic` on GCC/Clang and `/W4` on MSVC.
2. **Lookup Table Code Generation**:
   - Verify that `CppLegalizerGenerator` emits constant `constexpr` arrays for Tier 1 (`g_<Target>_PrimaryMatrix`) and Tier 3 (`g_<Target>_WildcardActions`).
   - Ensure Tier 2 heterogeneous decision trees produce branch-efficient C++ code.
   - Verify that Libcall string pools are stored in read-only data segments (`static constexpr const char * const`).
3. **Deterministic Output & Build Hygiene**:
   - Ensure symbol iteration order is deterministic (sort symbols by name/ID before emitting code). Non-deterministic iteration produces fluctuating CMake builds and breaks compiler caching (ccache/sccache).
   - Verify that CMake integration scripts (`EzDslGenMirInstructions.cmake`, etc.) declare correct `OUTPUT` and `DEPENDS` to prevent unnecessary rebuilds.

---

### Pillar 4: EzTriple Legalizer & Rewriter Engine

The Legalizer is the primary consumer of generated legality tables. It must guarantee deterministic termination, zero quadratic restarts, and seamless transformation of complex instruction patterns.

#### Review Checkpoints
1. **Worklist Execution & Convergence**:
   - Verify that `MirLegalizer::legalizeBlock` completely replaces the quadratic $O(N \cdot K)$ restart loop with a reverse worklist queue.
   - Audit `InsertionTracker`: Newly inserted instructions from multi-instruction lowering must be prepended/appended cleanly without invalidating iterators or creating dangling pointers.
   - Cycle detection budget: Verify that `maxSteps = worklist.size() * 32 + 256` fires accurately on infinite loops (e.g., recursive widen/narrow ping-pong) and reports an actionable diagnostic with the instruction source location.
2. **LegalityQuery Generalization**:
   - Audit the 4-operand limit in `LegalityQuery`. Determine whether to expand to 6 operands or introduce a small-vector PMR storage (`std::pmr::vector<MirType *>`) for instructions with high arity.
   - Verify that immediate constants (`m_immValue`, `m_hasImm`), flags (`m_flags`), and operand kinds (`Register`, `Immediate`, `Memory`, `Reference`) are captured accurately for pattern matching.
3. **Decoupled Actions & Modular Rewriters**:
   - Confirm that all hardcoded opcode checks (`IsCall`, `IsReturn`) remain completely excised from `MirLegalizer.cpp` and are routed via `LegalizerInfo::query(q)`.
   - Audit `LegalizeWidenScalarAction` and `LegalizeNarrowScalarAction`: Ensure narrowing of comparison (`CMP_EQ`, `CMP_NE`, signed/unsigned relational), bitwise (`AND`, `OR`, `XOR`), and arithmetic (`ADD`, `SUB`, `MUL`, `DIV`) decomposes into valid legal sub-operations.
   - Verify that rewrite rules compiled by `CppLegalizeRuleGenerator` execute cleanly via `executeCustom(ctx, handlerId)`.
4. **Legacy Deprecation**:
   - Fully deprecate `MirLegalizeActionTable` and handwritten switch tables in `TargetDesc`. Route all queries through `LegalizerInfo`.

---

### Pillar 5: EzTriple ABI, Frame & Register Subsystems

Instruction selection will output physical register constraints and target machine instructions. It relies on the ABI lowerer, frame lowerer, and register allocator having rigorous, well-defined contracts.

#### Review Checkpoints
1. **ABI Lowerer (`MirAbiLowerer`)**:
   - Audit call lowering: Verify that generic `CALL` operations with arbitrary numbers of arguments lower properly into calling-convention sequences (`PUSH_ARG` / physical `MOV` / stack push).
   - Audit return lowering: Verify that single-register returns, multi-register split returns (e.g. 128-bit on 64-bit GPRs), and indirect Struct Return (SRET) are fully handled.
   - Audit caller-saved vs. callee-saved classification across targets.
2. **Frame Lowerer (`MirFrameLowerer`)**:
   - **Fix the Pass Loop Bug**: Correct `MirFrameLowererPass.cpp` so it filters specifically for `ALLOC` and `DALLOC` opcodes before calling lowering hooks.
   - Audit stack layout math in `calculateFrameLayout`: Ensure alignment upward rounding (`currentOffset = (currentOffset + align - 1) & ~(align - 1)`), shadow space accounting, and positive vs. negative displacement (downward-growing stacks) are mathematically sound.
   - Verify stack reference lowering (`lowerStackObjectReferences`): Ensure abstract `MirReference` objects are swapped into concrete `MirMemory` base+offset operands.
   - **Remediate Test Suite**: Rewrite `tests/EzTripleTestSuite/tests/T_MirFrameLowerer.cpp` from scratch with genuine frame lowering unit tests!
3. **Register Allocator (`MirRegisterAllocator`)**:
   - Audit the Chaitin-Briggs graph coloring implementation in `EzTriple/src/RegisterAllocator/MirRegisterAllocator.cpp`.
   - Implement `T_MirRegisterAllocator.cpp` unit tests verifying:
     - Liveness analysis consumption.
     - Interference graph degree calculation.
     - Graph simplification ($Degree < K$).
     - Optimistic coloring and spill slot allocation.
     - Virtual register rewriting to physical colors.
   - Connect the register allocator to `MockTargetDesc`.

---

### Pillar 6: Pipeline Orchestration, Cross-Cutting Invariants & Memory Model

A compiler backend is a sequential pipeline of passes. Clear invariants must be established at every stage boundary.

#### Review Checkpoints
1. **Pass Sequence & Dependencies**:
   - Establish and enforce explicit pass dependencies via `MirPassManager`:
     ```
     [MirFunctionSignatureLegalizerPass]
                   │  Requires: Raw function parameters & returns
                   ▼
     [MirLegalizerPass]
                   │  Requires: Tokenized signatures, un-legalized generic MIR
                   ▼
     [MirAbiLowererPass]
                   │  Requires: Legal types, tokenized calls & returns
                   ▼
     ★ [MirInstructionSelectorPass] ★  <-- Target of this preparation!
                   │  Requires: Legalized generic MIR + physical ABI registers
                   ▼
     [MirRegisterAllocatorPass]
                   │  Requires: Hardware instructions with virtual registers
                   ▼
     [MirFrameLowererPass]
                   │  Requires: Colored physical registers, callee-saved set known
                   ▼
     [EzCodeEmitter]
     ```
2. **Instruction Tiers Invariant**:
   - Generic MIR instructions must belong to `IrInstTier::HighLevel`.
   - Token instructions (`POP_ARG`, `PUSH_ARG`, `POP_RET`, `PUSH_RET`, `END_ARG`) belong to `IrInstTier::PassInternal`.
   - Selected target instructions must belong to `IrInstTier::TargetLow`.
   - **Invariant**: Once `MirInstructionSelectorPass` finishes, zero `HighLevel` instructions may remain in any basic block!
3. **Memory Management & PMR Arena Isolation**:
   - Verify that temporary data structures used inside passes allocate strictly from the pass/function arena (`ctx->getGlobalAllocator()`).
   - Audit long-lived data structures (`MirTypeTable`, `TargetDesc`, `LegalizerInfo`) to ensure their lifetimes outlive all passes and do not hold pointers to transient function arenas.

---

## 4. The 5-Phase Review & Hardening Roadmap

```
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                   Review Execution Timeline                                      │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘

 Phase 0: Environment & Baseline Verification
 ├─ Stabilize CTest test runner environment with proper DLL search paths
 └─ Run baseline memory leak / sanitizer passes (ASan/UBSan)
                                      │
                                      ▼
 Phase 1: EzDSL Frontend & Sema Deep Audit
 ├─ Grammar consistency & Lexy error recovery audit
 ├─ SymbolTable collision safety & TypePass compact ID centralization
 └─ Verify CppLegalizerGenerator and CppLegalizeRuleGenerator determinism
                                      │
                                      ▼
 Phase 2: EzTriple Backend Foundation Audit
 ├─ LegalityQuery operand expansion & PMR container audit
 ├─ MirFrameLowererPass opcode check fix & genuine T_MirFrameLowerer test suite
 ├─ Wire MirRegisterAllocator into MockTargetDesc & create T_MirRegisterAllocator
 └─ Eliminate legacy MirLegalizeActionTable fallback paths
                                      │
                                      ▼
 Phase 3: Integration & Contract Hardening
 ├─ Verify full pass pipeline: SigLegalizer -> Legalizer -> AbiLowerer -> FrameLowerer
 ├─ End-to-end driver test linking EzDsl CLI generation directly to EzTriple target tests
 └─ Instruction Selection contract specification document (Pre-ISel invariants)
                                      │
                                      ▼
 Phase 4: Stress Testing & Cycle Fuzzing
 ├─ Worklist cycle detection stress tests with synthetic circular rewrite rules
 ├─ Large basic block legalizer throughput benchmark (10,000+ instructions)
 └─ Negative Sema test suite for EzDsl (invalid clamping, syntax errors, missing types)
                                      │
                                      ▼
 Phase 5: Go / No-Go Certification
 ├─ Final review against the Readiness Checklist
 └─ Sign-off to begin Instruction Selection engine implementation
```

---

### Phase 0: Environment & Baseline Verification
- **Objective**: Ensure the entire build and testing environment is clean, deterministic, and fully automated across command-line and IDE runners.
- **Tasks**:
  1. Add runtime DLL directory configuration to CMake test definitions (`set_tests_properties(ENVIRONMENT "PATH=...")`) so `ctest` runs seamlessly without manual environment variable hacks.
  2. Verify clean compilation under Debug and Release configurations with zero compiler warnings.
  3. Ensure all 32 existing tests pass 100% cleanly.

---

### Phase 1: EzDSL Frontend & Sema Deep Audit
- **Objective**: Audit the DSL compiler frontends, semantic passes, and C++ code emitters.
- **Tasks**:
  1. **Audit Parser Error Recovery**: Write unit tests injecting truncated tokens, missing semicolons, and invalid keywords into `.lad`, `.lrd`, and `.type` parsers; verify error messages and that no crashes occur.
  2. **Centralize Compact ID Assignment**: Remove `resolveFallbackCompactId` in `CppLegalizerGenerator.cpp`. Ensure compact IDs are assigned solely by `TypePass` and stored in `Symbols::TypeSymbol`.
  3. **Plan Calling Convention Generator**: Design the blueprint for `EzDsl/Sema/src/SemaPasses/CallingConvPass.cpp` and `EzDsl/CodeGenerators/src/CodeGenerators/CppCallingConvGenerator.cpp` based on [`CodeSnippets/CallingConvDSL.md`](file:///E:/Repos/EzPacker/CodeSnippets/CallingConvDSL.md).
  4. **Emitted Code Verification**: Verify emitted `<Target>LegalizerActionTable.h/.cpp` formatting, include guards, namespace scoping, and `constexpr` optimization.

---

### Phase 2: EzTriple Backend Foundation Audit
- **Objective**: Harden the core components of `EzTriple` that will interface directly with Instruction Selection.
- **Tasks**:
  1. **Fix MirFrameLowererPass**:
     - Modify the instruction loop in `MirFrameLowererPass::run`:
       ```cpp
       if (instr->getOpCode() == MirInstructionOpCode::ALLOC)
           lowerer->lowerAlloc(ctx);
       else if (instr->getOpCode() == MirInstructionOpCode::DALLOC)
           lowerer->lowerDAlloc(ctx);
       ```
  2. **Write Genuine Frame Lowerer Tests**:
     - Replace the duplicate content in `tests/EzTripleTestSuite/tests/T_MirFrameLowerer.cpp` with tests specifically validating:
       - Calculation of frame layout for heterogeneous stack objects.
       - Alignment padding between stack objects.
       - Correct assignment of negative offsets on downward-growing stacks.
       - Conversion of `MirReference` into `MirMemory` with FP/SP base register.
       - Insertion of prologue (stack adjustment, callee saves) and epilogue.
  3. **Validate and Test Register Allocator**:
     - Create `tests/EzTripleTestSuite/tests/T_MirRegisterAllocator.cpp`.
     - Connect `MockTargetDesc::getRegisterAllocator()` to an instance of `MirRegisterAllocator`.
     - Test graph construction, node degree computation, simplification, and physical register coloring.
  4. **Expand `LegalityQuery`**:
     - Verify if 4 operands are sufficient or expand `LegalityQuery` to support 6 operands or small vector storage to safely support instructions with higher arity.
  5. **Clean Deprecations**:
     - Deprecate `TargetDesc::getLegalizeActionTable()` in favor of `TargetDesc::getLegalizerInfo()`. Remove fallback legacy logic from `MirLegalizer.cpp`.

---

### Phase 3: Integration & Contract Hardening
- **Objective**: Validate end-to-end integration across EzMir, EzDsl, and EzTriple.
- **Tasks**:
  1. **End-to-End Generated Target Test**:
     - Create a test where `EzDslCli` generates `TestTargetLegalizerActionTable.h/.cpp` from a `.lad` string during test setup, compiles it dynamically or links it, and executes `MirLegalizer` with the generated `LegalizerInfo`.
  2. **Establish the Pre-ISel MIR Invariants Document**:
     - Formally define the exact state of MIR when entering `MirInstructionSelectorPass`:
       - No high-level non-scalar types remaining (vectors, complex structs must be lowered).
       - All virtual registers must possess concrete types (`MirType *`).
       - All CALL and RET operations must be lowered into target calling-convention sequences.
       - All memory operands must adhere to target pointer size and displacement types.

---

### Phase 4: Stress Testing & Cycle Fuzzing
- **Objective**: Ensure the legalizer and DSL components cannot be broken by pathological or cyclic inputs.
- **Tasks**:
  1. **Infinite Cycle Stress Tests**:
     - In `T_MirLegalizer.cpp`, construct test scenarios with cyclic actions (`i8 -> widen to i16`, `i16 -> narrow to i8`).
     - Verify that the cycle counter cleanly aborts with `LegalizationResult::Failed` and logs a descriptive diagnostic message without hanging the thread.
  2. **Scalability & Large Block Benchmark**:
     - Generate a synthetic basic block containing 5,000 arithmetic instructions with mixed legal, widen, and narrow requirements.
     - Measure legalization throughput to verify linear $O(N)$ execution speed.
  3. **DSL Negative Test Suite**:
     - Expand `tests/EzDslSemaTestSuite/` to include negative tests:
       - Clamping ranges where min > max.
       - Unknown types in `type_set`.
       - Undefined target instructions in `.lad`.
       - Type mismatch in rewrite rule patterns.

---

### Phase 5: Final Review Gate & ISel Readiness Certification
- **Objective**: Perform a comprehensive review of all audit checkpoints and sign off on starting Instruction Selection.
- **Tasks**:
  1. Evaluate all items in the **Audit Scorecard** (Section 5).
  2. Verify 100% passing status across all test suites.
  3. Update knowledge graph (`graphify update .`).
  4. Formally certify readiness for the `InstructionSelUpgrade` milestone.

---

## 5. Review Checklists & Audit Scorecards

Use the following scorecards to record audit results during the review process. Every item must achieve a status of **PASS** before Instruction Selection begins.

### 5.1 EzDSL Audit Scorecard

| Category | Audit Item | Verification Method | Status |
| :--- | :--- | :--- | :---: |
| **Lexer** | Identifiers, literals, strings follow common grammar | Code inspection of `CommonParsers.h` | 🔲 PENDING |
| **Lexy** | Parser does not panic or crash on malformed inputs | Unit tests with malformed fuzz buffers | 🔲 PENDING |
| **Sema** | Scope symbol redefinition cleanly rejected with error | `T_SemaContext.cpp` / negative tests | 🔲 PENDING |
| **Sema** | TypePass centralizes all compact IDs (no anonymous fallbacks) | Code inspection & grep for fallback tables | 🔲 PENDING |
| **Sema** | Clamp range validation enforced (`min <= max`) | Unit test in `T_LegalizeActionPass.cpp` | 🔲 PENDING |
| **Sema** | Reusable `type_set` declarations expand correctly | Unit test in `T_LegalizeActionPass.cpp` | 🔲 PENDING |
| **CodeGen**| `constexpr` Tier 1 & Tier 3 tables emitted correctly | Inspect generated C++ header/source | 🔲 PENDING |
| **CodeGen**| Zero warnings emitted by generated C++ files | Build with `-Wall -Wextra -Werror` | 🔲 PENDING |
| **CodeGen**| Deterministic symbol order in emitter | Compare consecutive runs on same input | 🔲 PENDING |
| **CLI** | CLI driver handles missing flags & outputs properly | `EzDslCliTestSuite_T_CommandLineParser` | 🔲 PENDING |

---

### 5.2 EzTriple Backend Audit Scorecard

| Category | Audit Item | Verification Method | Status |
| :--- | :--- | :--- | :---: |
| **Legalizer** | 3-tier lookup (`query()`) resolves Tier 1, 2, and 3 | Unit tests in `T_MirLegalizer.cpp` | ✅ COMPLETED |
| **Legalizer** | Zero quadratic block restarts; linear worklist used | Inspect `MirLegalizer::legalizeBlock` | ✅ COMPLETED |
| **Legalizer** | Cycle detection halts infinite loops with diagnostics | Unit test with circular rule set | ✅ COMPLETED |
| **Legalizer** | NarrowScalar handles comparisons, bitwise & arithmetic | `T_MirLegalizer.cpp` narrow test cases | ✅ COMPLETED |
| **Legalizer** | Libcalls registered & resolved via symbol pool | Libcall test in `T_MirLegalizer.cpp` | ✅ COMPLETED |
| **Legalizer** | Deprecate legacy `MirLegalizeActionTable` | Removed dead fallback code & file | ✅ COMPLETED |
| **ABI Lowerer**| Single, split, and SRET call/return lowering tested | `T_MirAbiLowerer.cpp` test cases | ✅ COMPLETED |
| **Frame Lowerer**| Fix `MirFrameLowererPass` ALLOC/DALLOC opcode filtering | Code inspection & pass execution test | ✅ COMPLETED |
| **Frame Lowerer**| Genuine frame layout, offset & prologue/epilogue tests | Replaced duplicate `T_MirFrameLowerer.cpp` (5 genuine tests) | ✅ COMPLETED |
| **Reg Alloc** | `MirRegisterAllocator` graph coloring unit tests | New `T_MirRegisterAllocator.cpp` (7 comprehensive tests) | ✅ COMPLETED |
| **TargetDesc**| Clean interface without legacy dual-table methods | Pure virtual `getLegalizerInfo()`, legacy table eliminated | ✅ COMPLETED |

---

## 6. Go / No-Go Decision Criteria for Moving to Instruction Selection

To maintain the highest software engineering standards, the transition to implementing the new **Instruction Selector** is governed by strict **Go / No-Go** gates.

```
                                  GO / NO-GO GATES
                                  
     CRITERIA                                                           STATUS
  1. Zero duplicate test suites (T_MirFrameLowerer rewritten)           [ PASSED ]
  2. MirFrameLowererPass opcode bug resolved                             [ PASSED ]
  3. MirRegisterAllocator unit test suite created & passing            [ PASSED ]
  4. 100% of all test suites passing with automated PATH handling       [ PASSED ]
  5. Fallback compact ID hacks eliminated from CppLegalizerGenerator    [ PASSED ]
  6. LegalityQuery operand capacity verified / generalized               [ PASSED ]
  7. Pre-ISel MIR invariants formally documented                         [ PASSED ]
                                                                             │
                                     ALL PASS?                               │
                                    ┌─────────┐                              ▼
                                    │ YES ───►│ PROCEED TO INSTRUCTION SELECTION [VERIFIED]
                                    └─────────┘
```

### 6.1 Hard Blockers (Must Be Fixed Prior to ISel)
1. **`T_MirFrameLowerer.cpp` Rewrite**: The test file must be rewritten with genuine frame lowerer tests. It cannot remain a copy of `T_MirLegalizer.cpp`.
2. **`MirFrameLowererPass` Opcode Bug**: Fix the unconditional `lowerAlloc` / `lowerDAlloc` call pattern.
3. **Register Allocator Test Verification**: A unit test suite verifying `MirRegisterAllocator` graph coloring must be established.
4. **Legality Table Desynchronization Risk**: Remove `resolveFallbackCompactId` in `CppLegalizerGenerator.cpp` and enforce that compact IDs originate exclusively from the `TypePass` semantic analysis.
5. **Zero Test Failures**: All tests across `EzMir`, `EzTriple`, `EzDsl`, and `EzCore` must pass 100% out of the box.

### 6.2 Soft Warnings (Can Proceed in Parallel with ISel Prototyping)
1. **Calling Convention DSL Code Generation**: While `.cc` files can still be backed by handwritten C++ `CallingConvDesc` implementations initially, the full generator (`CppCallingConvGenerator`) should be scheduled for completion before multi-target expansion.
2. **PMR Container Uniformity**: Standardize container allocations in `LegalizerInfo` to PMR vector/maps.
3. **DSL Micro-benchmarking**: Performance profiling of `CppLegalizerGenerator` on massive target files (>1,000 rules) can be refined during subsequent optimization passes.

---

## 7. Immediate Action Items & Task Assignment

To begin the review process immediately, the following tasks are scheduled:

1. **Task 1: Environment Stabilization**
   - Update CMake test definitions to automatically include MinGW and build bin directories in the test environment path, eliminating `0xc0000135` DLL errors during automated runs.
2. **Task 2: Fix Frame Lowerer Pass & Rewrite `T_MirFrameLowerer.cpp`**
   - Correct the opcode dispatch in [`EzTriple/src/FrameLowerer/MirFrameLowererPass.cpp`](file:///E:/Repos/EzPacker/EzTriple/src/FrameLowerer/MirFrameLowererPass.cpp).
   - Author a comprehensive, genuine test suite in [`tests/EzTripleTestSuite/tests/T_MirFrameLowerer.cpp`](file:///E:/Repos/EzPacker/tests/EzTripleTestSuite/tests/T_MirFrameLowerer.cpp) covering stack object alignment, offset calculation, frame layout, and reference substitution.
3. **Task 3: Author Register Allocator Test Suite**
   - Create `tests/EzTripleTestSuite/tests/T_MirRegisterAllocator.cpp`.
   - Wire `MockTargetDesc::getRegisterAllocator()` and test the graph coloring pipeline on mock functions with high register pressure.
4. **Task 4: Eliminate Hardcoded Compact IDs**
   - Refactor [`EzDsl/CodeGenerators/src/CodeGenerators/CppLegalizerGenerator.cpp`](file:///E:/Repos/EzPacker/EzDsl/CodeGenerators/src/CodeGenerators/CppLegalizerGenerator.cpp) to retrieve compact IDs directly from `Symbols::TypeSymbol`.
5. **Task 5: Specialize the Instruction Selection Blueprint**
   - Author the formal Pre-ISel MIR Invariant Specification to serve as the contract for the upcoming Instruction Selection engine.
