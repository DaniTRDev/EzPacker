# EzMir: Machine Intermediate Representation & Middle-End SSA Framework

[`EzMir`](file:///E:/Repos/EzPacker/EzMir) is the intermediate representation and middle-end compilation framework of the **EzPacker** compiler toolchain. It provides a strongly-typed, SSA-capable, multi-tier IR engineered to span the continuum from high-level structured operations down to physical, hardware-encoded machine instructions.

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
   │    MirFunction    │      │   MirGlobalVar    │      │  Type & ABI Descs │
   │  - Stack Frame    │      │  - Constant Data  │      │  - MirTypeTable   │
   │  - Parameters     │      │  - Linkage        │      │  - CallingConvDesc│
   │  - Basic Blocks   │      │  - Memory Ptr     │      │  - RegisterBanks  │
   └─────────┬─────────┘      └───────────────────┘      └───────────────────┘
             │
             ▼
   ┌───────────────────┐
   │     MirBlock      │
   │  - Predecessors   │
   │  - Successors     │
   │  - Intrusive List │
   └─────────┬─────────┘
             │
             ▼
   ┌───────────────────────────────────────────────────────────────┐
   │                        MirInstruction                         │
   │  - MirInstructionOpCode          - MirInstructionTier         │
   │  - MirInstructionCategory        - MirInstructionFlags        │
   │  - SourceReference*              - Operands Vector            │
   │  - TargetDesc* (when selected)   - Intrusive Next/Prev Links  │
   └───────────────────────────────┬───────────────────────────────┘
                                   │
                                   ▼
   ┌───────────────────────────────────────────────────────────────┐
   │                          MirOperand                           │
   │  - MirRegister (Virtual %v / Physical %p)                     │
   │  - MirInteger (Arbitrary-Precision FlexInt)                   │
   │  - MirFloat (Arbitrary-Precision FlexFloat)                   │
   │  - MirMemory (Base + Displacement + Scaled Index)             │
   │  - MirReference (Symbolic Block, Function, Global, StackSlot) │
   │  - MirRuntimeSymbol (External @symbol Linkage)                │
   └───────────────────────────────────────────────────────────────┘
```

---

## Key Subsystems & Design

### 1. Multi-Tier IR Representation

To bridge high-level programming semantics with hardware execution without impedance mismatch, `EzMir` partitions instructions into three distinct compilation tiers via [`MirInstructionTier`](file:///E:/Repos/EzPacker/EzMir/include/Instruction/MirInstructionMetadata.h#L195):

1. **High-Level IR (`MirInstructionTier::HighLevel`)**:
   - Abstract, target-independent SSA operations emitted by frontends and builders.
   - Arithmetic (`ADD`, `SUB`, `MUL`, `DIV`, `NEG`), bitwise logic (`AND`, `OR`, `XOR`, `SHL`, `SHR`), comparisons (`CMP_EQ`, `CMP_NE`, `CMP_LT`, etc.), memory operations (`LOAD`, `STORE`), and structured control flow (`BR`, `JMP`, `CALL`, `RET`, `PHI`).
   - Operands are virtual registers (`vreg`), literals, or symbolic references.
2. **Pass-Internal IR (`MirInstructionTier::PassInternal`)**:
   - Lowering primitives used by intermediate compiler transformations before final instruction selection.
   - Includes calling convention staging operations (`PUSH_ARG`, `POP_ARG`, `PUSH_RET`, `POP_RET`), parameter passing tokens, and scalar merge/unmerge markers (`MERGE_VALUES`, `UNMERGE_VALUES`).
3. **Target-Low IR (`MirInstructionTier::TargetLow`)**:
   - Machine-specific instructions produced by target instruction selection (`TARGET_INST`).
   - Bound directly to physical [`MirTargetInstructionDesc`](file:///E:/Repos/EzPacker/EzMir/include/Instruction/MirTargetInstructionDesc.h) records, hardware register classes ([`MirRegisterClass`](file:///E:/Repos/EzPacker/EzMir/include/Operand/MirRegisterClass.h)), and physical hardware encodings.

---

### 2. Module Containers & Fluent Builders (`EzMir/include/Builder/`, `Function/`, `Block/`)

`EzMir` enforces strict hierarchy through arena-backed builders:

* **[`MirBuilderContext`](file:///E:/Repos/EzPacker/EzMir/include/Builder/MirBuilderContext.h)**:
  - Root container for a compilation unit.
  - Owns the primary memory arena (`std::pmr::monotonic_buffer_resource`), [`MirTypeTable`](file:///E:/Repos/EzPacker/EzMir/include/Type/MirType.h), [`DiagnosticCollector`](file:///E:/Repos/EzPacker/EzCore/include/Diagnostics/DiagnosticCollector.h), and the default target [`CallingConvDesc`](file:///E:/Repos/EzPacker/EzMir/include/Function/CallingConvDesc.h).
  - Maintains index maps for functions, global variables, basic blocks, and virtual registers.
* **[`MirFunctionBuilder`](file:///E:/Repos/EzPacker/EzMir/include/Function/MirFunctionBuilder.h)**:
  - Fluent builder for instantiating [`MirFunction`](file:///E:/Repos/EzPacker/EzMir/include/Function/MirFunction.h) objects.
  - Accumulates typed formal parameters (`buildParam(type, name)`), assigns calling conventions, and creates entry basic blocks.
* **[`MirBlockBuilder`](file:///E:/Repos/EzPacker/EzMir/include/Block/MirBlockBuilder.h)**:
  - Fluent builder for constructing [`MirBlock`](file:///E:/Repos/EzPacker/EzMir/include/Block/MirBlock.h) instances.
  - Connects blocks to functions and maintains labels for branch targets.
* **[`MirInstructionBuilder`](file:///E:/Repos/EzPacker/EzMir/include/Instruction/MirInstructionBuilder.h)**:
  - High-performance fluent instruction emission interface.
  - Exposes dedicated methods for every generic opcode (`ADD`, `SUB`, `MUL`, `LOAD`, `STORE`, `CALL`, `RET`, `BR`, `PHI`, etc.).
  - Supports flexible insertion cursors: `InsertionType::AppendToEnd`, `InsertionType::PrependToStart`, `InsertionType::InsertBefore`, and `InsertionType::InsertAfter`.
  - Manages virtual register allocation (`createVReg(type, name)`).
* **Intrusive Doubly-Linked List Model**:
  - Functions own basic blocks and basic blocks own instructions using [`IntrusiveLinkedList<MirInstruction>`](file:///E:/Repos/EzPacker/EzCore/include/HelperClasses/IntrusiveLinkedList.h).
  - Eliminates secondary node allocations when inserting, removing, reordering, or splicing instructions during optimization and legalization passes.

---

### 3. Instructions & Operand Model (`EzMir/include/Instruction/`, `Operand/`)

* **[`MirInstruction`](file:///E:/Repos/EzPacker/EzMir/include/Instruction/MirInstruction.h)**:
  - Encapsulates opcode ([`MirInstructionOpCode`](file:///E:/Repos/EzPacker/EzMir/include/Instruction/MirInstructionSet.h)), category ([`MirInstructionCategory`](file:///E:/Repos/EzPacker/EzMir/include/Instruction/MirInstructionMetadata.h#L179)), tier ([`MirInstructionTier`](file:///E:/Repos/EzPacker/EzMir/include/Instruction/MirInstructionMetadata.h#L195)), semantic flags ([`MirInstructionFlags`](file:///E:/Repos/EzPacker/EzMir/include/Instruction/MirInstructionMetadata.h#L136)), optional [`SourceReference*`](file:///E:/Repos/EzPacker/EzCore/include/SourceManager/GenericSourceManager.h), and a dynamic operand array (`std::pmr::vector<MirOperand*>`).
  - Flags capture invariants: `IsCommutative`, `ReadsMemory`, `WritesMemory`, `IsTerminator`, `IsBranch`, `IsCall`, `IsReturn`, `HasSideEffect`, and `IsMove`.
* **[`MirOperand`](file:///E:/Repos/EzPacker/EzMir/include/Operand/MirOperand.h) Hierarchy** ([`MirOperands.h`](file:///E:/Repos/EzPacker/EzMir/include/Operand/MirOperands.h)):
  - **[`MirRegister`](file:///E:/Repos/EzPacker/EzMir/include/Operand/MirOperands.h#L233)**: Unifies virtual SSA registers (`%v0`, `%v1`) and physical registers (`%p0(rax:GPR64)`) through [`MirRegisterRef`](file:///E:/Repos/EzPacker/EzMir/include/Operand/MirRegisterReference.h). Tracks register class constraints ([`MirRegisterClass`](file:///E:/Repos/EzPacker/EzMir/include/Operand/MirRegisterClass.h)) and sub-register aliasing.
  - **[`MirRegisterClass`](file:///E:/Repos/EzPacker/EzMir/include/Operand/MirRegisterClass.h) & [`MirRegisterBank`](file:///E:/Repos/EzPacker/EzMir/include/Operand/MirRegisterBank.h)**: Organize target registers into classes (e.g. `GPR8`, `GPR16`, `GPR32`, `GPR64`, `FPR32`, `FPR64`) with detailed [`MirRegisterDescriptor`](file:///E:/Repos/EzPacker/EzMir/include/Operand/MirRegisterClass.h#L16) records tracking bit sizes, hardware encodings, and sub-part slicing.
  - **[`MirInteger`](file:///E:/Repos/EzPacker/EzMir/include/Operand/MirOperands.h#L79)**: Arbitrary-precision integer literal powered by [`FlexInt`](file:///E:/Repos/EzPacker/EzCore/include/FlexNumber/FlexInt.h).
  - **[`MirFloat`](file:///E:/Repos/EzPacker/EzMir/include/Operand/MirOperands.h#L39)**: Arbitrary-precision IEEE-754 floating-point literal powered by [`FlexFloat`](file:///E:/Repos/EzPacker/EzCore/include/FlexNumber/FlexFloat.h).
  - **[`MirMemory`](file:///E:/Repos/EzPacker/EzMir/include/Operand/MirOperands.h#L318)**: Base-plus-displacement addressing mode: `[base + index * scale + displacement]`.
  - **[`MirReference`](file:///E:/Repos/EzPacker/EzMir/include/Operand/MirOperands.h#L119)**: Symbolic address reference classified by [`MirReferenceType`](file:///E:/Repos/EzPacker/EzMir/include/Operand/MirOperands.h#L27):
    - `Block`: Basic block label (branch/jump target).
    - `Function`: Function entry point (call target).
    - `GlobalVar`: Relocatable global variable.
    - `StackFrameObject`: Local stack frame slot resolved during frame lowering.
  - **[`MirRuntimeSymbol`](file:///E:/Repos/EzPacker/EzMir/include/Operand/MirOperands.h#L199)**: External link-time symbol from runtime libraries (`@__divdi3`, `@memcpy`).

---

### 4. Type System & Layout (`EzMir/include/Type/`)

* **[`MirTypeTable`](file:///E:/Repos/EzPacker/EzMir/include/Type/MirType.h)**:
  - Canonical interned type registry allocating from PMR arena memory.
  - Guarantees pointer equality ($T_1 == T_2 \iff \&T_1 == \&T_2$) for all canonical types.
  - Primitives:
    - Integers: `i1`, `i8`, `i16`, `i32`, `i64`, `i128`.
    - Floats: `f16`, `f32`, `f64`, `f128`.
    - Void type: `void`.
  - Derived types:
    - Pointers: `getPtr(baseType)` (with target address space).
    - Arrays: `getArray(elementType, length)`.
    - Functions: `getFuncType(returnType, parameterTypes)`.
* **[`IMirTargetTypeLayout`](file:///E:/Repos/EzPacker/EzMir/include/Type/MirType.h)**:
  - Abstract layout interface queried by the type table and backend passes to obtain target-specific byte sizes, natural alignments, and struct padding rules.

---

### 5. SSA Construction & Dominance Algorithms (`EzMir/include/MirPasses/Passes/`)

`EzMir` provides complete Single Static Assignment (SSA) infrastructure implementing the **Cytron, Ferrante, Rosen, Wegman, and Zadeck** algorithm:

* **Dominance Tree Calculation**:
  - Implements the **Cooper, Harvey, and Kennedy** immediate dominator algorithm (`buildImmDomTree()`).
  - Computes reverse post-order traversals and assigns dense post-order indexes to basic blocks.
* **Dominance Frontier & Iterated Dominance Frontier ($\text{IDF}$)**:
  - Computes dominance frontiers ($\text{DF}$) for every basic block (`buildDominanceFrontier()`).
  - For each virtual register with multiple definition sites (`m_defSites`), computes the iterated dominance frontier ($\text{IDF}$) to place minimal $\phi$-nodes (`insertPhiNodes()`).
* **Variable Renaming Stack**:
  - Traverses the dominator tree recursively (`renameVariables()`).
  - Maintains version stacks for each original variable, rewriting operand uses to dominant version names and generating unique versioned SSA registers.
* **[`NonSsaToSsaPass`](file:///E:/Repos/EzPacker/EzMir/include/MirPasses/Passes/NonSsaToSsaPass.h)**:
  - Encapsulates SSA conversion as a clean transformation pass registered in [`MirPassManager`](file:///E:/Repos/EzPacker/EzMir/include/MirPasses/MirPassManager.h).

---

### 6. Built-in Middle-End Passes

* **[`MirPassManager`](file:///E:/Repos/EzPacker/EzMir/include/MirPasses/MirPassManager.h)**:
  - Dependency-driven pass orchestrator.
  - Automatically sorts passes topologically based on declared analysis requirements.
  - Caches analysis results and handles pass invalidation.
* **[`CodeFlowAnalysisPass`](file:///E:/Repos/EzPacker/EzMir/include/MirPasses/Passes/CodeFlowAnalysisPass.h)**:
  - Inspects block terminator instructions (`JMP`, `BR`, `RET`) to construct the Control Flow Graph (CFG).
  - Populates predecessor and successor adjacency lists for each basic block.
  - Detects unreachable blocks and back-edges for loop identification.
* **[`LivenessAnalysisPass`](file:///E:/Repos/EzPacker/EzMir/include/MirPasses/Passes/LivenessAnalysisPass.h)**:
  - Solves backward dataflow equations over the CFG using [`DenseBitSet`](file:///E:/Repos/EzPacker/EzCore/include/HelperClasses/DenseBitSet.h):
    $$\text{LiveIn}[B] = \text{Use}[B] \cup (\text{LiveOut}[B] \setminus \text{Def}[B])$$
    $$\text{LiveOut}[B] = \bigcup_{S \in \text{succ}(B)} \text{LiveIn}[S]$$
  - Computes precise live ranges and live-in/live-out sets per basic block for downstream register allocation.

---

### 7. ABI & Calling Convention Subsystem (`EzMir/include/Function/`)

* **[`CallingConvDesc`](file:///E:/Repos/EzPacker/EzMir/include/Function/CallingConvDesc.h)**:
  - Abstract specification of an ABI calling convention:
    - Stack growth direction (`DOWN` or `UP`) and stack alignment requirements (e.g. 16-byte alignment).
    - Caller-saved (scratch) and callee-saved (preserved) physical register sets.
    - Dedicated registers: frame pointer (`RBP`/`FP`), stack pointer (`RSP`/`SP`), return address pointer.
    - Shadow space allocation (e.g. 32 bytes for Microsoft x64 ABI).
  - Argument location mapping via [`ArgumentLocationDesc`](file:///E:/Repos/EzPacker/EzMir/include/Function/ArgumentLocationDesc.h): assigns arguments to physical registers, stack slots, split structs, or indirect memory pointers (`ByVal`).
* **[`MirFunctionStackFrame`](file:///E:/Repos/EzPacker/EzMir/include/Function/MirFunctionStackFrame.h)**:
  - Models function stack frame layout: local variables, spilled registers, callee-saved preservation areas, and parameter passing areas.
  - Assigns layout offsets resolved during frame lowering.
* **[`CallLoweringState`](file:///E:/Repos/EzPacker/EzMir/include/Function/CallLoweringState.h)**:
  - Tracks transient scratch state and register availability across call sequences.

---

### 8. Textual MIR Serialization & Parsing (`EzMir/include/Printer/`, `Parser/`)

* **[`MirPrinter`](file:///E:/Repos/EzPacker/EzMir/include/Printer/MirPrinter.h)**:
  - High-performance disassembler formatting in-memory functions, blocks, instructions, and operands into clean, human-readable textual `.mir`.
  - Configurable detail levels: `MirPrinterDetail::General` (clean assembly-like listings) vs `MirPrinterDetail::Detailed` (verbose metadata, types, and register class annotations).
* **[`MirParser`](file:///E:/Repos/EzPacker/EzMir/include/Parser/MirParser.h)**:
  - Complete, round-trippable parser for textual `.mir` files.
  - Implemented with **Lexy** zero-copy scanning ([`MirLexer`](file:///E:/Repos/EzPacker/EzMir/include/Parser/MirLexer.h), [`MirParserContext`](file:///E:/Repos/EzPacker/EzMir/include/Parser/MirParserContext.h), [`MirAstNodes.h`](file:///E:/Repos/EzPacker/EzMir/include/Parser/MirAstNodes.h)).
  - Configured via [`MirParserOptions`](file:///E:/Repos/EzPacker/EzMir/include/Parser/MirParser.h#L27) (`verifySsa`, `allowTargetInstructions`, `enableLogging`, `maxErrors`).
  - Error contract: reports syntax/semantic errors cleanly through [`DiagnosticCollector`](file:///E:/Repos/EzPacker/EzCore/include/Diagnostics/DiagnosticCollector.h); never lets exceptions escape parser boundaries.

---

## Working Code Examples

### 1. Programmatically Building an SSA Function

```cpp
#include "Builder/MirBuilderContext.h"
#include "Function/MirFunctionBuilder.h"
#include "Block/MirBlockBuilder.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Type/MirType.h"
#include "Printer/MirPrinter.h"
#include <iostream>

void buildSimpleFunction()
{
    std::pmr::monotonic_buffer_resource arena;
    DiagnosticCollector diags;
    MirTypeTable types(&arena);

    // 1. Initialize context with System V ABI convention
    CallingConvDesc *cc = nullptr; // Resolved via target or test mock
    MirBuilderContext ctx(cc, &diags, &types, &arena);

    // 2. Build function: i32 max(i32 a, i32 b)
    MirType *i32 = types.i32();
    MirFunctionBuilder fnBuilder(&ctx);
    MirFunction *fn = fnBuilder.buildParam(i32, "a")
                               .buildParam(i32, "b")
                               .build(i32, "max");

    // 3. Create basic blocks: entry, then_block, else_block, merge_block
    MirBlockBuilder blkBuilder(&ctx, fn);
    MirBlock *entryBlock = blkBuilder.build(nullptr, "entry");
    MirBlock *thenBlock  = blkBuilder.build(nullptr, "then");
    MirBlock *elseBlock  = blkBuilder.build(nullptr, "else");
    MirBlock *mergeBlock = blkBuilder.build(nullptr, "merge");

    auto params = fn->getParameters();
    MirRegister *regA = params[0];
    MirRegister *regB = params[1];

    // 4. Emit Entry: compare a > b
    MirInstructionBuilder ib(&ctx, entryBlock, InsertionType::AppendToEnd);
    MirRegister *cmpRes = ib.createVReg(types.i1(), "cmp");
    ib.CMP_GT(cmpRes, regA, regB);
    ib.BR(cmpRes, thenBlock, elseBlock);

    // 5. Emit Then: jump to merge
    ib.setBlock(thenBlock, InsertionType::AppendToEnd);
    ib.JMP(mergeBlock);

    // 6. Emit Else: jump to merge
    ib.setBlock(elseBlock, InsertionType::AppendToEnd);
    ib.JMP(mergeBlock);

    // 7. Emit Merge: phi(then: a, else: b) and return
    ib.setBlock(mergeBlock, InsertionType::AppendToEnd);
    MirRegister *res = ib.createVReg(i32, "res");
    ib.PHI(res, regA, thenBlock, regB, elseBlock);
    ib.RET(res);

    // 8. Disassemble to textual MIR
    std::cout << MirPrinter::printToString(fn, MirPrinterDetail::General) << "\n";
}
```

### 2. Ingesting Textual MIR with MirParser

```cpp
#include "Builder/MirBuilderContext.h"
#include "Parser/MirParser.h"
#include "Diagnostics/DiagnosticCollector.h"
#include <iostream>

void parseMirModule()
{
    std::pmr::monotonic_buffer_resource arena;
    DiagnosticCollector diags;
    MirTypeTable types(&arena);
    MirBuilderContext ctx(nullptr, &diags, &types, &arena);

    std::string_view mirSource = R"(
        func @add(i32 %a, i32 %b) -> i32 {
        entry:
            %sum = ADD i32 %a, %b
            RET i32 %sum
        }
    )";

    EzMir::MirParserOptions options;
    options.verifySsa = true;
    EzMir::MirParser parser(&ctx, &diags, options);

    if (parser.parseModule(mirSource, "sample.mir"))
    {
        std::cout << "Successfully parsed MIR module!\n";
    }
}
```

---

## Testing & CMake Integration

### Linking EzMir
```cmake
target_link_libraries(YourTarget PRIVATE EzMir EzCore)
target_include_directories(YourTarget PRIVATE ${EZPACKER_ROOT}/EzMir/include)
```

### Subproject Test Suite
The tests for `EzMir` reside in [`tests/EzMirTestSuite/tests/`](file:///E:/Repos/EzPacker/tests/EzMirTestSuite/tests/):
- [`T_Block.cpp`](file:///E:/Repos/EzPacker/tests/EzMirTestSuite/tests/T_Block.cpp): Basic block construction, instruction appending, and label resolution.
- [`T_Instruction.cpp`](file:///E:/Repos/EzPacker/tests/EzMirTestSuite/tests/T_Instruction.cpp): Instruction creation, opcode classification, flag querying, and operand mutation.
- [`T_Operand.cpp`](file:///E:/Repos/EzPacker/tests/EzMirTestSuite/tests/T_Operand.cpp): Virtual/physical registers, `FlexInt`/`FlexFloat` numeric literals, symbolic references, and memory operands.
- [`T_Function.cpp`](file:///E:/Repos/EzPacker/tests/EzMirTestSuite/tests/T_Function.cpp): Function building, parameter lists, and return types.
- [`T_GlobalVar.cpp`](file:///E:/Repos/EzPacker/tests/EzMirTestSuite/tests/T_GlobalVar.cpp): Global variable constant definitions, alignment, and linkages.
- [`T_CodeFlowPass.cpp`](file:///E:/Repos/EzPacker/tests/EzMirTestSuite/tests/T_CodeFlowPass.cpp): CFG construction, predecessor/successor maps, and loop back-edges.
- [`T_NonSsaToSsa.cpp`](file:///E:/Repos/EzPacker/tests/EzMirTestSuite/tests/T_NonSsaToSsa.cpp): Dominator tree computation, iterated dominance frontiers, $\phi$-node insertion, and variable renaming.
- [`T_LivenessAnalysis.cpp`](file:///E:/Repos/EzPacker/tests/EzMirTestSuite/tests/T_LivenessAnalysis.cpp): `def`/`use` calculation, backward dataflow equations, and `liveIn`/`liveOut` sets.
- [`T_MirParser.cpp`](file:///E:/Repos/EzPacker/tests/EzMirTestSuite/tests/T_MirParser.cpp): Textual `.mir` syntax ingestion, error recovery, and round-tripping.
- [`T_MirRegisterDescriptor.cpp`](file:///E:/Repos/EzPacker/tests/EzMirTestSuite/tests/T_MirRegisterDescriptor.cpp): Hardware register descriptions, sub-part hierarchies, and hardware encodings.