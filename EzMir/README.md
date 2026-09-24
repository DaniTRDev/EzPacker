# EzMir (Machine Intermediate Representation)

`EzMir` is the intermediate representation and middle-end compilation framework of the **EzPacker** toolchain. It provides a strongly-typed, SSA-capable, target-configurable IR designed to span the continuum from high-level structured representations down to low-level target machine instructions.

---

## Key Features

1. **Multi-Tier IR Representation**:
   - **High-Level IR**: Abstract, target-independent SSA operations (`ADD`, `SUB`, `CALL`, `RET`, etc.).
   - **Pass-Internal IR**: Intermediate lowering primitives (`PUSH_ARG`, `POP_ARG`, `PUSH_RET`, `POP_RET`).
   - **Target-Low IR**: Post-instruction-selection target instructions tied to physical register classes.
2. **Complete SSA Infrastructure**:
   - Automated conversion from non-SSA variable assignments to SSA form using the **Cytron et al.** algorithm.
   - Immediate dominator computation (**Cooper, Harvey, and Kennedy**), dominance frontiers, iterated dominance frontier ($\text{IDF}$) placement for $\phi$ nodes, and variable version stack renaming.
3. **Flexible Calling Convention & ABI Subsystem**:
   - Pluggable calling convention models (`CallingConvDesc`) supporting System V AMD64, Microsoft x64, and custom ABIs.
   - Dynamic argument placement resolution (`ArgumentLocationDesc`: Register, Stack, Split structs, Indirect/ByVal).
   - Dedicated `CallLoweringState` tracking scratch registers across complex calls.
4. **Target Layout & Type System**:
   - Canonical interned type table (`MirTypeTable`) supporting primitives (`i1`..`i128`, `f16`..`f128`, `void`), pointers, arrays, and classes.
   - Target-independent layout query interface (`IMirTargetTypeLayout`).
5. **Pass Manager Pipeline**:
   - Dependency-driven pass orchestration (`MirPassManager`) with automatic topological pipeline sorting, on-demand analysis caching, and pass invalidation.
   - Extensible analysis (`IMirAnalysisPass`) and transformation (`IMirTransformPass`) passes.
6. **Polymorphic Memory Architecture (`std::pmr`)**:
   - Fast monotonic arena allocations for functions, blocks, instructions, and operands with zero heap fragmentation.

---

## Architecture & Hierarchy

```
                            ┌────────────────────────┐
                            │   MirBuilderContext    │
                            │ (Arena, Types, Diags)  │
                            └───────────┬────────────┘
                                        │
             ┌──────────────────────────┼──────────────────────────┐
             ▼                          ▼                          ▼
   ┌───────────────────┐      ┌───────────────────┐      ┌───────────────────┐
   │    MirFunction    │      │     MirClass      │      │   MirGlobalVar    │
   │  - Stack Frame    │      │  - Fields         │      │  - Constant Data  │
   │  - Parameters     │      │  - VTable Methods │      │  - Linkage        │
   │  - Basic Blocks   │      │  - Offsets        │      │  - Ptr Type       │
   └─────────┬─────────┘      └───────────────────┘      └───────────────────┘
             │
             ▼
   ┌───────────────────┐
   │     MirBlock      │
   │  - Instruction    │
   │    Linked List    │
   └─────────┬─────────┘
             │
             ▼
   ┌───────────────────┐
   │  MirInstruction   │
   │  - OpCode & Tier  │
   │  - Operands List  │
   │  - Metadata/Flags │
   └─────────┬─────────┘
             │
             ▼
   ┌───────────────────────────────────────────────────────────────┐
   │                         MirOperand                            │
   │  - MirRegister (Virtual/Physical)    - MirReference           │
   │  - MirInteger (FlexInt)              - MirMemory [Base+Disp]  │
   │  - MirFloat (FlexFloat)              - MirRuntimeSymbol       │
   └───────────────────────────────────────────────────────────────┘
```

---

## Core Subsystems

### 1. Context and Builders (`EzMir/include/Builder/`)
- **`MirBuilderContext`**: The root container for an IR module. Owns the global `std::pmr::monotonic_buffer_resource`, `MirTypeTable`, `DiagnosticCollector`, and index maps for blocks, functions, globals, classes, and registers.
- **`MirFunctionBuilder`**: Fluent builder for constructing `MirFunction` instances, accumulating formal parameters, and setting calling conventions.
- **`MirBlockBuilder`**: Fluent builder for instantiating and appending basic blocks (`MirBlock`) to functions.
- **`MirInstructionBuilder`**: Fluent instruction emission interface providing named helpers for every IR opcode (`ADD`, `SUB`, `CALL`, `RET`, `LOAD`, `STORE`, etc.) with positional operand insertion (`AppendToEnd`, `PrependToStart`, `InsertBefore`, `InsertAfter`).

### 2. Instructions & Operands (`EzMir/include/Instruction/`, `EzMir/include/Operand/`)
- **`MirInstruction`**: An instruction instance containing an opcode (`MirInstructionOpCode`), flags (`MirInstructionFlags`), source location (`SourceReference`), and a dynamic operand array (`std::pmr::vector<MirOperand*>`). Intrusively linked inside `MirBlock`.
- **`MirRegister` & `MirRegisterRef`**: Unifies virtual registers (`vreg(id)`) and physical registers (`preg(desc)`).
- **`MirMemory`**: Base-displacement memory addressing (`[baseReg + displacement]`).
- **`MirReference`**: Symbolic references to code blocks, global variables, functions, class fields, or stack slots, lowered by downstream passes into concrete pointer offsets.

### 3. ABI and Calling Conventions (`EzMir/include/Function/`)
- **`CallingConvDesc`**: Encapsulates ABI-specific rules:
  - Stack growth direction (`UP`, `DOWN`) and alignment boundaries.
  - Caller-saved and callee-saved register partitions.
  - Frame pointer (`RBP`/`FP`) and stack pointer (`RSP`/`SP`) requirements.
  - Argument and return value classification (`getArgLoc()`, `getReturnLoc()`).
- **`MirFunctionStackFrame`**: Tracks static locals, spill slots, and stack parameters with layout offsets resolved during frame lowering.

### 4. Built-in Compiler Passes (`EzMir/include/MirPasses/Passes/`)
- **`CodeFlowAnalysisPass`**: Analyzes control flow instructions to construct the Control Flow Graph (CFG) containing predecessor and successor mappings for each basic block.
- **`NonSsaToSsaPass`**: Computes dominator trees and dominance frontiers to place minimal $\phi$ nodes and rename virtual registers into Single Static Assignment (SSA) form.
- **`LivenessAnalysisPass`**: Computes local block `def`/`use` sets and solves backward dataflow equations to determine `liveIn`/`liveOut` register intervals for register allocation.
- **`ClassOffsetResolverPass`**: Resolves byte offsets and virtual table layout for object-oriented class hierarchies.
- **`RelativeReferenceLowererPass`**: Lowers symbolic `MirReference` operands into base-plus-displacement `MirMemory` operations.

### 5. Formatting & Disassembly (`EzMir/include/Printer/`)
- **`MirPrinter`**: Disassembles blocks, functions, global variables, instructions, and operands into human-readable formatted textual IR with configurable verbosity (`MirPrinterDetail::General` vs `MirPrinterDetail::Detailed`).

---

## Usage Example

```cpp
#include "Builder/MirBuilderContext.h"
#include "Function/MirFunctionBuilder.h"
#include "Block/MirBlockBuilder.h"
#include "Instruction/MirInstructionBuilder.h"
#include "MirPasses/MirPassManager.h"
#include "MirPasses/Passes/CodeFlowAnalysisPass.h"
#include "MirPasses/Passes/NonSsaToSsaPass.h"
#include "Printer/MirPrinter.h"
#include "Type/MirTypeTable.h"

// 1. Initialize Memory Arena, Diagnostics & Context
std::pmr::monotonic_buffer_resource arena;
DiagnosticCollector diagCollector;
MirTypeTable typeTable(&arena);
CallingConvDesc *callingConv = GetSystemVCallingConv(&arena);

MirBuilderContext context(callingConv, &diagCollector, &typeTable, &arena);

// 2. Build Function: i32 add(i32 a, i32 b)
MirType *i32Type = typeTable.i32();
MirType *funcType = typeTable.getFuncType(i32Type, { i32Type, i32Type });

MirFunctionBuilder funcBuilder(&context);
MirFunction *func = funcBuilder.buildParam(i32Type, "a")
                               .buildParam(i32Type, "b")
                               .build(i32Type, "add");

// 3. Build Entry Block & Instructions
MirBlockBuilder blockBuilder(&context, func);
MirBlock *entryBlock = blockBuilder.build(/*sourceRef=*/nullptr, "entry");

MirInstructionBuilder instBuilder(&context, entryBlock, InsertionType::AppendToEnd);

// Fetch parameter registers
auto paramIt = func->getParameters().begin(); 
MirRegister *regA = *paramIt++;
MirRegister *regB = *paramIt;

// Allocate result virtual register & emit ADD + RET
MirRegister *resReg = instBuilder.createVReg(i32Type, "sum");
instBuilder.ADD(resReg, regA, regB);
instBuilder.RET(resReg);

// 4. Run Optimization / SSA Passes
MirPassManager passManager(&diagCollector, &arena);
passManager.addPass<CodeFlowAnalysisPass>(&context);
passManager.addPass<NonSsaToSsaPass>(&context);

passManager.generatePipeline();
passManager.runPipeline(&context);

// 5. Print Textual IR
std::string irText = MirPrinter::printToString(func, MirPrinterDetail::Detailed);
std::cout << irText << std::endl;
```