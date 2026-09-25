# EzTriple Subproject Documentation

[EzPacker Documentation Index](../index.md) > [Subprojects](EzTriple.md) > **EzTriple** | [Doxygen API Reference](../doxygen/index.html)

---

## 1. Overview & Architectural Role

`EzTriple` is the target-independent machine lowering engine of EzPacker. It sits between middle-end SSA MIR and the final binary code emitter. EzTriple executes the critical five-stage lowering pipeline that transforms abstract, target-agnostic MIR into concrete, hardware-mapped machine instructions:

1. **Legalization**: Decomposes unsupported types and opcodes via table-driven rewrite rules.
2. **ABI Lowering**: Translates procedural parameter and return tokens into hardware ABI calling convention registers and stack slots.
3. **Instruction Selection**: Replaces generic operations with target hardware instructions via Bottom-Up Maximal Munch pattern matching.
4. **Register Allocation**: Maps unbounded virtual registers to finite physical hardware registers using a Chaitin-Briggs graph-coloring algorithm.
5. **Frame Lowering**: Calculates stack frame layouts, replaces abstract frame objects with base-pointer/stack-pointer displacements, and emits target function prologues and epilogues (PEI).

```
+-------------------------------------------------------------------------------+
|                            EzTriple Pipeline Passes                           |
|                                                                               |
|  Generic SSA MIR                                                              |
|        |                                                                      |
|        v                                                                      |
|  +-------------------------------------+                                      |
|  |     MirLegalizerPass                | <-- LegalizerInfo (3-tier matrix)    |
|  | - WidenScalar, NarrowScalar         |                                      |
|  | - Lower, Libcall, Custom Handlers   |                                      |
|  +-------------------------------------+                                      |
|        |                                                                      |
|        v                                                                      |
|  +-------------------------------------+                                      |
|  |     MirAbiLowererPass               | <-- CallingConvDesc (SysV, Win64...) |
|  | - Lowers PUSH_ARG, POP_ARG, CALL    |                                      |
|  | - Lowers PUSH_RET, POP_RET, RET     |                                      |
|  +-------------------------------------+                                      |
|        |                                                                      |
|        v                                                                      |
|  +-------------------------------------+                                      |
|  |  MirInstructionSelectorPass         | <-- Pattern Decision Tree (.isf)     |
|  | - Bottom-Up Maximal Munch           |                                      |
|  | - AddressingModeMatcher & Folding   |                                      |
|  +-------------------------------------+                                      |
|        |                                                                      |
|        v                                                                      |
|  +-------------------------------------+                                      |
|  |  MirRegisterAllocatorPass           | <-- Register Classes & Banks (.tdesc)|
|  | - Chaitin-Briggs Graph Coloring     |                                      |
|  | - Interference Graph & Spilling     |                                      |
|  +-------------------------------------+                                      |
|        |                                                                      |
|        v                                                                      |
|  +-------------------------------------+                                      |
|  |     MirFrameLowererPass             | <-- Target Stack Frame Layout        |
|  | - Prologue / Epilogue Insertion     |                                      |
|  | - ALLOC / DALLOC Lowering           |                                      |
|  +-------------------------------------+                                      |
|        |                                                                      |
|        v                                                                      |
|  Lowered Machine Instructions (Ready for EzCodeEmitter)                       |
+-------------------------------------------------------------------------------+
```

---

## 2. The 5 Core Subsystems

### 2.1 The Legalizer (`include/Legalizer/`)

Hardware architectures do not natively support all possible integer bit-widths, vector dimensions, or high-level operations. The Legalizer transforms illegal operations into sequences of legal instructions.

#### Legalization Action Kinds (`LegalizeQuery.h`)
```cpp
enum class LegalizeActionKind : uint8_t
{
    Legal = 0,    // Natively supported by hardware.
    WidenScalar,  // Promote to a wider legal type (e.g. i8 -> i32).
    NarrowScalar, // Split into smaller legal types (e.g. i128 -> 2x i64).
    Bitcast,      // Reinterpret bit pattern without conversion (e.g. f32 -> i32).
    Libcall,      // Lower into runtime helper call (e.g. __divdi3).
    Lower,        // Decompose into standard target MIR primitives.
    Custom,       // Target-defined C++ callback.
    Unsupported   // Rejected combination; emits a diagnostic error.
};
```

#### Legality Query & Response
Every instruction is queried against `LegalizerInfo` using exact descriptor structs:

```cpp
struct LegalityQuery
{
    MirInstructionOpCode m_opcode{ MirInstructionOpCode::INVALID };
    uint32_t m_flags{ 0 };
    size_t m_operandCount{ 0 };
    std::array<MirType *, 6> m_types{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
    std::array<uint8_t, 6> m_compactIds{ 0, 0, 0, 0, 0, 0 };
    std::array<ExpectedOperandType, 6> m_operandKinds{ ExpectedOperandType::None, ... };
    int64_t m_immValue{ 0 };
    bool m_hasImm{ false };
};

struct alignas(8) LegalityResponse
{
    LegalizeActionKind m_action{ LegalizeActionKind::Unsupported };
    uint8_t m_slot{ 0 };               // Target operand slot (0, 1, 2)
    uint8_t m_targetCompactId{ 0 };    // Destination compact type ID
    uint16_t m_handlerOrStringId{ 0 }; // Libcall symbol ID or custom callback index

    constexpr bool isLegal() const noexcept { return m_action == LegalizeActionKind::Legal; }
    constexpr bool isUnsupported() const noexcept { return m_action == LegalizeActionKind::Unsupported; }
};
```

#### 3-Tier Architecture
1. **Tier 1 (Dense Primary Matrix)**: An $O(1)$ lookup table indexing `[Opcode][CompactTypeID]` returning an immediate `LegalityResponse`.
2. **Tier 2 (Signature Matchers)**: Evaluates multi-operand operations with mismatched types (e.g., truncated loads, zero/sign extensions, conversions).
3. **Tier 3 (Declarative Rewrite Rules)**: Evaluates algebraic rules and strength-reduction rewrites generated from `.lrd` specifications.

---

### 2.2 ABI Lowerer (`include/AbiLowerer/`)

The ABI lowerer translates abstract procedural calls and argument passes into concrete machine registers and stack slots governed by `CallingConvDesc`:

- **Argument Token Binding**:
  - Replaces `PUSH_ARG` with moves into target argument registers (`rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9` on System V; `rcx`, `rdx`, `r8`, `r9` on Windows) or stack spill stores.
  - Replaces `CALL` with target machine calls (`CALL64r`, `CALL64m`, `CALL64p`).
  - Replaces `POP_RET` with moves out of target return registers (`rax`, `rdx`, `xmm0`).
- **Function Entry/Exit**:
  - Replaces `POP_ARG` + `END_ARG` at the entry block with moves from physical parameter registers into virtual registers.
  - Replaces `PUSH_RET` + `RET` with assignments to the target return registers followed by machine return instructions (`RET`).
- **Shadow Space & Red Zone**:
  - Allocates 32 bytes of shadow space (homing space) above the return address for Win64 ABI calls.
  - Accounts for the 128-byte System V AMD64 Red Zone under `%rsp`.

---

### 2.3 Instruction Selector (`include/InstructionSelector/`)

Transforms generic SSA opcodes into concrete machine instruction descriptors:

- **Bottom-Up Maximal Munch**:
  Traverses the instruction stream in reverse topological order within each basic block, matching the largest possible expression trees against pattern decision trees generated from `.isf` files.
- **Addressing Mode Matcher (`MirAddressingModeMatcher.h`)**:
  Synthesizes complex addressing expressions into memory operands:
  ```text
  Effective Address = Base + (Index * Scale) + Displacement
  ```
- **Load-Folding Optimization**:
  Automatically folds memory loads into consuming ALU instructions (e.g., `ADD %dst, %src, (LOAD %addr)` -> `ADD64rm %dst, %src, %addr`) when the loaded value has a single user and no intervening memory store clobbers the address.

---

### 2.4 Register Allocator (`include/RegisterAllocator/`)

EzTriple implements a production-grade **Chaitin-Briggs Graph-Coloring Register Allocator** (`MirRegisterAllocator.h`):

```
      +--------------------------------------------------+
      |        Build Interference Graph (m_iGraph)       |
      |   (Nodes = VRegs, Edges = Concurrent Liveness)   |
      +--------------------------------------------------+
                                |
                                v
      +--------------------------------------------------+
      |       Calculate Spill Costs (m_spillCosts)       |
      |          (Loop depth & usage frequency)          |
      +--------------------------------------------------+
                                |
                                v
                 +----------------------------+
                 |  Can simplify node (deg < K)?
                 +----------------------------+
                   /                        \
           [Yes]  /                          \  [No]
                 v                            v
      +----------------------+     +----------------------+
      | Simplify: Push node  |     | Optimistic Spill:    |
      | to select stack      |     | Push lowest cost node|
      +----------------------+     +----------------------+
                 \                            /
                  \                          /
                   v                        v
             +------------------------------------+
             | Are all virtual registers removed? |
             +------------------------------------+
                               | [Yes]
                               v
      +--------------------------------------------------+
      | Select: Pop nodes from stack in reverse order    |
      | and assign valid physical color from class       |
      +--------------------------------------------------+
                               |
              +----------------+----------------+
              |                                 |
              v [Success]                       v [Spill Occurred]
     +-------------------+            +--------------------+
     |   RewriteColors   |            | Insert Spills/     |
     | (VRegs -> PRegs)  |            | Reloads & Repeat   |
     +-------------------+            +--------------------+
```

#### Working Context (`RegisterAllocatorCtx`)
```cpp
struct RegisterAllocatorCtx
{
    MirBuilderContext *m_ctx;
    MirFunction *m_targetFunction;
    TargetDesc *m_targetDesc;
    std::pmr::memory_resource *m_allocator;

    std::pmr::vector<MirRegisterRef> m_selectStack;
    std::pmr::unordered_set<MirRegisterRef> m_removedNodes;
    std::pmr::unordered_set<MirRegisterRef> m_reservedRegs;
    std::pmr::unordered_map<MirRegisterRef, MirRegisterRef> m_allocatedRegs;
    std::pmr::unordered_map<MirRegisterRef, size_t> m_degree;
    std::pmr::unordered_map<MirRegisterRef, StackFrameObject *> m_spilledRegs;
    std::pmr::unordered_map<MirRegisterRef, std::pmr::set<MirRegisterRef>> m_iGraph;
    std::pmr::unordered_set<MirRegisterRef> m_unspillableRegs;
    std::pmr::unordered_map<MirRegisterRef, double> m_spillCosts;
    bool m_spillCostsValid{ false };
};
```

#### Abstract Allocator Class (`MirRegisterAllocator`)
```cpp
class MirRegisterAllocator
{
public:
    virtual ~MirRegisterAllocator() = default;

    bool buildInterferenceGraph(LivenessResult *liveness, RegisterAllocatorCtx *ctx);
    void evaluateInterferenceGraphDegree(RegisterAllocatorCtx *ctx);
    bool simplify(RegisterAllocatorCtx *ctx);
    bool selectColors(RegisterAllocatorCtx *ctx);
    void rewriteColors(RegisterAllocatorCtx *ctx);

protected:
    double calculateSpillCost(MirRegisterRef node, RegisterAllocatorCtx *ctx);
    void rewriteSpilledRegisters(const std::pmr::unordered_set<MirRegisterRef> &spilledNodes, RegisterAllocatorCtx *ctx);

    // Target-specific pure virtual hooks
    virtual bool isInstructionDAlloc(MirInstruction *instr) = 0;
    virtual bool isRematerializable(MirRegister *vreg, MirInstruction *definingInst) = 0;
    virtual MirInstruction *emitReload(RegisterAllocatorCtx *ctx,
                                       MirBlock *block,
                                       IntrusiveLinkedList<MirInstruction>::iterator it,
                                       SourceReference *srcRef,
                                       MirRegister *dstReg,
                                       StackFrameObject *spillSlot) = 0;
    virtual MirInstruction *emitSpill(RegisterAllocatorCtx *ctx,
                                      MirBlock *block,
                                      IntrusiveLinkedList<MirInstruction>::iterator it,
                                      SourceReference *srcRef,
                                      StackFrameObject *spillSlot,
                                      MirRegister *srcReg) = 0;
    virtual MirInstruction *reMaterialize(RegisterAllocatorCtx *ctx,
                                          MirBlock *block,
                                          IntrusiveLinkedList<MirInstruction>::iterator it,
                                          SourceReference *srcRef,
                                          MirRegister *dstReg,
                                          MirInstruction *defInst) = 0;
};
```

---

### 2.5 Frame Lowerer (`include/FrameLowerer/`)

The Frame Lowerer executes Prologue/Epilogue Insertion (PEI) and resolves abstract stack offsets:

```cpp
struct FrameLowererCtx
{
    MirBuilderContext *m_ctx;
    MirFunction *m_targetFunc;
    TargetDesc *m_targetDesc;
    IntrusiveLinkedList<MirInstruction>::iterator m_allocIt;
};

class MirFrameLowerer
{
public:
    virtual ~MirFrameLowerer() = default;

    virtual void calculateFrameLayout(FrameLowererCtx &ctx);
    virtual void insertPrologue(FrameLowererCtx &ctx) = 0;
    virtual void insertEpilogue(FrameLowererCtx &ctx) = 0;
    virtual bool lowerAlloc(FrameLowererCtx &ctx) = 0;
    virtual bool lowerDAlloc(FrameLowererCtx &ctx) = 0;
    virtual void lowerStackObjectReferences(FrameLowererCtx &ctx);
};
```

#### Lowering Sequence
1. **`lowerAlloc`**: Lowers static stack allocations (`ALLOC`) into stack frame object slots.
2. **`lowerDAlloc`**: Lowers dynamic stack allocations (`DALLOC`) by emitting stack pointer decrements (`sub rsp, size`), alignment masks, and forcing frame pointer generation.
3. **`calculateFrameLayout`**: Computes cumulative stack size, pads to satisfy target alignment (e.g. 16 bytes), and assigns concrete base-pointer or stack-pointer offsets to each `StackFrameObject`.
4. **`lowerStackObjectReferences`**: Replaces abstract stack object references with concrete `MirMemory` operands `[rbp - offset]` or `[rsp + offset]`.
5. **`insertPrologue`**: Emits target instructions setting up the frame pointer (`push rbp; mov rbp, rsp`), allocating stack space (`sub rsp, FrameSize`), and preserving callee-saved registers.
6. **`insertEpilogue`**: Emits target instructions restoring callee-saved registers, collapsing the stack frame (`mov rsp, rbp; pop rbp` or `add rsp, FrameSize`), and emitting the machine return (`ret`).

---

## 3. Target Descriptors (`include/Descriptors/`)

### 3.1 `TargetDesc` (`Descriptors/TargetDesc.h`)

The abstract CPU architecture descriptor:
- `virtual const char *getName() const = 0`
- `virtual MirFrameLowerer *getFrameLowerer() = 0`
- `virtual MirInstructionSelector *getInstructionSelector() = 0`
- `virtual MirAddressingModeMatcher *getAddressingModeMatcher()`
- `virtual MirRegisterClass *getGprClass()`
- `virtual MirLegalizer *getLegalizer() = 0`
- `virtual LegalizerInfo *getLegalizerInfo() = 0`
- `virtual MirRegisterAllocator *getRegisterAllocator() = 0`
- `virtual MirType *getMemOperandDisplacementType() = 0`
- `virtual MirRegisterRef getInstructionPtrReg() const = 0`
- `virtual size_t getStackSlotSize() const = 0`
- `virtual void initialize() = 0`
- `virtual std::string_view getLibcallStr(uint8_t symId) = 0`
- `virtual const std::pmr::vector<TargetBinaryDesc *> &getAvailableBinaryDescriptors() = 0`
- `virtual const std::pmr::vector<CallingConvDesc *> &getAvailableCallingConventions() = 0`
- `virtual std::unique_ptr<GenericCodeEmitter> createCodeEmitter() = 0`

### 3.2 `TargetBinaryDesc` (`Descriptors/TargetBinaryDesc.h`)

The abstract OS and object-file format descriptor:
- `virtual bool isLittleEndian() const = 0`
- `virtual bool isPositionIndependent() const = 0`
- `virtual const char *getName() const = 0`
- `virtual CodeSection *getSection(SectionType type) = 0`
- `virtual TargetObjectFormat getObjectFormat() const = 0` (`ELF`, `COFF`, `MachO`)
- `virtual size_t getFunctionAlignment() const = 0`
- `virtual void initialize() = 0`
- `virtual const std::pmr::unordered_map<SectionType, CodeSection *> &getSections() const = 0`

---

## 4. Header & Class Index

| Component | Header Location | Key Classes / Structs |
|---|---|---|
| Legalizer | `EzTriple/include/Legalizer/LegalityQuery.h` | `LegalizeActionKind`, `LegalityQuery`, `LegalityResponse` |
| Legalizer | `EzTriple/include/Legalizer/LegalizerInfo.h` | `LegalizerInfo` |
| Legalizer | `EzTriple/include/Legalizer/MirLegalizer.h` | `MirLegalizer` |
| ABI Lowerer | `EzTriple/include/AbiLowerer/MirAbiLowerer.h` | `MirAbiLowerer` |
| Instruction Selector | `EzTriple/include/InstructionSelector/MirInstructionSelector.h` | `MirInstructionSelector` |
| Instruction Selector | `EzTriple/include/InstructionSelector/MirAddressingModeMatcher.h` | `MirAddressingModeMatcher` |
| Register Allocator | `EzTriple/include/RegisterAllocator/MirRegisterAllocator.h` | `MirRegisterAllocator`, `RegisterAllocatorCtx` |
| Frame Lowerer | `EzTriple/include/FrameLowerer/MirFrameLowerer.h` | `MirFrameLowerer`, `FrameLowererCtx` |
| Descriptors | `EzTriple/include/Descriptors/TargetDesc.h` | `TargetDesc` |
| Descriptors | `EzTriple/include/Descriptors/TargetBinaryDesc.h` | `TargetBinaryDesc`, `TargetObjectFormat` |
| Descriptors | `EzTriple/include/Descriptors/TargetRelocationResolver.h` | `TargetRelocationResolver` |
| Passes | `EzTriple/include/Passes/MirLegalizerPass.h` | `MirLegalizerPass` |
| Passes | `EzTriple/include/Passes/MirAbiLowererPass.h` | `MirAbiLowererPass` |
| Passes | `EzTriple/include/Passes/MirInstructionSelectorPass.h` | `MirInstructionSelectorPass` |
| Passes | `EzTriple/include/Passes/MirRegisterAllocatorPass.h` | `MirRegisterAllocatorPass` |
| Passes | `EzTriple/include/Passes/MirFrameLowererPass.h` | `MirFrameLowererPass` |
