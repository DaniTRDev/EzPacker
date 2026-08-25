# EzPacker "BackSpring" Architectural Optimization & Remediation Plan

> **Master Engineering Specification for Performance Optimization, Memory Safety, Modern C++ Refactoring, and Test Hardening across `EzCore`, `EzDsl`, `EzMir`, `tests/EzMirTestSuite`, and `tests/EzDslTestSuite`.**

---

## Table of Contents

1. [Executive Summary & Cross-Cutting Architectural Themes](#1-executive-summary--cross-cutting-architectural-themes)
2. [Subsystem Review: EzCore](#2-subsystem-review-ezcore)
   - [2.1 Core Utilities & Header Hygiene](#21-core-utilities--header-hygiene)
   - [2.2 FlexNumber Subsystem (`FlexInt`, `FlexFloat`)](#22-flexnumber-subsystem-flexint-flexfloat)
   - [2.3 Diagnostics Pipeline](#23-diagnostics-pipeline)
   - [2.4 SourceManager Subsystem](#24-sourcemanager-subsystem)
3. [Subsystem Review: EzDsl](#3-subsystem-review-ezdsl)
   - [3.1 Memory Allocation & Allocator Bypass Bugs](#31-memory-allocation--allocator-bypass-bugs)
   - [3.2 Symbol Table Resolution & Scope Hierarchy](#32-symbol-table-resolution--scope-hierarchy)
   - [3.3 Semantic Analysis Pipeline & Missing Passes](#33-semantic-analysis-pipeline--missing-passes)
   - [3.4 Code Generators & CLI Orchestration](#34-code-generators--cli-orchestration)
4. [Subsystem Review: EzMir](#4-subsystem-review-ezmir)
   - [4.1 Critical Correctness Bugs (Builder Insertion & PHI Nodes)](#41-critical-correctness-bugs-builder-insertion--phi-nodes)
   - [4.2 Pass Manager Invalidation & Liveness Arena Leaks](#42-pass-manager-invalidation--liveness-arena-leaks)
   - [4.3 IR Representation Density & Intrusive Lists](#43-ir-representation-density--intrusive-lists)
   - [4.4 MIR Verification Invariants (`MirVerifierPass`)](#44-mir-verification-invariants-mirverifierpass)
5. [Subsystem Review: tests/EzMirTestSuite](#5-subsystem-review-testsezmirtestsuite)
   - [5.1 Synchronous Logging I/O Bottlenecks](#51-synchronous-logging-io-bottlenecks)
   - [5.2 Fixture Lifecycle & Memory Teardown](#52-fixture-lifecycle--memory-teardown)
   - [5.3 Assertion Hygiene & Deep SSA Verification](#53-assertion-hygiene--deep-ssa-verification)
   - [5.4 Coverage Expansion & Test DSL](#54-coverage-expansion--test-dsl)
6. [Subsystem Review: tests/EzDslTestSuite](#6-subsystem-review-testsezdsltestsuite)
   - [6.1 Negative Testing & Diagnostic Verification](#61-negative-testing--diagnostic-verification)
   - [6.2 AST Matchers & Test Boilerplate Elimination](#62-ast-matchers--test-boilerplate-elimination)
   - [6.3 Gold-File Code Generator Verification](#63-gold-file-code-generator-verification)
7. [Unified Prioritized Remediation Roadmap](#7-unified-prioritized-remediation-roadmap)
8. [Concrete Refactoring Blueprints & Implementation Code](#8-concrete-refactoring-blueprints--implementation-code)

---

# 1. Executive Summary & Cross-Cutting Architectural Themes

A comprehensive code and architecture audit was performed across the foundational layers of the **EzPacker** compiler: **`EzCore`**, **`EzDsl`**, **`EzMir`**, **`tests/EzMirTestSuite`**, and **`tests/EzDslTestSuite`**.

```
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                   EzPacker Compiler Architecture                                 │
├──────────────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                                  │
│  [EzDsl] Target & ISA Specs (.tdf, .idf, .lad, .lrd, .isf, .tyf, .ccdf)                          │
│     │                                                                                            │
│     ▼ (Multi-pass Sema & C++ Emitters)                                                           │
│  [EzMir] Generated Encoders, Action Matrices, Type Tables, Instruction Builders                 │
│     │                                                                                            │
│     ▼ (SSA MIR Passes: Verifier, Liveness, Legalizer, ISel, RegAlloc, FrameLowerer)              │
│  [EzTriple / EzCodeEmitter] Machine Code Emission                                               │
│                                                                                                  │
│  ══════════════════════════════════════════════════════════════════════════════════════════════  │
│  [EzCore Foundation] FlexInt / FlexFloat (LibBF & LibTomMath), DiagnosticCollector, SourceMgr   │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
```

### Core Identified Vulnerabilities & Performance Themes:

1. **Memory Allocator Bypasses & Permanent Arena Leaks**:
   - **`EzDsl`**: `PmrListSink` checked for `state.memoryResource()` rather than `state.getAllocator()`, silently routing all vector allocations across all 7 DSL parsers to the global heap.
   - **`EzMir`**: `LivenessAnalysisPass` allocated `std::pmr::unordered_set` instances inside the fixed-point solver loop from a monotonic buffer, permanently leaking memory until whole-module completion.
   - **`EzCore`**: `SourceManager` allocated entries via placement `new` but lacked an explicit destructor, leaking internal buffers.

2. **Critical Correctness & Safety Hazards**:
   - **`EzMir`**: `MirBlockBuilder::build()` initialized with `InsertionType::InsertAfter` and a stale iterator, causing subsequent instructions to insert out of order.
   - **`EzCore`**: `FlexInt::operator=` called `mp_init_copy` without clearing previous limbs, causing memory leaks and undefined behavior on self-assignment. Missing move constructors (Rule of 5 violation) caused thousands of multi-precision heap allocations during constant folding.
   - **`EzCore`**: `FlexFloat::dump()` (>64-bit) copied only mantissa limbs, omitting sign and exponent and corrupting 128-bit IEEE-754 serialization.

3. **Algorithmic Scalability ($O(N^2)$ to $O(1)$)**:
   - **`EzDsl`**: `SymbolTable::getSymInScope` performed linear searches over symbol vectors, causing quadratic slowdown during parsing of large target architectures.
   - **`EzMir`**: `MirPassManager` unconditionally called `invalidateAnalysis()` on every instruction/block iteration even when MIR was unmodified ($O(N^3)$ pass thrashing).

4. **Test Quality & Tooling Fragility**:
   - **`tests/EzMirTestSuite`**: Uncontrolled synchronous console logging printed >3,000 lines of trace dumps, slowing test runs by 10x.
   - **`tests/EzDslTestSuite`**: Negative tests asserted only `EXPECT_FALSE(...)` without verifying error codes or diagnostic locations, masking false-positive passes.

---

# 2. Subsystem Review: EzCore

### 2.1 Core Utilities & Header Hygiene

#### 1. Global Static Logger in Common Header
- **File**: [`EzCore/include/EzCoreCommon.h:28`](file:///home/osikasuke/Repos/EzPacker/EzCore/include/EzCoreCommon.h#L28)
- **Issue**: `inline std::unique_ptr<SyncLogger> g_logger = EzLogger::createSyncLogger("EzPacker");` causes Static Initialization Order Fiasco (SIOF) hazards and forces dynamic allocation on any translation unit including the header.
- **Remediation**: Replace with a Meyer's singleton (`SyncLogger& getDefaultLogger()`) with function-local static initialization.

#### 2. Non-Standard `extern "C"` Linkage & Header Pollution
- **File**: [`EzCore/include/EzCoreCommon.h:18-24`](file:///home/osikasuke/Repos/EzPacker/EzCore/include/EzCoreCommon.h#L18-L24)
- **Issue**: `extern "C" { namespace libbf { #include <libbf.h> }; };` is non-standard in C++ (namespaces cannot be nested in `extern "C"` blocks). Transitively includes heavy C headers into all units.
- **Remediation**: Nest `extern "C"` inside the namespace: `namespace libbf { extern "C" { #include <libbf.h> } }`. Restrict library headers to `FlexInt.cpp` and `FlexFloat.cpp`.

#### 3. Undefined Behavior in `StringUtils`
- **File**: [`EzCore/include/StringUtils.h:9-28`](file:///home/osikasuke/Repos/EzPacker/EzCore/include/StringUtils.h#L9-L28)
- **Issue**: Passing signed `char` directly to `tolower`/`toupper` without casting to `unsigned char` causes UB for values $< 0$. Redundant `.resize()` after copy in `StrToUpper`.
- **Remediation**: Accept `std::string_view`, cast characters via `static_cast<unsigned char>`, and provide zero-allocation in-place overloads (`StrToLowerInPlace`).

---

### 2.2 FlexNumber Subsystem (`FlexInt`, `FlexFloat`)

#### 1. Missing Rule of 5 (No Move Semantics)
- **Files**: [`EzCore/include/FlexNumber/FlexInt.h`](file:///home/osikasuke/Repos/EzPacker/EzCore/include/FlexNumber/FlexInt.h), [`EzCore/include/FlexNumber/FlexFloat.h`](file:///home/osikasuke/Repos/EzPacker/EzCore/include/FlexNumber/FlexFloat.h)
- **Issue**: Both classes define custom copy constructors and destructors but omit move constructors and move assignment operators. Every binary arithmetic operation (`+`, `-`, `*`, `/`) and return-by-value triggers deep buffer copies (`mp_init_copy` / `bf_set`).
- **Remediation**: Implement `noexcept` move constructors and move assignments transferring internal limb pointers and context structs.

#### 2. Memory Leak and Self-Assignment in `FlexInt::operator=`
- **File**: [`EzCore/src/FlexNumber/FlexInt.cpp:99-107`](file:///home/osikasuke/Repos/EzPacker/EzCore/src/FlexNumber/FlexInt.cpp#L99-L107)
- **Issue**: Calling `mp_init_copy(&m_number, &other.m_number)` on an already initialized `m_number` leaks the previous limb buffer (`m_number.dp`). Self-assignment (`x = x;`) corrupts memory.
- **Remediation**: Check `this != &other` and use `mp_copy(&other.m_number, &m_number)`.

#### 3. Extreme Heap Churn in `FlexInt::clampToTwosComplement()`
- **File**: [`EzCore/src/FlexNumber/FlexInt.cpp:542-611`](file:///home/osikasuke/Repos/EzPacker/EzCore/src/FlexNumber/FlexInt.cpp#L542-L611)
- **Issue**: Initializes and clears 4 `mp_int` instances (`maxVal`, `minVal`, `modMask`, `fullRange`) on **every arithmetic step and constructor**.
- **Remediation**: Implement an $O(1)$ fast-path: inspect `mp_count_bits(&m_number)`. If the number already fits within `m_bitWidth`, return immediately (0 allocations). For bitwidths $\le 64$, clamp using native integer arithmetic.

#### 4. Corrupted IEEE-754 Serialization in `FlexFloat::dump()` (>64-bit)
- **File**: [`EzCore/src/FlexNumber/FlexFloat.cpp:368-376`](file:///home/osikasuke/Repos/EzPacker/EzCore/src/FlexNumber/FlexFloat.cpp#L368-L376)
- **Issue**: Copies raw `m_number.tab` limbs into the output buffer, omitting the exponent (`m_number.expn`) and sign (`m_number.sign`) fields for 128-bit/quad floats.
- **Remediation**: Utilize LibBF IEEE-754 serialization utilities or pack sign, exponent, and mantissa limbs according to IEEE-754 binary128 specifications.

---

### 2.3 Diagnostics Pipeline

#### 1. Uninitialized Variables in `DiagnosticScope`
- **File**: [`EzCore/include/Diagnostics/DiagnosticScope.h:54-56`](file:///home/osikasuke/Repos/EzPacker/EzCore/include/Diagnostics/DiagnosticScope.h#L54-L56)
- **Issue**: `bool m_hasFatalErrors;` and `DiagnosticScopeAction m_action;` are uninitialized, causing undefined behavior when querying scope state.
- **Remediation**: Add in-class default initializers: `bool m_hasFatalErrors{ false };` and `DiagnosticScopeAction m_action{ DiagnosticScopeAction::Commit };`.

#### 2. Deep Scope Copy on `endScope()`
- **File**: [`EzCore/src/Diagnostics/DiagnosticCollector.cpp:66-67`](file:///home/osikasuke/Repos/EzPacker/EzCore/src/Diagnostics/DiagnosticCollector.cpp#L66-L67)
- **Issue**: `auto closingScope = m_scopes.back();` clones the entire vector of `DiagnosticMessage`s and their PMR strings right before popping the element.
- **Remediation**: Move the scope: `DiagnosticScope closingScope = std::move(m_scopes.back()); m_scopes.pop_back();`.

#### 3. Missing Mutex Lock in `DiagnosticCollector::addListener`
- **File**: [`EzCore/src/Diagnostics/DiagnosticCollector.cpp:44`](file:///home/osikasuke/Repos/EzPacker/EzCore/src/Diagnostics/DiagnosticCollector.cpp#L44)
- **Issue**: `m_listeners.push_back(listener)` is unprotected, while `onDiag()` reads `m_listeners` under a lock, creating a data race during concurrent compilation.
- **Remediation**: Acquire `std::lock_guard lock(m_mutex);` in `addListener`.

---

### 2.4 SourceManager Subsystem

#### 1. Missing Destructor Cleanup
- **File**: [`EzCore/include/SourceManager/SourceManager.h:13-104`](file:///home/osikasuke/Repos/EzPacker/EzCore/include/SourceManager/SourceManager.h#L13-L104)
- **Issue**: `SourceFileEntry` objects are allocated using placement `new` from `m_alloc`, but `~SourceManager()` is defaulted. Internal strings and vectors (`m_content`, `m_name`, `m_lines`) never have their destructors called.
- **Remediation**: Implement `~SourceManager()` to explicitly destroy and deallocate each entry.

#### 2. Redundant File Buffer Copy in `loadFile`
- **File**: [`EzCore/src/SourceManager/SourceManager.cpp:204-213`](file:///home/osikasuke/Repos/EzPacker/EzCore/src/SourceManager/SourceManager.cpp#L204-L213)
- **Issue**: Reads file into a temporary `std::string` on the heap, then copies it into `entry->m_content` (`std::pmr::string`).
- **Remediation**: Read directly into the PMR buffer allocated from `m_alloc` in a single operation.

---

# 3. Subsystem Review: EzDsl

### 3.1 Memory Allocation & Allocator Bypass Bugs

#### 🚨 Critical: `PmrListSink` Allocator Bypass
- **File**: [`EzDsl/include/Parser/CommonParsers.h:165-175`](file:///home/osikasuke/Repos/EzPacker/EzDsl/include/Parser/CommonParsers.h#L165-L175)
- **Issue**: `PmrListSink` checked `if constexpr (requires { state.memoryResource(); })`, but `ParseContext` defines `getAllocator()`. All lists fell back to `std::pmr::get_default_resource()` (global heap), completely bypassing the 16MB monotonic arena.
- **Remediation**: Update concept check to support `state.getAllocator()`.

#### ⚠️ Heap Allocation in Expression Trees
- **Files**: [`EzDsl/include/Ast/InstructionDefLangAst.h:55`](file:///home/osikasuke/Repos/EzPacker/EzDsl/include/Ast/InstructionDefLangAst.h#L55), [`EzDsl/include/Sema/Symbols/Symbols.h:111`](file:///home/osikasuke/Repos/EzPacker/EzDsl/include/Sema/Symbols/Symbols.h#L111)
- **Issue**: `BitExpression` and `ResolvedBitExpr` wrap child nodes in `std::shared_ptr<BitExpression>`, causing reference-counted heap allocations on every AST node.
- **Remediation**: Use non-owning raw pointers `BitExpression*` allocated from the monotonic memory resource.

---

### 3.2 Symbol Table Resolution & Scope Hierarchy

#### 1. $O(N)$ Linear Searches in `SymbolTable::getSymInScope`
- **File**: [`EzDsl/src/Sema/SymbolTable.cpp:98-111`](file:///home/osikasuke/Repos/EzPacker/EzDsl/src/Sema/SymbolTable.cpp#L98-L111)
- **Issue**: Iterates over `std::pmr::vector<ScopeId>` with string equality comparisons. In large targets with thousands of symbols, lookup degrades to $O(N^2)$.
- **Remediation**: Equip `Scope` with `std::pmr::unordered_map<std::string_view, SymbolId> m_symbolMap` for $O(1)$ lookups.

#### 2. Discarded `parentId` in `SymbolTable::createScope`
- **File**: [`EzDsl/src/Sema/SymbolTable.cpp:14-21`](file:///home/osikasuke/Repos/EzPacker/EzDsl/src/Sema/SymbolTable.cpp#L14-L21)
- **Issue**: `createScope(parentId, debugName)` ignores `parentId` and passes `m_currentScopeId` to the constructor.
- **Remediation**: Pass `parentId` to `Scope` constructor.

#### 3. Lexical Shadowing Blocked by `declareSym`
- **File**: [`EzDsl/src/Sema/SymbolTable.cpp:62`](file:///home/osikasuke/Repos/EzPacker/EzDsl/src/Sema/SymbolTable.cpp#L62)
- **Issue**: `declareSym` calls `getSymByName(name)`, which searches all parent scopes and rejects valid inner variable shadowing.
- **Remediation**: Only check for duplicate symbols within the *current* scope (`getSymInScope(m_currentScopeId, name)`).

---

### 3.3 Semantic Analysis Pipeline & Missing Passes

| Language | Extension | AST / Parser | Sema Pass | C++ Code Generator |
|:---|:---:|:---:|:---:|:---:|
| **Types** | `.tyf` | ✅ Done | ✅ `TypePass` | ✅ `CppMirTypeTableGenerator` |
| **Generic IR** | `.irdf` | ✅ Done | ✅ `IrInstructionPass` | ✅ `CppMirInstructionGenerator` |
| **Target Def** | `.tdf` | ✅ Done | ✅ `RegisterBankPass` | ❌ Planned: `CppTargetInstGenerator` |
| **Target Inst** | `.idf` | ✅ Done | ✅ `TargetInstPass` | ❌ Planned: `CppTargetInstGenerator` |
| **Legalize Action** | `.lad` | ✅ Done | ❌ Missing: `LegalizeActionPass` | ❌ Planned: `CppLegalizerGenerator` |
| **Legalize Rules** | `.lrd` | ✅ Done | ❌ Missing: `LegalizeRulePass` | ❌ Planned: `CppLegalizerGenerator` |
| **Instruction Sel** | `.isf` | ✅ Done | ❌ Missing: `ISelPatternPass` | ❌ Planned: `CppISelTableGenerator` |
| **Calling Conv** | `.ccdf` | ✅ Done | ❌ Missing: `CallingConvPass` | ❌ Planned: `CppCallingConvGenerator` |

#### Missing Inclusion Resolver
- `.tdf` files contain `include idf "..."`, `include lad "..."`, `include isf "..."`. A multi-file inclusion resolution driver is required to load child specifications into the unified symbol table.

---

### 3.4 Code Generators & CLI Orchestration

1. **Streaming Output Buffer**: Replace `std::ostringstream` and intermediate string copies in `CppMirTypeTableGenerator` and `CppMirInstructionGenerator` with `std::format_to(std::back_inserter(buf), ...)`.
2. **CLI Extension**: Expand [`EzDsl/src/Driver/Main.cpp`](file:///home/osikasuke/Repos/EzPacker/EzDsl/src/Driver/Main.cpp) to accept `.tdf`, `.idf`, `.lad`, `.lrd`, `.isf`, and `.ccdf` files.

---

# 4. Subsystem Review: EzMir

### 4.1 Critical Correctness Bugs (Builder Insertion & PHI Nodes)

#### 🚨 Critical: `MirBlockBuilder` Out-of-Order Instruction Insertion
- **Files**: [`EzMir/src/Block/MirBlockBuilder.cpp`](file:///home/osikasuke/Repos/EzPacker/EzMir/src/Block/MirBlockBuilder.cpp), [`EzMir/src/Instruction/MirInstructionBuilder.cpp`](file:///home/osikasuke/Repos/EzPacker/EzMir/src/Instruction/MirInstructionBuilder.cpp)
- **Mechanism**: `MirBlockBuilder::build()` sets `InsertionType::InsertAfter` with a stale iterator on an empty block. The builder iterator is never updated upon insertion, causing subsequent instructions to be inserted out of order (reversing or misplacing sequences).
- **Remediation**: Use `InsertionType::Append` by default for block builders, and ensure `finalizeInstruction()` updates `m_insertionPoint.m_iterator` to the newly inserted instruction.

#### 🚨 Fragile PHI Node Mapping in `NonSsaToSsaPass`
- **File**: [`EzMir/src/MirPasses/Passes/NonSsaToSsaPass.cpp`](file:///home/osikasuke/Repos/EzPacker/EzMir/src/MirPasses/Passes/NonSsaToSsaPass.cpp)
- **Issue**: PHI nodes store flat operand values mapped by positional offset on a sorted `std::set<size_t>` of predecessor block IDs without storing the incoming `MirBlock*`. Any CFG block reordering or edge splitting corrupts PHI argument associations.
- **Remediation**: Represent PHI operands as explicit `(MirOperand *val, MirBlock *incomingBlock)` pairs.

---

### 4.2 Pass Manager Invalidation & Liveness Arena Leaks

#### 🚨 Catastrophic Liveness Dataflow Arena Leak
- **File**: [`EzMir/src/MirPasses/Passes/LivenessAnalysisPass.cpp`](file:///home/osikasuke/Repos/EzPacker/EzMir/src/MirPasses/Passes/LivenessAnalysisPass.cpp)
- **Issue**: `computeGlobalLiveness()` allocates brand-new `std::pmr::unordered_set<MirRegisterRef>` on the monotonic arena inside the fixed-point loop on every iteration, leaking memory permanently.
- **Remediation**: Replace sets with `BitVector` / `DenseBitSet` indexed by `MirId`. Dataflow equations become bitwise word operations (`liveOut |= liveIn[succ]`) with zero allocations.

#### 🚨 Pass Manager Invalidation Thrashing
- **File**: [`EzMir/src/MirPasses/MirPassManager.cpp`](file:///home/osikasuke/Repos/EzPacker/EzMir/src/MirPasses/MirPassManager.cpp)
- **Issue**: `invalidateAnalysis()` is called after every function, block, and instruction step even when `m_modifiedMir == false`, causing $O(N^3)$ recalculations on instruction-level passes.
- **Remediation**: Only invalidate analyses when `r.m_modifiedMir == true` and after the pass completes its iteration over the container.

---

### 4.3 IR Representation Density & Intrusive Lists

#### 1. Instruction Memory Density
- **File**: [`EzMir/include/Instruction/MirInstruction.h`](file:///home/osikasuke/Repos/EzPacker/EzMir/include/Instruction/MirInstruction.h)
- **Issue**: `MirInstruction` is ~128 bytes, containing 3 separate PMR vectors (`m_operands`, `m_definedRegisters`, `m_usedRegisters`). `getOperand()` unconditionally invalidates def/use caches on read.
- **Remediation**: Compute defs/uses on-the-fly from opcode metadata flags or store as small inline buffers. Remove cached def/use vectors and fix `getOperand()` constness.

#### 2. Intrusive Doubly-Linked Lists (`ilist`)
- **Files**: [`EzMir/include/Block/MirBlock.h`](file:///home/osikasuke/Repos/EzPacker/EzMir/include/Block/MirBlock.h), [`EzMir/include/Function/MirFunction.h`](file:///home/osikasuke/Repos/EzPacker/EzMir/include/Function/MirFunction.h)
- **Issue**: `std::pmr::list` allocates 24-byte `_List_node` wrappers per instruction and block; `at()` is an $O(N)$ traversal.
- **Remediation**: Embed `m_prev`/`m_next` intrusive pointers directly in `MirInstruction` and `MirBlock`, eliminating node wrapper allocations and enabling $O(1)$ pointer-to-iterator conversions.

#### 3. `CallLoweringState` Vector Erase
- **File**: [`EzMir/src/Function/CallLoweringState.cpp`](file:///home/osikasuke/Repos/EzPacker/EzMir/src/Function/CallLoweringState.cpp)
- **Issue**: `pool.erase(pool.begin())` shifts the remaining vector elements on every argument allocation.
- **Remediation**: Use an index cursor `size_t m_cursor = 0;` for $O(1)$ allocation.

---

### 4.4 MIR Verification Invariants (`MirVerifierPass`)

- Implement `MirVerifierPass` to validate:
  1. **SSA Invariant**: Every virtual register has exactly one definition; uses are dominated by their definition.
  2. **CFG Symmetry**: Predecessors match successors across all basic blocks.
  3. **Terminator Invariant**: Exactly one terminator instruction at the end of each block.
  4. **PHI Grouping**: PHI nodes appear exclusively at block entry with valid incoming block pairs.

---

# 5. Subsystem Review: tests/EzMirTestSuite

### 5.1 Synchronous Logging I/O Bottlenecks
- **File**: [`tests/EzMirTestSuite/src/EzMirTestSuite.cpp`](file:///home/osikasuke/Repos/EzPacker/tests/EzMirTestSuite/src/EzMirTestSuite.cpp)
- **Issue**: Unconditionally enables `Diag_Trace` and `Diag_Debug` to `std::cout`, dumping >3,000 lines of terminal output and function dumps during normal test runs.
- **Remediation**: Silence trace logging by default. Route logs to a memory buffer or only print on test failure.

### 5.2 Fixture Lifecycle & Memory Teardown
- **File**: [`tests/EzMirTestSuite/src/EzMirTestSuite.cpp`](file:///home/osikasuke/Repos/EzPacker/tests/EzMirTestSuite/src/EzMirTestSuite.cpp)
- **Issue**: `destroy()` resets only 4 of 8 members and never calls `m_arena.release()`.
- **Remediation**: Invoke `m_arena.release()`, reset all smart pointers, and clear dangling pointers.

### 5.3 Assertion Hygiene & Deep SSA Verification
- **Files**: [`tests/EzMirTestSuite/tests/T_*.cpp`](file:///home/osikasuke/Repos/EzPacker/tests/EzMirTestSuite/tests/)
- **Issue**: Multiple tests use `EXPECT_*` instead of `ASSERT_*` before indexing operands or dereferencing pointers, risking test runner crashes. `T_NonSsaToSsa.cpp` checks only PHI opcode and operand count.
- **Remediation**: Ensure pointer/index checks use `ASSERT_*` before dereference. Implement full SSA invariant verification checking incoming block/value pairs.

### 5.4 Coverage Expansion & Test DSL
- Add fluent test helpers (`vreg(type, name)`, `imm(type, val)`, `createBlock(name)`, `addJump(from, to)`).
- Expand `T_Instruction.cpp` with parameterized tests (`TEST_P`) across all opcode families (arithmetic, bitwise, comparison, memory, control flow).
- Upgrade `EzMirTestSuiteCallingConv` to a configurable ABI mock supporting GPR/FPR argument assignment and return registers.

---

# 6. Subsystem Review: tests/EzDslTestSuite

### 6.1 Negative Testing & Diagnostic Verification
- **Files**: Across all 13 test files in [`tests/EzDslTestSuite/tests/`](file:///home/osikasuke/Repos/EzPacker/tests/EzDslTestSuite/tests/)
- **Issue**: Negative tests assert only `EXPECT_FALSE(res.has_value())` or `EXPECT_FALSE(pass.run(...))` without checking error codes or source locations, giving false confidence on unexpected failures.
- **Remediation**: Introduce a `DiagnosticVerifier` utility checking exact diagnostic severity, code, line, and message substrings.

### 6.2 AST Matchers & Test Boilerplate Elimination
- **Issue**: Verifying AST nodes requires dozens of manual `std::holds_alternative` and `std::get` calls.
- **Remediation**: Introduce GoogleTest Matchers (`MatchesIdentifier("a")`, `MatchesOp(BitExprOp::Or)`). Provide `parseDsl<Rule, AstNode>()` in `DslTestSuiteAsGtest`.

### 6.3 Gold-File Code Generator Verification
- **Files**: [`tests/EzDslTestSuite/tests/T_EzDslCli_GenMirInstruction.cpp`](file:///home/osikasuke/Repos/EzPacker/tests/EzDslTestSuite/tests/T_EzDslCli_GenMirInstruction.cpp), [`tests/EzDslTestSuite/tests/T_EzDslCli_GenTypeTable.cpp`](file:///home/osikasuke/Repos/EzPacker/tests/EzDslTestSuite/tests/T_EzDslCli_GenTypeTable.cpp)
- **Issue**: Brittle multi-line `content.find(...)` chains; uses `std::this_thread::sleep_for(10ms)` for timestamp testing.
- **Remediation**: Compare against gold reference files in `fixtures/gold/`. Explicitly set `std::filesystem::last_write_time` instead of sleeping.

---

# 7. Unified Prioritized Remediation Roadmap

```
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                               EzPacker Master Remediation Roadmap                                │
├────────────────────────────────┬────────────────────────────────┬────────────────────────────────┤
│   Phase 1: Critical Safety     │   Phase 2: High Performance    │   Phase 3: Architecture        │
│   & Correctness (P0)           │   & Memory Efficiency (P1)     │   & Feature Completeness (P2)  │
├────────────────────────────────┼────────────────────────────────┼────────────────────────────────┤
│ 1. Fix MirBlockBuilder bug     │ 1. Rewrite Liveness (BitVector)│ 1. Implement missing DSL passes│
│ 2. Fix PmrListSink alloc hook  │ 2. FlexInt clamp fast-path     │ 2. Implement backend emitters  │
│ 3. Fix FlexInt operator= leak  │ 3. Scope hash map O(1) lookups │ 3. Implement MirVerifierPass   │
│ 4. Rule of 5 for FlexInt/Float │ 4. Fix PassManager inval       │ 4. Compact MirInstruction/Oper │
│ 5. Fix DiagnosticScope UB      │ 5. Intrusive lists in MIR      │ 5. Multi-file .tdf resolver    │
│ 6. Fix SourceManager dtor leak │ 6. Fix CallLoweringState erase │ 6. Expand test suites & DSL    │
└────────────────────────────────┴────────────────────────────────┴────────────────────────────────┘
```

---

# 8. Concrete Refactoring Blueprints & Implementation Code

### Blueprint 1: `PmrListSink` Allocator Hook Fix (`EzDsl/include/Parser/CommonParsers.h`)

```cpp
// In EzDsl/include/Parser/CommonParsers.h
template <typename State>
_sink sink(State &state) const
{
    if constexpr (requires { state.getAllocator(); })
    {
        return _sink(state.getAllocator());
    }
    else if constexpr (requires { state.memoryResource(); })
    {
        return _sink(state.memoryResource());
    }
    else
    {
        return _sink(std::pmr::get_default_resource());
    }
}
```

---

### Blueprint 2: `FlexInt` Move Semantics & Copy Assignment Fix (`EzCore/src/FlexNumber/FlexInt.cpp`)

```cpp
// In EzCore/src/FlexNumber/FlexInt.cpp

// Move Constructor
FlexInt::FlexInt(FlexInt &&other) noexcept :
    m_number(other.m_number),
    m_isSigned(other.m_isSigned),
    m_bitWidth(other.m_bitWidth),
    m_lastErr(other.m_lastErr)
{
    // Invalidate source multi-precision struct
    other.m_number.dp = nullptr;
    other.m_number.used = 0;
    other.m_number.alloc = 0;
}

// Move Assignment
FlexInt &FlexInt::operator=(FlexInt &&other) noexcept
{
    if (this != &other)
    {
        if (m_number.dp != nullptr)
            mp_clear(&m_number);

        m_number = other.m_number;
        m_isSigned = other.m_isSigned;
        m_bitWidth = other.m_bitWidth;
        m_lastErr = other.m_lastErr;

        other.m_number.dp = nullptr;
        other.m_number.used = 0;
        other.m_number.alloc = 0;
    }
    return *this;
}

// Safe Copy Assignment
FlexInt &FlexInt::operator=(const FlexInt &other)
{
    if (this != &other)
    {
        if (m_lastErr = mp_copy(&other.m_number, &m_number); m_lastErr != MP_OKAY)
            throw std::bad_alloc();

        m_isSigned = other.m_isSigned;
        m_bitWidth = other.m_bitWidth;
    }
    return *this;
}
```

---

### Blueprint 3: `MirBlockBuilder` Insertion Fix (`EzMir/src/Block/MirBlockBuilder.cpp`)

```cpp
// In EzMir/src/Block/MirBlockBuilder.cpp
MirInstructionBuilder MirBlockBuilder::build()
{
    // Default to InsertionType::Append so instructions are always emitted in sequence
    InsertionPoint point{
        .m_type = InsertionType::Append,
        .m_block = m_block,
        .m_iterator = m_block->getInstructions().end()
    };
    return MirInstructionBuilder(m_ctx, point);
}
```

---

### Blueprint 4: Liveness Analysis `BitVector` Engine (`EzMir/src/MirPasses/Passes/LivenessAnalysisPass.cpp`)

```cpp
// Dataflow equation execution without inner-loop allocations
void LivenessAnalysisPass::computeGlobalLiveness(MirFunction *func, const CodeFlowResult *cfg)
{
    const size_t numRegs = m_maxVRegId + 1;
    bool changed = true;

    while (changed)
    {
        changed = false;
        for (auto *block : func->getBlocks())
        {
            const auto blockId = block->getId();
            BitVector newLiveOut(numRegs);

            // liveOut = UNION(liveIn[succ])
            for (auto succId : cfg->getSuccessors(blockId))
            {
                newLiveOut |= m_liveIn[succId];
            }

            // liveIn = use | (liveOut & ~def)
            BitVector newLiveIn = m_use[blockId] | (newLiveOut.andNot(m_def[blockId]));

            if (newLiveIn != m_liveIn[blockId] || newLiveOut != m_liveOut[blockId])
            {
                m_liveIn[blockId] = std::move(newLiveIn);
                m_liveOut[blockId] = std::move(newLiveOut);
                changed = true;
            }
        }
    }
}
```

---

### Blueprint 5: Diagnostic Verifier for GTest Suites (`tests/EzDslTestSuite/include/EzDslTestSuite.h`)

```cpp
// Diagnostic assertion helper for robust negative testing
class DiagnosticVerifier
{
public:
    explicit DiagnosticVerifier(DiagnosticCollector *collector) : m_collector(collector) {}

    bool hasErrorContaining(std::string_view substring) const
    {
        for (const auto &msg : m_collector->getDiagnostics())
        {
            if (msg.m_severity == DiagnosticMessageType::Diag_Error &&
                msg.m_content.find(substring) != std::string_view::npos)
            {
                return true;
            }
        }
        return false;
    }
};

#define EXPECT_DIAG_ERROR(collector, substr) \
    EXPECT_TRUE(DiagnosticVerifier(collector).hasErrorContaining(substr))
```
