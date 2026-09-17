# MirLegalizer Architectural Specification & EzDSL Integration Plan

## 1. Executive Summary

The purpose of this specification is to design a next-generation, fully table-driven **`MirLegalizer`** for the **`EzTriple`** backend, eliminating all ad-hoc hardcoded instruction checks (such as `CALL` and `RET` branches in `MirLegalizer.cpp`) and unifying backend target legality definitions under **`EzDSL`** (`.lad` and `.lrd`).

### Core Design Goals
1. **100% Table-Driven Engine**: Zero hardcoded opcode or flag checks in the legalization dispatch loop. Special instructions like `CALL`, `RET`, `ALLOC`, `VA_ARG`, and atomic operations are handled through standardized legalization actions (`Lower`, `Custom`, `Libcall`, etc.) declared directly in target tables.
2. **First-Class Developer Convenience via EzDSL**: Express target legality using high-level, declarative syntax in `.lad` files—including instruction grouping, scalar clamping ranges, type aliases, multi-slot constraints, and action callbacks—eliminating manual C++ table construction.
3. **C++ Fluent Builder API (`LegalizerInfo`)**: Provide a modern, expressive C++ builder interface for test harnesses, mock targets, and target plugins, eliminating handwritten function-pointer switch tables.
4. **$O(1)$ High-Throughput Lookup**: A cache-friendly, multi-tiered lookup engine combining a dense 2D primary matrix for common scalar operations with a fast rule matcher for complex heterogeneous signatures.
5. **Worklist-Driven Execution Pipeline**: Replace the inefficient block-restart loop ($O(N \cdot K)$ iteration) with a robust worklist queue featuring mutation tracking, recursive re-checking, and cycle detection.
6. **Automated EzDSL Generator (`CppLegalizerGenerator`)**: Complete Phase 2.6 of the EzPacker backend roadmap by generating production-ready C++ action tables and rule dispatchers directly from `.lad` and `.lrd` sources.

---

## 2. Current Implementation Audit & Technical Debt Analysis

### 2.1 The Hardcoded Branch Problem
In [`EzTriple/src/Legalizer/MirLegalizer.cpp`](file:///E:/Repos/EzPacker/EzTriple/src/Legalizer/MirLegalizer.cpp#L80-L98):

```cpp
LegalizationResult MirLegalizer::legalizeInstruction(IntrusiveLinkedList<MirInstruction>::iterator it, MirBlock *block)
{
    if (!m_ctx) return LegalizationResult::Failed;
    MirInstruction *inst = *it;
    if (!inst) return LegalizationResult::NotModified;

    LegalizeCtx ctx(m_ctx, m_targetDesc, it);

    // Hardcoded special-case bypasses:
    if (inst->getFlags() & MirInstructionFlags::IsCall)
    {
        return LegalizeActions::LegalizeCall(ctx);
    }
    if (inst->getFlags() & MirInstructionFlags::IsReturn)
    {
        return LegalizeActions::LegalizeReturn(ctx);
    }

    const auto *actionTable = m_targetDesc->getLegalizeActionTable();
    // ...
}
```

#### Deficiencies
- **Breaks Abstraction & Target Polymorphism**: Targets that require specialized call sequences (e.g., PIC call thunks, syscalls, indirect stubs, fastcc calls, or tail calls) cannot configure their behavior through the target descriptor; the legalizer forcefully executes `LegalizeActions::LegalizeCall`.
- **Anti-Pattern Precedent**: Adding support for additional operations (e.g., `ALLOC`, `DYNAMIC_ALLOC`, `VA_START`, `ATOMIC_FENCE`, vector instructions) would inevitably introduce more `if (inst->getOpCode() == ...)` or `if (inst->getFlags() & ...)` checks into the core compiler loop.
- **Incomplete Separation of Concerns**: Function signature legalization is split arbitrarily between [`MirFunctionSignatureLegalizerPass.cpp`](file:///E:/Repos/EzPacker/EzTriple/src/Legalizer/MirFunctionSignatureLegalizerPass.cpp) (which handles `POP_ARG`/`END_ARG`) and hardcoded branches in `MirLegalizer.cpp` (which handles `CALL`/`RET`).

---

### 2.2 The Rigid 3-Operand Compact-ID Lookup Model
In [`EzTriple/include/Legalizer/MirLegalizeActionTable.h`](file:///E:/Repos/EzPacker/EzTriple/include/Legalizer/MirLegalizeActionTable.h#L9-L38):

```cpp
struct alignas(8) LegalizeQueryResult
{
    LegalizeAction m_action;
    uint8_t m_compactId;       // Target type compact ID
    uint8_t m_slot;            // Operand slot
    uint8_t m_libcallOffset;   // Libcall string pool offset
    uint16_t m_customActionId; // Custom callback ID
};

using LegalizeActionQueryFunc = LegalizeQueryResult (*)(size_t op1Type, size_t op2Type, size_t op3Type);

struct MirLegalizeActionTable
{
    LegalizeActionQueryFunc m_queryTable[static_cast<uint16_t>(MirInstructionOpCode::OPCODE_COUNT) + 1];
    LegalizeCustomActionFunc m_customActionTable[static_cast<uint8_t>(UINT8_MAX)];
};
```

#### Deficiencies
- **Hardcoded Operand Arity**: Assumes all instructions have at most 3 typed operands (`t0, t1, t2`). Instructions with 4+ operands (or variadic calls, intrinsic expansions, bitfield extractions, vector operations) cannot be cleanly checked.
- **Type-Only Predication**: Cannot express legality conditions dependent on:
  - Immediate values (e.g., shift amounts in range $[0, 31]$, division by power-of-two constants).
  - Operand kinds (e.g., register vs immediate vs memory reference).
  - Predicate / condition codes (e.g., signed vs unsigned comparison legality).
- **Manual Boilerplate in Target Descriptors**: Look at [`tests/EzTripleTestSuite/include/EzTripleTestSuite.h`](file:///E:/Repos/EzPacker/tests/EzTripleTestSuite/include/EzTripleTestSuite.h#L130-L325). For `MockTargetDesc`, developers must manually write individual functions (`queryAlu`, `queryDiv`, `queryCmp`, `querySext`, `queryMov`, `queryPopArg`, `queryEndArg`) filled with massive `switch` and `if` blocks, and wire them up in `initialize()`. This creates severe maintenance friction.

---

### 2.3 Block Restart Iteration Performance Flaw
In [`EzTriple/src/Legalizer/MirLegalizer.cpp`](file:///E:/Repos/EzPacker/EzTriple/src/Legalizer/MirLegalizer.cpp#L44-L75):

```cpp
constexpr size_t MaxPasses = 32;
size_t passCount = 0;
bool changed = true;

while (changed && passCount < MaxPasses)
{
    changed = false;
    passCount++;

    auto &instList = block->getInstructions();
    auto it = instList.begin();

    while (it != instList.end())
    {
        auto curIt = it;
        ++it;

        LegalizationResult res = legalizeInstruction(curIt, block);
        if (res == LegalizationResult::Failed) return false;

        if (res == LegalizationResult::Legalized)
        {
            changed = true;
            // Restart iteration on this block to ensure newly introduced instructions are legalized!
            break; 
        }
    }
}
```

#### Deficiencies
- **$O(N \cdot K)$ Worst-Case Quadratic Overhead**: If a basic block contains $N$ instructions and several require legalization, the outer loop aborts with `break` and restarts scanning from instruction 0 every time an instruction is changed.
- **Fixed Hard Limit Failure**: If a chain of complex expansions takes more than 32 passes (e.g., large vector lowering or wide multi-word integer arithmetic), legalization silently truncates or fails without explicit cycle analysis.

---

### 2.4 Monolithic Action Implementations
In [`EzTriple/src/Legalizer/Actions/LegalizeNarrowScalarAction.cpp`](file:///E:/Repos/EzPacker/EzTriple/src/Legalizer/Actions/LegalizeNarrowScalarAction.cpp#L145-L278):

```cpp
bool isCompare = (instr->getMetadata().m_category == MirInstructionCategory::MirCat_Compare);
if (isCompare) {
    if (instr->getOpCode() == MirInstructionOpCode::CMP_EQ) { ... }
    else if (instr->getOpCode() == MirInstructionOpCode::CMP_NE) { ... }
    else { return LegalizationResult::Failed; }
}
if (instr->getOpCode() == MirInstructionOpCode::ADD ...) { ... }
else if (instr->getOpCode() == MirInstructionOpCode::SUB ...) { ... }
else if (instr->getOpCode() == MirInstructionOpCode::NEG ...) { ... }
else if ((instr->getOpCode() == MirInstructionOpCode::NOT || instr->getOpCode() == MirInstructionOpCode::MOV) ...) { ... }
else { return LegalizationResult::Failed; }
```

#### Deficiencies
- Narrowing logic is hardcoded inside a single file with an explicit `if/else` cascade.
- Standard operations like `MUL`, `SHL`, `LSHR`, `ASHR`, `UDIV`, `SDIV`, or bitwise rotations cannot be narrowed without modifying the engine source code.
- Target-specific rewriters from `.lrd` files are not integrated into the narrowing pipeline.

---

## 3. The New MirLegalizer Architecture

```
┌────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           EzDSL Frontend                                               │
│                                Target Definition File: <Target>.lad                                    │
│                                Rewrite Rules File:    <Target>.lrd                                     │
└───────────────────────────────────────────────────┬────────────────────────────────────────────────────┘
                                                    │ Two-Phase Sema: LegalizeActionPass & LegalizeRulePass
                                                    ▼
┌────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                      CppLegalizerGenerator                                             │
│  - Synthesizes <Target>LegalizerActionTable.h / .cpp                                                  │
│  - Generates Dense 2D Action Matrix (O(1) Hot Path)                                                    │
│  - Generates Sparse Rule Matcher Chains (Heterogeneous Signatures)                                     │
│  - Generates Target Lowering Dispatchers (CALL, RET, ALLOC, Custom)                                    │
└───────────────────────────────────────────────────┬────────────────────────────────────────────────────┘
                                                    │ Synthesized C++ Code
                                                    ▼
┌────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                       EzTriple Legalizer Engine                                        │
│                                                                                                        │
│   MirFunction                                                                                          │
│        │                                                                                               │
│        ▼                                                                                               │
│   [Worklist Initialization] ─── Push Block Instructions to Processing Queue                            │
│        │                                                                                               │
│        ▼                                                                                               │
│   [Legality Query Builder] ──── Extract (Opcode, Operands, Types, Flags, Immediates)                   │
│        │                                                                                               │
│        ▼                                                                                               │
│   [LegalizerInfo::query] ────── 3-Tier Dispatch:                                                       │
│        │                        ├── Tier 1: Dense 2D Primary Matrix Lookup (O(1))                      │
│        │                        ├── Tier 2: Heterogeneous Signature Predicate Evaluator                │
│        │                        └── Tier 3: Target Action Resolver (Lower / Custom / Libcall)          │
│        │                                                                                               │
│        ├── Action == Legal ───────────── Continue (no work)                                            │
│        │                                                                                               │
│        └── Action != Legal ──────────── Execute Action Handler via InsertionTracker:                   │
│                                         ├── LegalizeWidenScalar                                        │
│                                         ├── LegalizeNarrowScalar                                       │
│                                         ├── LegalizeBitcast                                            │
│                                         ├── LegalizeLibcall                                            │
│                                         ├── LegalizeLower (Target-driven CALL, RET, ALLOC)             │
│                                         └── LegalizeCustom (Rewrite rules or C++ callbacks)            │
│                                                │                                                       │
│                                                ▼                                                       │
│                                   [Queue Newly Created Instructions back into Worklist]                │
│                                   [Cycle Detection: abort if max iterations exceeded]                  │
└────────────────────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 4. Legality Action Specification

### 4.1 Extended `LegalizeActionKind` Enumeration

In [`EzTriple/include/Legalizer/Actions/LegalizeActionCommon.h`](file:///E:/Repos/EzPacker/EzTriple/include/Legalizer/Actions/LegalizeActionCommon.h):

```cpp
enum class LegalizeActionKind : uint8_t
{
    Legal = 0,     // Instruction is natively supported for given types/operands.
    WidenScalar,   // Promote scalar operand(s) to a larger legal scalar type (e.g. i8 -> i32).
    NarrowScalar,  // Split scalar operand(s) into smaller legal types (e.g. i128 -> 2x i64).
    Bitcast,       // Reinterpret bit pattern into a legal type of equal bit-width (e.g. f32 -> i32).
    Libcall,       // Lower instruction into a standard ABI runtime library call (e.g. __divdi3).
    Lower,         // Decompose complex high-level instruction into standard target MIR primitives
                   // (Used for CALL -> PUSH_ARG/POP_RET, RET -> PUSH_RET/RET, ALLOC -> frame layout).
    Custom,        // Delegate legalization to target-defined rewrite rule or C++ member function.
    Unsupported    // Explicitly rejected combination. Emits a compiler diagnostic error.
};
```

### 4.2 Generalized `LegalityQuery` Structure

```cpp
struct LegalityQuery
{
    MirInstructionOpCode m_opcode;
    uint32_t m_flags;                     // MirInstructionFlags (IsCall, IsReturn, IsSigned, etc.)
    size_t m_operandCount;
    std::array<MirType *, 4> m_types;     // Concrete MirType pointers for first 4 operands
    std::array<uint8_t, 4> m_compactIds;  // Compact type IDs for fast indexing
    std::array<ExpectedOperandType, 4> m_operandKinds; // Register, Immediate, Memory, etc.
    int64_t m_immValue{ 0 };              // Immediate constant value if second operand is imm
    bool m_hasImm{ false };
};
```

### 4.3 `LegalityResponse` Decision Structure

```cpp
struct alignas(8) LegalityResponse
{
    LegalizeActionKind m_action{ LegalizeActionKind::Unsupported };
    uint8_t m_slot{ 0 };                 // Primary operand slot targeted by action (0, 1, 2)
    uint8_t m_targetCompactId{ 0 };      // Destination type compact ID for Widen/Narrow/Bitcast
    uint16_t m_handlerOrStringId{ 0 };   // Libcall string index or Custom/Lower callback index

    bool isLegal() const noexcept { return m_action == LegalizeActionKind::Legal; }
    bool isUnsupported() const noexcept { return m_action == LegalizeActionKind::Unsupported; }
};
```

---

## 5. High-Throughput Three-Tier Lookup Engine

To combine **zero-overhead compilation** on common operations with **arbitrary expressiveness** for complex signatures, `LegalizerInfo` organizes rules into three tiers:

```
                          ┌────────────────────────┐
                          │     LegalityQuery      │
                          └───────────┬────────────┘
                                      │
                                      ▼
                        ┌────────────────────────────┐
                        │   Is Primary Opcode &      │
                        │   Homogeneous Scalar Type? │
                        └─────────────┬──────────────┘
                                      │
                         YES          │          NO
             ┌────────────────────────┘          └────────────────────────┐
             ▼                                                            ▼
┌─────────────────────────────┐                              ┌─────────────────────────────┐
│    Tier 1: Dense Matrix     │                              │    Tier 2: Signature Chain  │
│  O(1) Array Direct Index    │                              │  Predicate & Pattern Test   │
│  g_DenseMatrix[Op][Type]    │                              │  (SEXT, STORE, CMP, etc.)   │
└────────────┬────────────────┘                              └────────────┬────────────────┘
             │                                                            │
             └────────────────────────┬───────────────────────────────────┘
                                      ▼
                        ┌────────────────────────────┐
                        │ Action == Lower or Custom? │
                        └─────────────┬──────────────┘
                                      │
                         YES          │          NO
             ┌────────────────────────┘          └────────────────────────┐
             ▼                                                            ▼
┌─────────────────────────────┐                              ┌─────────────────────────────┐
│    Tier 3: Action Table     │                              │   Direct Standard Action    │
│  Dispatch Handler Callback  │                              │  (Legal, Widen, Narrow,     │
│  (CALL, RET, ALLOC, Rules)  │                              │   Bitcast, Libcall)         │
└─────────────────────────────┘                              └─────────────────────────────┘
```

### Tier 1: Dense 2D Flat Array ($O(1)$ Constant Time)
For symmetric arithmetic, logic, and move operations (`ADD`, `SUB`, `AND`, `OR`, `XOR`, `MOV`, etc.) where all operands share the primary type:
```cpp
// Indexed by [Opcode - 0][TypeCompactId]
// Total table size: ~150 opcodes * 32 compact types = ~4.8 KB (fits comfortably in L1 Cache!)
static const LegalityResponse g_PrimaryMatrix[OPCODE_COUNT][MAX_COMPACT_TYPES];
```
Direct evaluation:
```cpp
inline LegalityResponse queryFast(MirInstructionOpCode op, uint8_t typeId) const
{
    return g_PrimaryMatrix[static_cast<uint16_t>(op)][typeId];
}
```

### Tier 2: Heterogeneous Rule Checkers
For operations where operands have different types (e.g. `SEXT i64, i8`, `STORE i32, ptr`, `CMP_EQ i1, f64, f64`), the table registers a pointer to an optimized pattern checker for that opcode.

### Tier 3: Action Handler Registry
For actions with `ActionKind::Lower` or `ActionKind::Custom`, the response contains an index into a table of functional lowering callbacks:
```cpp
using LegalizeHandler = LegalizationResult (*)(LegalizeCtx &ctx);
std::pmr::vector<LegalizeHandler> m_handlers;
```

---

## 6. Worklist-Driven Legalization Algorithm

The new legalization loop completely abandons the block-restart pattern. It uses an explicit worklist, tracks newly inserted instructions, and detects cycles.

```cpp
bool MirLegalizer::legalizeBlock(MirBlock *block)
{
    if (!block) return false;

    // 1. Initialize worklist with all instructions in the block in reverse post-order
    std::pmr::vector<MirInstruction *> worklist(m_ctx->getAllocator());
    worklist.reserve(block->getInstructions().size());
    for (MirInstruction *inst : block->getInstructions())
    {
        worklist.push_back(inst);
    }

    // 2. Cycle detection budget (proportional to block size)
    const size_t maxSteps = worklist.size() * 32 + 256;
    size_t stepsTaken = 0;

    while (!worklist.empty())
    {
        if (++stepsTaken > maxSteps)
        {
            m_ctx->getDiagCollector()->error(
                "MirLegalizer", 
                "Infinite legalization cycle detected in block '{}'", 
                block->getName());
            return false;
        }

        MirInstruction *inst = worklist.back();
        worklist.pop_back();

        // Skip instructions erased by prior lowering actions
        if (!inst || inst->isErased()) continue;

        // 3. Formulate Legality Query
        LegalityQuery query = buildQuery(inst);

        // 4. Query LegalizerInfo
        LegalityResponse response = m_targetDesc->getLegalizerInfo()->query(query);

        // Fast path: instruction is already legal
        if (response.isLegal()) continue;

        if (response.isUnsupported())
        {
            m_ctx->getDiagCollector()->error(
                "MirLegalizer",
                "Unsupported instruction '{}' with operand type(s)",
                inst->getOpCodeName()) << inst->getSourceRef();
            return false;
        }

        // 5. Track insertion of new instructions during transformation
        InsertionTracker tracker(block, inst);
        LegalizeCtx ctx(m_ctx, m_targetDesc, tracker.getIterator());

        LegalizationResult result = executeAction(response, ctx, inst);
        if (result == LegalizationResult::Failed)
        {
            return false;
        }

        // 6. Push newly synthesized instructions onto worklist for recursive verification
        for (MirInstruction *newInst : tracker.getProducedInstructions())
        {
            worklist.push_back(newInst);
        }
    }

    return true;
}
```

### Key Advantages
1. **$O(N)$ Linear-Time Execution**: Instructions already legalized are never re-scanned.
2. **Deterministic Termination**: The cycle counter guarantees the compiler will never hang on malformed rule cycles (e.g. `i8` widens to `i16` while `i16` narrows to `i8`).
3. **PMR Memory Efficiency**: Uses the function's PMR monotonic memory arena for the worklist vector.

---

## 7. Developer Convenience via EzDSL (`.lad` Specification)

### 7.1 Proposed High-Level `.lad` Syntax

We extend the EzDSL Legalize Action Language (`.lad`) to provide an ergonomic syntax for backend developers:

```dsl
// ============================================================================
// EzDSL Target Legality Specification: AMD64.lad
// ============================================================================

target AMD64;

// 1. Reusable Type Aliases & Sets
type_set GPR_SCALARS = (i8, i16, i32, i64);
type_set FPR_SCALARS = (f32, f64);
type_set ALL_NATIVE  = (i8, i16, i32, i64, f32, f64, ptr);

// 2. Clamping Shorthand (Dev Convenience Macro)
// Automatically widens anything smaller than i32 to i32,
// marks i32 & i64 as LEGAL, and narrows anything larger than i64 to i64.
action ADD {
    CLAMP_SCALAR(i32, i64);
};

// 3. Instruction Grouping
// Apply identical legality rules to multiple opcodes at once
group IntegerALU = (SUB, AND, OR, XOR) {
    CLAMP_SCALAR(i32, i64);
};

// 4. Libcall Lowering
action SDIV {
    LEGAL(i32, i64);
    WIDENS(i8, i16)   >> i32;
    LIBCALL(i128)     >> "__divti3";
};

action UDIV {
    LEGAL(i32, i64);
    WIDENS(i8, i16)   >> i32;
    LIBCALL(i128)     >> "__udivti3";
};

// 5. Heterogeneous Multi-Slot Declarations
action SEXT {
    LEGAL(i32:0, i8:1);
    LEGAL(i32:0, i16:1);
    LEGAL(i64:0, i8:1);
    LEGAL(i64:0, i16:1);
    LEGAL(i64:0, i32:1);
    WIDENS(i1:1) >> i8; // If source is i1, widen source to i8 first
};

action STORE {
    LEGAL(GPR_SCALARS:0, ptr:1);
    LEGAL(FPR_SCALARS:0, ptr:1);
    WIDENS(i1:0) >> i8;
};

action LOAD {
    LEGAL(GPR_SCALARS:0, ptr:1);
    LEGAL(FPR_SCALARS:0, ptr:1);
};

// 6. First-Class Call, Return, and ABI Lowering (No hardcoded C++ branches!)
action CALL {
    LOWER >> AMD64CallLowering;
};

action RET {
    LOWER >> AMD64ReturnLowering;
};

action ALLOC {
    LOWER >> AMD64FrameAllocLowering;
};
```

### 7.2 AST Node Additions (`EzDsl/Lexer/include/Ast/LegalizeActionDefLangAst.h`)

```cpp
namespace DSL::Ast::LegalizeActionDef
{

enum class LegalizeActionKind : uint8_t
{
    Legal,
    WidenScalar,
    NarrowScalar,
    Bitcast,
    Libcall,
    Lower,        // NEW: Standard procedural lowering (CALL, RET, ALLOC)
    Custom,       // Target custom C++ / .lrd rule
    Unsupported
};

struct ClampScalarClause
{
    Common::Identifier m_minType;  // Lower bound type (e.g. i32)
    Common::Identifier m_maxType;  // Upper bound type (e.g. i64)
};

struct TypeSetDecl
{
    Common::Identifier m_name;
    std::pmr::vector<Common::Identifier> m_types;
};

struct InstructionGroupDecl
{
    Common::Identifier m_groupName;
    std::pmr::vector<Common::Identifier> m_instructions;
    std::pmr::vector<LegalizeActionClause> m_actionClauses;
    std::optional<ClampScalarClause> m_clampClause;
};

} // namespace DSL::Ast::LegalizeActionDef
```

---

## 8. C++ Developer Experience: Fluent `LegalizerInfo` API

For unit testing, mock backends, and handwritten target plugins, we introduce a fluent builder pattern modeled after modern compiler standards. This replaces the manual switch functions in `MockTargetDesc`:

```cpp
class MockTargetLegalizerInfo : public LegalizerInfo
{
  public:
    MockTargetLegalizerInfo(MirTypeTable *tt)
    {
        auto *i1 = tt->i1();
        auto *i8 = tt->i8();
        auto *i16 = tt->i16();
        auto *i32 = tt->i32();
        auto *i64 = tt->i64();
        auto *i128 = tt->i128();
        auto *f32 = tt->f32();
        auto *f64 = tt->f64();
        auto *ptr = tt->getPtr(i8);

        // 1. ALU Operations with Clamping
        getActionDefinitions({ MirInstructionOpCode::ADD,
                               MirInstructionOpCode::SUB,
                               MirInstructionOpCode::XOR,
                               MirInstructionOpCode::AND,
                               MirInstructionOpCode::OR })
            .legalFor({ i32, i64 })
            .widenScalarTo(0, { i1, i8, i16 }, i32)
            .narrowScalarTo(0, { i128 }, i64);

        // 2. Division & Libcalls
        getActionDefinitions({ MirInstructionOpCode::DIV, MirInstructionOpCode::IDIV })
            .legalFor({ i32 })
            .widenScalarTo(0, { i1, i8, i16 }, i32)
            .libcallFor(i64, "__divdi3")
            .narrowScalarTo(0, { i128 }, i64);

        // 3. Comparisons
        getActionDefinitions({ MirInstructionOpCode::CMP_EQ, MirInstructionOpCode::CMP_NE })
            .legalForTypesWithSource({ i32, i64, f32, f64 })
            .widenScalarSourceTo({ i1, i8, i16 }, i32)
            .narrowScalarSourceTo({ i128 }, i64);

        // 4. Extension Operations
        getActionDefinitions(MirInstructionOpCode::SEXT)
            .legalFor({ { i32, i8 }, { i32, i16 }, { i64, i32 } })
            .widenScalarTo(1, { i1 }, i8);

        // 5. Data Movement & Bitcasts
        getActionDefinitions(MirInstructionOpCode::MOV)
            .legalIfSameType()
            .bitcastBetween(i32, f32);

        // 6. High-Level Procedural Lowerings (Replaces hardcoded checks!)
        getActionDefinitions(MirInstructionOpCode::CALL)
            .lowerWith(&LegalizeActions::LegalizeCall);

        getActionDefinitions(MirInstructionOpCode::RET)
            .lowerWith(&LegalizeActions::LegalizeReturn);

        // 7. ABI Token Lowering Primitives
        getActionDefinitions({ MirInstructionOpCode::POP_ARG, MirInstructionOpCode::PUSH_RET })
            .legalFor({ tt->__bindToken() });

        getActionDefinitions(MirInstructionOpCode::END_ARG)
            .legalFor({ tt->__bindToken() });
    }
};
```

---

## 9. Code Generator Design: `CppLegalizerGenerator`

To bridge EzDSL and EzTriple, we introduce [`EzDsl/CodeGenerators/src/CodeGenerators/CppLegalizerGenerator.cpp`](file:///E:/Repos/EzPacker/EzDsl/CodeGenerators/src/CodeGenerators/):

### Generated Artifacts
For target `AMD64`:
- `build/generated/AMD64/AMD64LegalizerActionTable.h`
- `build/generated/AMD64/AMD64LegalizerActionTable.cpp`

### Generator Responsibilities
1. **Unroll Groups & Shorthands**: Expands `group` declarations and `CLAMP_SCALAR` into concrete per-opcode action entries.
2. **Dense Array Synthesis**: Emits the $O(1)$ fast lookup array:
   ```cpp
   static const LegalityResponse g_AMD64_PrimaryMatrix[OPCODE_COUNT][MAX_COMPACT_TYPES] = { ... };
   ```
3. **Heterogeneous Signature Functions**: Synthesizes small, branch-free decision functions for opcodes with multi-slot constraints (`SEXT`, `STORE`, `LOAD`).
4. **Target Lowering Dispatcher**: Emits the table of lowering callbacks:
   ```cpp
   LegalizationResult AMD64LegalizerInfo::executeCustom(LegalizeCtx &ctx, uint16_t handlerId)
   {
       switch (handlerId) {
           case 0: return LegalizeActions::LegalizeCall(ctx);
           case 1: return LegalizeActions::LegalizeReturn(ctx);
           default: return LegalizationResult::Failed;
       }
   }
   ```
5. **Class Synthesis**: Generates concrete class `<Target>LegalizerInfo : public LegalizerInfo`.

---

## 10. Step-by-Step Implementation Roadmap

```
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                 Implementation Phase Timeline                                    │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘

 Phase 1: Core EzTriple Interfaces
 ├─ Define LegalizeActionKind (Lower, Custom, etc.)
 ├─ Create LegalityQuery & LegalityResponse structs
 ├─ Implement LegalizerInfo base class & fluent C++ builder
 └─ Implement InsertionTracker for instruction streams
                                      │
                                      ▼
 Phase 2: Worklist Engine & Decoupled Actions
 ├─ Rewrite MirLegalizer::legalizeBlock to use worklists
 ├─ Remove hardcoded IsCall / IsReturn if checks from legalizeInstruction
 ├─ Route CALL and RET through LegalizerInfo::query
 ├─ Refactor LegalizeNarrowScalarAction into modular operation dispatch
 └─ Update MockTargetDesc to use LegalizerInfo fluent builder
                                      │
                                      ▼
 Phase 3: EzDSL Language & Parser Enhancements
 ├─ Update LegalizeActionDefLangAst.h (type_set, group, CLAMP_SCALAR, LOWER)
 ├─ Extend LegalizeActionDefLang.h Lexy parsers
 ├─ Update LegalizeActionPass.cpp semantic validator
 └─ Add full unit test coverage in tests/EzDslLexerTestSuite/
                                      │
                                      ▼
 Phase 4: EzDSL C++ Code Generator (CppLegalizerGenerator)
 ├─ Implement CppLegalizerGenerator header and implementation
 ├─ Synthesize <Target>LegalizerActionTable.h/.cpp
 ├─ Hook generator into EzDsl-cli driver (Main.cpp)
 └─ Integrate into CMake via EzDslGenBackend.cmake
                                      │
                                      ▼
 Phase 5: Verification & Hardening
 ├─ Update tests/EzTripleTestSuite/tests/T_MirLegalizer.cpp
 ├─ Add stress tests for cycle detection & worklist scaling
 ├─ Verify full pipeline integration with InstructionSelector and FrameLowerer
 └─ Run graphify update . to sync repository knowledge graph
```

### Detailed Phase Tasks

#### Phase 1: Core EzTriple Interfaces
- [ ] Create `EzTriple/include/Legalizer/LegalityQuery.h`:
  - Define `LegalityQuery`, `LegalityResponse`, `LegalizeActionKind`.
- [ ] Create `EzTriple/include/Legalizer/LegalizerInfo.h`:
  - Implement base `LegalizerInfo` class with 3-tier lookup (`query`, `queryFast`, `executeCustom`).
  - Implement fluent helper `ActionDefinitionBuilder`.
- [ ] Create `EzTriple/include/Legalizer/InsertionTracker.h`:
  - Safe helper that tracks newly created or erased instructions around an iterator.

#### Phase 2: Worklist Engine & Decoupled Actions
- [ ] Refactor `EzTriple/src/Legalizer/MirLegalizer.cpp`:
  - Replace the 32-pass restart loop with the worklist queue.
  - Delete `if (inst->getFlags() & MirInstructionFlags::IsCall)` and `IsReturn`.
  - Wire `legalizeInstruction` exclusively through `m_targetDesc->getLegalizerInfo()->query(query)`.
- [ ] Update `EzTriple/include/Descriptors/TargetDesc.h`:
  - Add `virtual LegalizerInfo *getLegalizerInfo() = 0;`.
  - Maintain backward compatibility by keeping `getLegalizeActionTable()` as a fallback during transition.
- [ ] Refactor `MockTargetDesc` in `tests/EzTripleTestSuite/include/EzTripleTestSuite.h`:
  - Replace 200 lines of manual `queryAlu`/`queryDiv` functions with a clean `MockTargetLegalizerInfo`.
- [ ] Verify that existing tests in `tests/EzTripleTestSuite/tests/T_MirLegalizer.cpp` pass with 100% success.

#### Phase 3: EzDSL Language & Parser Enhancements
- [ ] Update AST in `EzDsl/Lexer/include/Ast/LegalizeActionDefLangAst.h`:
  - Add `LegalizeActionKind::Lower`.
  - Add `TypeSetDecl`, `InstructionGroupDecl`, and `ClampScalarClause`.
- [ ] Update Lexy grammar in `EzDsl/Lexer/include/Parser/LegalizeActionDefLang.h`:
  - Add parser rules for `type_set`, `group`, `CLAMP_SCALAR`, and `LOWER`.
- [ ] Update `EzDsl/Sema/src/SemaPasses/LegalizeActionPass.cpp`:
  - Resolve type sets and instruction groups into individual instruction symbols.
  - Validate clamp ranges (`minType.bitWidth <= maxType.bitWidth`).
- [ ] Add unit tests in `tests/EzDslLexerTestSuite/tests/T_LegalizeActionDefLang.cpp`:
  - Test grouping syntax, clamping, and `LOWER` action parsing.

#### Phase 4: EzDSL C++ Code Generator (`CppLegalizerGenerator`)
- [ ] Create `EzDsl/CodeGenerators/include/CodeGenerators/CppLegalizerGenerator.h`:
  - Inherit from `CodeGenerator`.
- [ ] Implement `EzDsl/CodeGenerators/src/CodeGenerators/CppLegalizerGenerator.cpp`:
  - Generate `<Target>LegalizerActionTable.h` and `.cpp`.
  - Synthesize the dense 2D lookup array and rule functions.
- [ ] Wire `CppLegalizerGenerator` into `EzDsl/Cli/src/Cli/Driver.cpp`.
- [ ] Add generator test `tests/EzDslTestSuite/tests/T_EzDslCli_GenLegalizer.cpp`.

#### Phase 5: Verification & Hardening
- [ ] Run full test suite across EzMir, EzDsl, and EzTriple.
- [ ] Add unit tests verifying:
  - Detection and diagnostics of infinite rule cycles.
  - Correct lowering of `CALL` and `RET` via `LegalizerInfo`.
  - Proper narrowing of multi-word arithmetic and comparisons.
- [ ] Update `graphify` knowledge graph via `graphify update .`.

---

## 11. Risk Analysis & Mitigation

| Risk / Challenge | Impact | Mitigation Strategy |
|:---|:---|:---|
| **Breaking Existing Unit Tests** | High | Keep `LegalizeActions::LegalizeCall` and `LegalizeReturn` as standard action routines; wire them through `MockTargetLegalizerInfo` so all current test expectations remain bit-identical. |
| **Lookup Performance Degradation** | Medium | The Tier 1 dense matrix ensures $O(1)$ constant-time resolution without hash collisions or pointer chasing. Benchmarking will be included in the test suite. |
| **Table Binary Size Bloat** | Low | A dense matrix of 150 opcodes $\times$ 32 compact types storing a 4-byte struct is only 19.2 KB per target—well within modern CPU L1 data cache limits. |
| **Infinite Legalization Cycles** | Medium | The worklist engine enforces a deterministic `maxSteps = worklist.size() * 32 + 256` step budget with immediate diagnostic reporting including instruction source refs. |
