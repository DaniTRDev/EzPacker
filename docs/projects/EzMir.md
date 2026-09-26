# EzMir Subproject Documentation

[EzPacker Documentation Index](../index.md) > [Subprojects](EzMir.md) > **EzMir** | [Doxygen API Reference](../doxygen/index.html)

---

## 1. Overview & Architectural Role

`EzMir` (Machine Intermediate Representation) is the core representation and middle-end analysis layer of EzPacker. It models functions, basic blocks, instructions, operands, and types in both high-level generic Static Single Assignment (SSA) form and low-level target-lowered machine instruction form.

EzMir serves as the universal pivot of the entire compiler:
1. **Frontends** (or the built-in textual `.mir` parser) construct an in-memory `MirFunction` graph.
2. **Middle-End Passes** analyze control flow, construct SSA, and compute register liveness.
3. **EzTriple Lowering Engine** legalizes opcodes/types, binds ABI calling conventions, selects machine instructions, allocates hardware registers, and lowers stack frames.
4. **EzCodeEmitter** reads lowered `MirInstruction` sequences to produce relocatable object bytes.

```
       +-------------------------------------------------------------+
       |                      Frontend / Source                      |
       +-------------------------------------------------------------+
                                       |
                                       v
                    +-------------------------------------+
                    |       MirLexer & MirParser          |
                    +-------------------------------------+
                                       |
                                       v
+-------------------------------------------------------------------------------+
|                                  EzMir Module                                 |
|                                                                               |
|  MirFunction                                                                  |
|   +-- MirFunctionStackFrame (stack objects, spill slots, alignments)          |
|   +-- MirFunctionRegisterInfo (virtual registers, physical register bindings) |
|   +-- MirBlock (Entry)                                                        |
|   |    +-- IntrusiveLinkedList<MirInstruction>                                |
|   |         +-- MirInstruction [MOV dst:%0, src:42]                          |
|   |              +-- MirOperand (VReg, PReg, Imm, Mem, Symbol, Label)         |
|   |              +-- MirType (i1..i256, f32..f128, ptr, vectors)              |
|   +-- MirBlock (Exit)                                                         |
+-------------------------------------------------------------------------------+
       |                                       |
       v                                       v
+-----------------------------+       +-----------------------------------------+
|     Middle-End Passes       |       |              MirPrinter                 |
| - CodeFlowAnalysisPass      |       | (Emits human-readable textual MIR)      |
| - NonSsaToSsaPass           |       +-----------------------------------------+
| - LivenessAnalysisPass      |
| - MirPeepholePass           |
+-----------------------------+
       |
       v
  To EzTriple Lowering Pipeline
```

---

## 2. In-Memory Intermediate Representation (IR)

### 2.1 `MirFunction` (`Function/MirFunction.h`)

The top-level entity representing a callable routine.
- **Name & Coordinates**: Identifier symbol (e.g., `@calculate_hash`) and associated `SourceReference`.
- **Calling Convention**: Pointer to `CallingConvDesc` specifying parameter placement, return registers, and preservation rules.
- **Linkage (`MirLinkage`)**: External visibility and binding (`MirLinkage::External`, `MirLinkage::Internal`, `MirLinkage::Weak`). Queried via `getLinkage()` and modified via `setLinkage()`.
- **Declarations vs. Definitions**:
  - `isDeclaration()`: True when the function has 0 basic blocks (e.g. extern C declarations such as `declare @puts(ptr) -> i32;`). External declarations are excluded from code optimization/lowering passes and emitted as undefined symbols (`SHN_UNDEF`).
  - `isDefinition()`: True when the function has 1 or more basic blocks containing executable code.
- **Return Type & Parameters**: Monomorphic return type (`MirType*`) and formal parameter list (`const std::pmr::list<MirRegister*>&`).
- **Basic Block Stream**: An intrusive sequence of `MirBlock` nodes stored via `IntrusiveLinkedList<MirBlock>`, with the head block serving as the function entry point.
- **`MirFunctionStackFrame`**: Manages all function stack allocations:
  - Local fixed stack objects (`StackFrameObject`).
  - Dynamic stack allocations (`DALLOC`).
  - Required stack alignment (e.g. 16-byte alignment on x86-64).
  - Spill slots allocated dynamically by the register allocator.
- **`MirFunctionRegisterInfo`**:
  - Virtual register tracker allocating unique IDs.
  - Register class constraints (`MirRegisterClass*`).
  - Physical register mapping assigned during register allocation.
  - Callee-saved register usage sets.

### 2.2 `MirGlobalVar` & Linkage (`GlobalVar/MirGlobalVar.h`, `Linkage/MirLinkage.h`)

Global variables represent statically allocated data objects:
- **Linkage**: Configured with `MirLinkage` (`External`, `Internal`, `Weak`). Controls whether the symbol is exported (`STB_GLOBAL` / `COFF_SYM_CLASS_EXTERNAL`), private to the translation unit (`STB_LOCAL` / `COFF_SYM_CLASS_STATIC`), or weak (`STB_WEAK`).
- **Immutability & Section Placement**: Immutable constants go to read-only memory (`.rodata`), zero-initialized or uninitialized variables go to `.bss`, and initialized mutable variables go to `.data`.
- **Initializers**: Multi-precision integer or floating-point literal operands.

### 2.3 `MirBlock` (`Block/MirBlock.h`)

A single-entry, single-exit basic block:
- **Instruction Container**: Holds an `IntrusiveLinkedList<MirInstruction>` providing zero-heap-allocation insertion, erasure, and iteration.
- **CFG Links**: Explicit predecessor and successor lists (`std::pmr::vector<MirBlock*>`).
- **Terminator**: The last instruction in a well-formed basic block must have terminator semantics (`RET`, `BR_COND`, `JMP`, or `UNREACHABLE`).
- **Phi Functions**: SSA `PHI` nodes positioned strictly at the beginning of the block prior to normal instructions.

### 2.3 `MirInstruction` (`Instruction/MirInstruction.h`)

Represents a single executable operation:
- **Opcode**:
  - **Generic Opcodes** (`MirInstructionOpCode` from `Instruction/MirInstructionSet.h`): `MOV`, `ADD`, `SUB`, `IMUL`, `SDIV`, `UDIV`, `AND`, `OR`, `XOR`, `SHL`, `LSHR`, `ASHR`, `LOAD`, `STORE`, `CMP_EQ`, `CMP_SLT`, `CMP_UGT`, `BR_COND`, `JMP`, `CALL`, `RET`, `PHI`, `ALLOC`, `DALLOC`, etc.
  - **Target Opcodes**: Bound via a pointer to `MirTargetInstructionDesc` synthesized by `EzDsl` (e.g., `ADD64rr`, `MOV32ri`, `VADDPSrr`).
- **Operands**: Array of polymorphic `MirOperand*` pointers (destinations and sources).
- **Flags (`MirInstructionFlags`)**:
  - `IsCommutative`: Operands 1 and 2 may be swapped.
  - `IsBranch` / `IsConditionalBranch` / `IsTerminator`: Control flow properties.
  - `IsCall` / `IsReturn`: Inter-procedural call/return boundaries.
  - `ReadsMemory` / `WritesMemory`: Memory side-effect and barrier tracking.
  - `HasSideEffect`: Inhibits dead-code elimination.
- **Intrusive Links**: Embedded `m_prev` and `m_next` pointers satisfying `IntrusiveLinkedList<MirInstruction>`.

### 2.4 `MirOperand` (`Operand/MirOperand.h`)

The polymorphic operand hierarchy representing instruction inputs and outputs:

| Operand Class | Header Location | Description |
|---|---|---|
| `MirRegister` | `Operand/MirRegister.h` | Virtual register (`%0`, `%sum`) or physical hardware register (`rax`, `xmm0`). |
| `MirInteger` | `Operand/MirInteger.h` | Multi-precision integer immediate holding a `FlexInt`. |
| `MirFloat` | `Operand/MirFloat.h` | Multi-precision floating-point immediate holding a `FlexFloat`. |
| `MirMemory` | `Operand/MirMemory.h` | Effective address: `[base + index*scale + displ]`. |
| `MirReference` | `Operand/MirReference.h` | Symbolic reference pointing to a `MirBlock`, `MirFunction`, `MirGlobalVar`, or `StackFrameObject`. |
| `MirRuntimeSymbol` | `Operand/MirRuntimeSymbol.h` | Named external runtime symbol (e.g., `@__divti3`). |

### 2.5 `MirType` & `MirTypeTable` (`Type/MirType.h`, `Type/MirTypeTable.h`)

EzPacker features a comprehensive type system capable of representing arbitrary scalar integers, IEEE floats, pointers, and SIMD vectors. Type instances are immutable and canonicalized (interned) in `MirTypeTable`:

- **Scalar Integers**: `i1` (bool), `i8`, `i16`, `i32`, `i64`, `i128`, `i256` (and arbitrary widths).
- **Floating-Point**: `f32` (single), `f64` (double), `f128` (quad).
- **Pointer**: `ptr` (target pointer width; 8 bytes on 64-bit systems).
- **Control & ABI Tokens**: `void`, `__bindToken` (used to order ABI argument/return sequences).
- **128-bit Vector (SSE)**: `v4f32`, `v2f64`, `v16i8`, `v8i16`, `v4i32`, `v2i64`.
- **256-bit Vector (AVX)**: `v8f32`, `v4f64`, `v32i8`, `v16i16`, `v8i32`, `v4i64`.

---

## 3. Middle-End Pass Framework

EzMir passes operate on `MirFunction` instances and are orchestrated by `MirPassManager` (`MirPasses/MirPassManager.h`).

```
       +-------------------------------------------------------------+
       |                       MirPassManager                        |
       +-------------------------------------------------------------+
         |                    |                   |                 |
         v                    v                   v                 v
   +----------------+  +----------------+  +---------------+  +---------------+
   | CodeFlowPass   |  |  NonSsaToSsa   |  | LivenessPass  |  | MirPeephole   |
   | CFG & Dominance|  | Cytron SSA     |  | LiveIntervals |  | SSA Optimizer |
   +----------------+  +----------------+  +---------------+  +---------------+
```

### 3.1 `CodeFlowAnalysisPass` (`MirPasses/Passes/CodeFlowAnalysisPass.h`)
- Traverses basic blocks to establish explicit CFG edges (`predecessors`, `successors`).
- Removes unreachable dead blocks.
- Computes the **Dominator Tree** and **Dominance Frontiers** using the Lengauer-Tarjan algorithm.
- Identifies loop headers and back-edges.

### 3.2 `NonSsaToSsaPass` (`MirPasses/Passes/NonSsaToSsaPass.h`)
- Converts non-SSA or partially-SSA code into minimal Static Single Assignment (SSA) form using Cytron's algorithm:
  1. Computes iterated dominance frontiers ($IDF$) for every multi-block variable.
  2. Places `PHI` nodes at the beginning of iterated dominance frontier blocks.
  3. Renames variables into versioned virtual registers via a dominator tree depth-first walk.

### 3.3 `LivenessAnalysisPass` (`MirPasses/Passes/LivenessAnalysisPass.h`)
- Executes backwards bit-vector dataflow analysis across all basic blocks using `DenseBitSet`.
- Computes `LiveIn` and `LiveOut` sets for each block using the transfer function:
  ```text
  LiveIn = Use ∪ (LiveOut \ Def)
  ```
- Computes linear **Live Intervals** $[start, end]$ for every virtual and physical register.
- Surfaces the `LivenessResult` structure directly consumed by `MirRegisterAllocator`.

### 3.4 `MirPeepholePass` (`MirPasses/Passes/MirPeepholePass.h`)
Generic SSA-level transformation pass (`IMirTransformPass`) active during optimization stages (`-O1`, `-O2`, `-Os`). Iterates over basic blocks and instructions until a fixed point is reached or the iteration budget is exhausted:
- **Algebraic Identities**:
  - `ADD %dst, %src, 0` / `ADD %dst, 0, %src` $\to$ `MOV %dst, %src`
  - `SUB %dst, %src, 0` $\to$ `MOV %dst, %src`
  - `SUB %dst, %src, %src` $\to$ `MOV %dst, 0`
  - `IMUL %dst, %src, 1` / `IMUL %dst, 1, %src` $\to$ `MOV %dst, %src`
  - `IMUL %dst, %src, 0` / `IMUL %dst, 0, %src` $\to$ `MOV %dst, 0`
  - `AND %dst, %src, 0` / `AND %dst, 0, %src` $\to$ `MOV %dst, 0`
  - `AND %dst, %src, -1` / `AND %dst, -1, %src` $\to$ `MOV %dst, %src`
  - `OR %dst, %src, 0` / `OR %dst, 0, %src` $\to$ `MOV %dst, %src`
  - `XOR %dst, %src, %src` $\to$ `MOV %dst, 0`
  - `XOR %dst, %src, 0` / `XOR %dst, 0, %src` $\to$ `MOV %dst, %src`
  - `SHL / LSHR / ASHR %dst, %src, 0` $\to$ `MOV %dst, %src`
- **Redundant Move Elimination**:
  - Eliminates self-moves (`MOV %x, %x`).
  - Eliminates reciprocal copies (`MOV %a, %b; MOV %b, %a` $\to$ second copy removed).
- **Dead Code Elimination After Terminators**:
  - Prunes dead, unreachable instructions occurring strictly after basic block terminators (`RET`, `JMP`, `UNREACHABLE`).
- **Fall-Through Jump Elimination**:
  - Erases unconditional `JMP` / `BR` instructions whose destination target is the immediately sequential basic block (`block->getNext()`).

---

## 4. Programmatic MIR Construction (Builder API)

EzMir provides a clean, factory-based builder architecture designed for compilers and frontend code generators. All builders allocate objects directly within the `MirBuilderContext` memory arena.

### 4.1 Builder Hierarchy

1. **`MirBuilderContext`** (`Builder/MirBuilderContext.h`):
   Central state owning the session memory arena, type table, diagnostic sink, and global ID counter.
2. **`MirFunctionBuilder`** (`Function/MirFunctionBuilder.h`):
   Instantiates `MirFunction` objects and produces child block builders:
   - `build(...)`: Constructs a function definition with an initial entry point basic block (`isDefinition() == true`). Supports configuring `MirLinkage` (`External`, `Internal`, `Weak`).
   - `declare(...)`: Constructs an external function declaration without any basic blocks (`getBlockCount() == 0`, `isDeclaration() == true`). Accepts parameter registers or parameter type lists (`std::span<MirType* const>` or `std::initializer_list<MirType*>`).
3. **`MirGlobalVarBuilder`** (`GlobalVar/MirGlobalVarBuilder.h`):
   Constructs global variable declarations with configurable type, immutability, `MirLinkage`, and initializer operands.
4. **`MirBlockBuilder`** (`Block/MirBlockBuilder.h`):
   Appends `MirBlock` nodes to the function and produces child instruction builders.
5. **`MirInstructionBuilder`** (`Instruction/MirInstructionBuilder.h`):
   Constructs instructions at a configurable insertion point (`Append`, `InsertBefore`, `InsertAfter`). Generates high-level opcode methods (`ADD`, `MOV`, `SUB`, `RET`, `JMP`, etc.), target instruction methods (`buildTarget`), and the `setOperand(MirInstruction *instr, size_t pos, MirOperand *newOperand)` utility for in-place operand substitution with automatic def/use tracking synchronization.
6. **`MirOperandBuilder`** (`Operand/MirOperandBuilder.h`):
   Constructs virtual registers, physical registers, memory operands, constants, and symbol references.

### 4.2 Complete Programmatic Example

The following C++20 code demonstrates constructing a complete function:
`fn @calculate(i32 %a, i32 %b) -> i32 { return %a + %b; }`

```cpp
#include "Builder/MirBuilderContext.h"
#include "Function/MirFunctionBuilder.h"
#include "Block/MirBlockBuilder.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Operand/MirOperandBuilder.h"
#include "Type/MirTypeTable.h"
#include "Diagnostics/DiagnosticCollector.h"
#include <memory_resource>

// 1. Initialize memory arena and compiler infrastructure
std::pmr::monotonic_buffer_resource arena(1024 * 1024);
DiagnosticCollector diagCollector;
MirTypeTable typeTable(&arena);

// Calling convention (nullptr selects target default)
CallingConvDesc *defaultCC = nullptr;

// 2. Instantiate the root MIR context
MirBuilderContext ctx(defaultCC, &diagCollector, &typeTable, &arena);

// Retrieve types
MirType *i32Type = typeTable.getI32();

// 3. Create operand builder and instantiate formal parameters as virtual registers
MirOperandBuilder opBuilder(&ctx);
MirRegister *paramA = opBuilder.buildVReg(i32Type, "a");
MirRegister *paramB = opBuilder.buildVReg(i32Type, "b");

// 4. Build the function: fn @calculate(i32 %a, i32 %b) -> i32
MirFunctionBuilder funcBuilder(&ctx);
MirFunction *func = funcBuilder.build(i32Type, { paramA, paramB }, "calculate", defaultCC);

// 5. Build the entry basic block
MirBlockBuilder blockBuilder = funcBuilder.blockBuilder();
MirBlock *entryBlock = blockBuilder.build(nullptr, "entry");

// 6. Build instructions inside the entry block
MirInstructionBuilder instrBuilder = blockBuilder.instrBuilder();

// Allocate virtual register for the sum
MirRegister *sumReg = opBuilder.buildVReg(i32Type, "sum");

// Emit: ADD %sum, %a, %b
instrBuilder.ADD(sumReg, paramA, paramB);

// Emit: RET %sum
instrBuilder.RET(sumReg);
```

---

## 5. Textual MIR Format

EzPacker supports a clean textual representation for serialization, unit testing, and human inspection.

### 5.1 Global Variables & External Declarations
```mir
; Global variables with linkage (internal, external, weak)
@greeting = internal const [14 x i8] "Hello, World!\0A\00";
@counter  = external var i64 = 0;
@flag     = weak var i32 = 1;

; External function prototypes (C-style declarations)
declare @puts(ptr) -> i32;
weak declare @custom_init(i64) -> void;
extern fn @external_worker(ptr, i32) -> void;
```

### 5.2 Function Definitions with Linkage
```mir
; Module-private helper function
internal fn @compute_offset(i32 %index) -> i64 {
entry:
    %ext = sext i32 %index -> i64;
    %off = mul i64 %ext, 4;
    ret i64 %off;
}

; Weakly-linked default handler (can be overridden by another object)
weak fn @fallback_handler() -> void {
entry:
    ret;
}

; Public function entry point
fn @dot_product(ptr %arr_a, ptr %arr_b, i32 %n) -> i32 {
entry:
    %acc.0 = mov.i32 0
    %i.0 = mov.i32 0
    jmp loop_cond

loop_cond:
    %cmp = cmp_slt.i1 %i.0, %n
    br_cond %cmp, loop_body, exit

loop_body:
    %offset_a = mul.i64 %i.0, 4
    %addr_a = add.ptr %arr_a, %offset_a
    %val_a = load.i32 [%addr_a]

    %offset_b = mul.i64 %i.0, 4
    %addr_b = add.ptr %arr_b, %offset_b
    %val_b = load.i32 [%addr_b]

    %prod = mul.i32 %val_a, %val_b
    %acc.next = add.i32 %acc.0, %prod
    %i.next = add.i32 %i.0, 1
    jmp loop_cond

exit:
    ret %acc.0
}
```

---

## 6. Header & Class Index

| Component | Header Location | Key Classes / Structs |
|---|---|---|
| Linkage | `EzMir/include/Linkage/MirLinkage.h` | `MirLinkage` |
| Function | `EzMir/include/Function/MirFunction.h` | `MirFunction` |
| Function Frame | `EzMir/include/Function/MirFunctionStackFrame.h` | `MirFunctionStackFrame`, `StackFrameObject` |
| Register Info | `EzMir/include/Function/MirFunctionRegisterInfo.h` | `MirFunctionRegisterInfo` |
| Global Variable | `EzMir/include/GlobalVar/MirGlobalVar.h` | `MirGlobalVar` |
| Block | `EzMir/include/Block/MirBlock.h` | `MirBlock` |
| Instruction | `EzMir/include/Instruction/MirInstruction.h` | `MirInstruction`, `MirInstructionFlags` |
| Instruction Set | `EzMir/include/Instruction/MirInstructionSet.h` | `MirInstructionOpCode` |
| Operand | `EzMir/include/Operand/MirOperand.h` | `MirOperand`, `ExpectedOperandType` |
| Operands | `EzMir/include/Operand/MirRegister.h` | `MirRegister`, `MirPhysicalRegId` |
| Operands | `EzMir/include/Operand/MirInteger.h` | `MirInteger` |
| Operands | `EzMir/include/Operand/MirFloat.h` | `MirFloat` |
| Operands | `EzMir/include/Operand/MirMemory.h` | `MirMemory` |
| Operands | `EzMir/include/Operand/MirReference.h` | `MirReference` |
| Operands | `EzMir/include/Operand/MirRuntimeSymbol.h` | `MirRuntimeSymbol` |
| Types | `EzMir/include/Type/MirType.h` | `MirType`, `MirTypeKind` |
| Types | `EzMir/include/Type/MirTypeTable.h` | `MirTypeTable` |
| Builders | `EzMir/include/Builder/MirBuilderContext.h` | `MirBuilderContext` |
| Builders | `EzMir/include/Function/MirFunctionBuilder.h` | `MirFunctionBuilder` |
| Builders | `EzMir/include/GlobalVar/MirGlobalVarBuilder.h` | `MirGlobalVarBuilder` |
| Builders | `EzMir/include/Block/MirBlockBuilder.h` | `MirBlockBuilder` |
| Builders | `EzMir/include/Instruction/MirInstructionBuilder.h` | `MirInstructionBuilder`, `MirInstructionInsertionPoint`, `InsertionType` |
| Builders | `EzMir/include/Operand/MirOperandBuilder.h` | `MirOperandBuilder` |
| Passes | `EzMir/include/MirPasses/MirPassManager.h` | `MirPassManager` |
| Passes | `EzMir/include/MirPasses/Passes/CodeFlowAnalysisPass.h` | `CodeFlowAnalysisPass` |
| Passes | `EzMir/include/MirPasses/Passes/NonSsaToSsaPass.h` | `NonSsaToSsaPass` |
| Passes | `EzMir/include/MirPasses/Passes/LivenessAnalysisPass.h` | `LivenessAnalysisPass`, `LivenessResult` |
| Passes | `EzMir/include/MirPasses/Passes/MirPeepholePass.h` | `MirPeepholePass` |
| Printer | `EzMir/include/Printer/MirPrinter.h` | `MirPrinter`, `MirPrinterMode`, `MirPrinterDetail` |
| Parser | `EzMir/include/Parser/MirParser.h` | `MirParser`, `MirParserOptions` |
