# EzMir Subproject Documentation

[EzPacker Documentation Index](../index.md) > **EzMir**

---

## 1. Overview & Architectural Role

`EzMir` (Machine Intermediate Representation) is the core representation layer of EzPacker. It models functions, basic blocks, instructions, operands, and types in both high-level generic SSA (Static Single Assignment) form and low-level target-lowered machine form.

EzMir bridges frontends (or parsed textual `.mir` files) with the backend optimization passes, legalizers, instruction selectors, and code emitters.

```
       +-------------------------------------------------------------+
       |                         Source / Frontend                   |
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
|   +-- MirFunctionStackFrame (local objects, spill slots, alignments)          |
|   +-- MirFunctionRegisterInfo (virtual registers, physical assignments)       |
|   +-- MirBlock (Entry)                                                        |
|   |    +-- IntrusiveLinkedList<MirInstruction>                                |
|   |         +-- MirInstruction [MOV dst:%0, src:42]                          |
|   |              +-- MirOperand (VReg, PReg, Imm, Mem, Symbol, Label)         |
|   |              +-- MirType (i1..i256, f32..f128, ptr, vectors)              |
|   +-- MirBlock (Loop / Exit)                                                  |
+-------------------------------------------------------------------------------+
       |                                       |
       v                                       v
+-----------------------------+       +-----------------------------------------+
|     Middle-End Passes       |       |              MirPrinter                 |
| - CodeFlowAnalysisPass      |       | (Emits human-readable textual MIR)      |
| - NonSsaToSsaPass           |       +-----------------------------------------+
| - LivenessAnalysisPass      |
+-----------------------------+
       |
       v
  To EzTriple Lowering Pipeline
```

---

## 2. In-Memory Intermediate Representation (IR)

### 2.1 `MirFunction` (`Function/MirFunction.h`)
The top-level entity representing a callable procedure or function:
- **Signature**: Function name (e.g. `@compute_hash`), calling convention descriptor, parameter types and names, return type.
- **Control Flow Structure**: An ordered list of `MirBlock` nodes, beginning with the designated entry block.
- **`MirFunctionStackFrame`**: Manages local variables allocated on the stack frame, dynamic stack allocations, alignment requirements, and compiler-generated spill slots.
- **`MirFunctionRegisterInfo`**: Allocates virtual register IDs, tracks their assigned register classes, records callee-saved registers, and stores final physical register bindings.

### 2.2 `MirBlock` (`Block/MirBlock.h`)
A straight-line sequence of instructions with single entry and single exit:
- **Instruction Container**: Holds an `IntrusiveLinkedList<MirInstruction>`, enabling zero-heap insertion, removal, and splicing.
- **CFG Edges**: Stores explicit predecessor and successor block lists (`std::pmr::vector<MirBlock*>`).
- **Phi Functions**: Tracks SSA phi instructions positioned at the block head.
- **Terminator**: The last instruction in the block must be a terminator (`RET`, `BR_COND`, `JMP`, or `UNREACHABLE`).

### 2.3 `MirInstruction` (`Instruction/MirInstruction.h`)
The fundamental executable operation:
- **Opcode**:
  - Generic High-Level Opcodes (`MirInstructionOpCode` from `Instruction/MirInstructionSet.h`): `MOV`, `ADD`, `SUB`, `IMUL`, `SDIV`, `UDIV`, `AND`, `OR`, `XOR`, `SHL`, `LSHR`, `ASHR`, `LOAD`, `STORE`, `CMP_EQ`, `CMP_SLT`, `BR_COND`, `JMP`, `CALL`, `RET`, `PHI`, etc.
  - Target-Specific Opcodes: Pointers to `MirTargetInstructionDesc` synthesized by `EzDsl` (e.g. `ADD64rr`, `MOV32ri`, `VADDPSrr`).
- **Operands**: Distinct destination operands (`OUT`) and source operands (`IN`).
- **Instruction Flags**:
  - `IsCommutative`: Operands can be swapped during pattern matching.
  - `IsBranch` / `IsConditionalBranch` / `IsTerminator`: Control flow indicators.
  - `IsCall` / `IsReturn`: Inter-procedural operations.
  - `ReadsMemory` / `WritesMemory`: Memory barrier and load-folding metadata.
  - `HasSideEffect`: Prevents dead-code elimination.
- **Intrusive Links**: Embedded `prev` and `next` pointers satisfying `IntrusiveLinkedList`.

### 2.4 `MirOperand` (`Operand/MirOperand.h`)
A tagged union representing an instruction input or output:
- **`VirtualRegister`**: Denoted `%0`, `%1`, `%val`. Created dynamically with an assigned `MirType`.
- **`PhysicalRegister`**: A concrete hardware register (e.g. `rax`, `rcx`, `xmm0`) belonging to a target `MirRegisterClass`.
- **`Immediate`**: Holds a constant `FlexInt` or `FlexFloat`.
- **`MemoryAddress`**: Base register, index register, scale factor (1, 2, 4, 8), and displacement (`FlexInt`).
- **`Symbol`**: Reference to an external function or global variable.
- **`BlockLabel`**: Jump target referencing a `MirBlock` ID.

### 2.5 `MirType` (`Type/MirType.h`)
EzPacker's rich type system:
- **Integer Types**: Arbitrary bit widths: `i1`, `i8`, `i16`, `i32`, `i64`, `i128`, `i256`.
- **Floating-Point Types**: IEEE 754 scalar types: `f32` (single), `f64` (double), `f128` (quad).
- **Pointers**: `ptr` (target pointer width, 8 bytes on 64-bit systems).
- **Special / Control**: `void`, `__bindToken` (for ABI sequence binding).
- **Vector / SIMD Types**:
  - 128-bit SSE vectors: `v4f32`, `v2f64`, `v16i8`, `v8i16`, `v4i32`, `v2i64`.
  - 256-bit AVX vectors: `v8f32`, `v4f64`, `v32i8`, `v16i16`, `v8i32`, `v4i64`.

---

## 3. Middle-End Pass Framework

EzMir provides an analysis and transformation pass architecture managed by `MirPassManager` (`MirPasses/MirPassManager.h`).

```
       +---------------------------------------------+
       |               MirPassManager                |
       +---------------------------------------------+
         |                      |                   |
         v                      v                   v
  +------------------+  +------------------+  +-------------------+
  | CodeFlowAnalysis |  |   NonSsaToSsa    |  | LivenessAnalysis  |
  | CFG & Dominators |  | SSA Construction |  | Live Intervals    |
  +------------------+  +------------------+  +-------------------+
```

### 3.1 `CodeFlowAnalysisPass` (`MirPasses/Passes/CodeFlowAnalysisPass.h`)
- Traverses the function's basic blocks to construct the Control Flow Graph (CFG).
- Identifies predecessors, successors, and unreachable dead blocks.
- Computes the **Dominator Tree** and **Dominance Frontiers** using the Lengauer-Tarjan algorithm.
- Identifies back-edges and natural loops.

### 3.2 `NonSsaToSsaPass` (`MirPasses/Passes/NonSsaToSsaPass.h`)
- Converts non-SSA or partially-SSA MIR into pure Static Single Assignment form.
- Uses Cytron's algorithm:
  1. Computes dominance frontiers from `CodeFlowAnalysisPass`.
  2. Places `PHI` nodes at iterated dominance frontiers for all variables assigned across multiple blocks.
  3. Renames variables into versioned virtual registers using a dominator tree depth-first walk.

### 3.3 `LivenessAnalysisPass` (`MirPasses/Passes/LivenessAnalysisPass.h`)
- Performs backwards bit-vector dataflow analysis across all basic blocks using `DenseBitSet`.
- Computes `LiveIn` and `LiveOut` sets for each block.
- Calculates precise linear **Live Intervals** $[start, end]$ for every virtual and physical register.
- Provides the foundational liveness data required by `MirRegisterAllocator`.

---

## 4. Programmatic MIR Construction (Builder API)

To generate MIR from a compiler frontend or AST, EzMir provides a set of fluent builders:

```cpp
#include "Builder/MirBuilderContext.h"
#include "Function/MirFunctionBuilder.h"
#include "Block/MirBlockBuilder.h"
#include "Instruction/MirInstructionBuilder.h"

// 1. Create the root context
MirBuilderContext ctx;
auto *i32Type = ctx.getTypeTable().getI32();

// 2. Build the function: fn @add_values(i32 %a, i32 %b) -> i32
MirFunctionBuilder funcBuilder(ctx, "add_values");
funcBuilder.setReturnType(i32Type);
auto *argA = funcBuilder.addParameter("a", i32Type);
auto *argB = funcBuilder.addParameter("b", i32Type);
auto *func = funcBuilder.build();

// 3. Build the entry block
MirBlockBuilder blockBuilder(ctx, func, "entry");
auto *entryBlock = blockBuilder.build();

// 4. Build instructions inside the block
MirInstructionBuilder instBuilder(ctx, entryBlock);

// %res = ADD i32 %a, %b
auto *resReg = func->getRegisterInfo().createVirtualRegister(i32Type);
instBuilder.build(MirInstructionOpCode::ADD)
    .addDef(resReg)
    .addUse(argA)
    .addUse(argB)
    .emit();

// RET i32 %res
instBuilder.build(MirInstructionOpCode::RET)
    .addUse(resReg)
    .emit();
```

---

## 5. Textual MIR: Parser & Printer

### 5.1 Textual Format Syntax
EzPacker MIR has a human-readable textual representation resembling LLVM IR and Cranelift IR:
```mir
fn @multiply_accumulate(i32 %a, i32 %b, i32 %c) -> i32 {
entry:
    %prod = IMUL i32 %a, %b;
    %sum = ADD i32 %prod, %c;
    RET i32 %sum;
}
```

### 5.2 `MirLexer` & `MirParser` (`Parser/MirLexer.h`, `Parser/MirParser.h`)
- Tokenizes and parses `.mir` source modules into `MirFunction` object trees.
- Validates operand arity, type consistency, and block label existence.
- Emits detailed diagnostics with line/column coordinates via `DiagnosticCollector`.

### 5.3 `MirPrinter` (`Printer/MirPrinter.h`)
- Serializes in-memory `MirFunction` and `MirBlock` hierarchies back to formatted textual MIR.
- Supports printing before and after each compiler pass for debugging and regression testing.

---

## 6. API Reference & Further Reading

- Generated Doxygen API documentation: [Doxygen Documentation Index](../doxygen/index.html)
- Next subproject: [EzDsl Subproject Documentation](EzDsl.md)
- Return to [EzPacker Landing Page](../index.md)
