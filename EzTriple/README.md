# EzTriple: Target Architecture, ABI & Backend Code Generation Engine

`EzTriple` is the target architecture, hardware description, and backend lowering library of the **EzPacker** compiler toolchain. It bridges target-independent Mid-Level Intermediate Representation (`EzMir`) with concrete target architectures (such as AMD64/x86-64, AArch64, and RISC-V), orchestrating:

1. **Hardware & Binary Target Descriptors**: Target CPU abstractions (`TargetDesc`) and OS/ABI object container specifications (`TargetBinaryDesc`).
2. **ABI & Calling Convention Lowerer**: Lowers unlowered argument/return tokens (`PUSH_ARG`/`CALL`, `PUSH_RET`/`RET`, `POP_ARG`) into physical calling convention registers and stack parameters.
3. **Chaitin-Briggs Graph-Coloring Register Allocator**: Interference graph construction from liveness analysis, degree simplification, optimistic spilling, rematerialization, and register rewriting.
4. **Prologue/Epilogue & Stack Frame Lowerer**: Stack layout calculation, callee-saved register preservation, ABI shadow space, and lowering abstract stack references into concrete base-plus-displacement memory operands (`[RBP/RSP + offset]`).

---

## Architecture Overview

```
                          ┌───────────────────────────┐
                          │    Generic / ISel MIR     │
                          └─────────────┬─────────────┘
                                        │
                         ┌──────────────▼──────────────┐
                         │      MirAbiLowererPass      │
                         │ (Lowers CALL / RET / ARGS)  │
                         └──────────────┬──────────────┘
                                        │
                         ┌──────────────▼──────────────┐
                         │   LivenessAnalysisPass      │
                         │ (Computes Live-In/Live-Out) │
                         └──────────────┬──────────────┘
                                        │
                         ┌──────────────▼──────────────┐
                         │  MirRegisterAllocatorPass   │
                         │ (Chaitin-Briggs Allocation) │
                         └──────────────┬──────────────┘
                                        │
                         ┌──────────────▼──────────────┐
                         │    MirFrameLowererPass      │
                         │ (Prologue / Epilogue / PEI) │
                         └──────────────┬──────────────┘
                                        │
                         ┌──────────────▼──────────────┐
                         │     Target Code Emitter     │
                         │       (EzCodeEmitter)       │
                         └─────────────────────────────┘
```

---

## Core Subsystems

### 1. Target & Binary Descriptors (`EzTriple/include/Descriptors/`)

Target specifications are decoupled into CPU-level architecture and OS-level binary container descriptors:

- **`TargetDesc`**: Hardware CPU-level descriptor.
  - Exposes target-specific components: `IMirTargetTypeLayout`, `MirFrameLowerer`, `MirRegisterAllocator`, `MirInstructionSelector`, `MirLegalizer`, and `MirExpansionRuleRegistry`.
  - Defines fundamental hardware properties: `getInstructionPtrReg()`, `getStackSlotSize()`, and `getMemOperandDisplacementType()`.
  - Provides lists of available register banks (`MirRegisterBank`) and calling conventions (`CallingConvDesc`).
- **`TargetBinaryDesc`**: OS/ABI-level binary container descriptor.
  - Encapsulates target object file format: `TargetObjectFormat` (`ELF`, `COFF`, `MachO`).
  - Position-independence settings (`PIC`/`PIE`) and endianness (`isLittleEndian()`).
  - Alignment rule for functions (`getFunctionAlignment()`, honored during `.text` emission).
  - Pre-allocated section map (`getSections()`) mapping `SectionType` to concrete `CodeSection` instances.

---

### 2. ABI & Calling Convention Lowering (`EzTriple/include/AbiLowerer/`)

The ABI Lowering subsystem transforms abstract parameter/return operations into machine-level data movements dictated by the target `CallingConvDesc`:

- **Unlowered Binding Sequence Model**:
  - Functions emit unlowered parameter blocks (`POP_ARG` + `END_ARG`), call sequences (`PUSH_ARG` + `CALL`), and returns (`PUSH_RET` + `RET`) bound by unique tokens.
- **`MirAbiLowerer`**:
  - `processFunctionArguments()`: Lowers incoming parameters into virtual registers or stack parameter loads.
  - `processCallBlock()`: Copies arguments into target physical parameter registers and manages caller-allocated stack argument slots.
  - `processReturnBlock()`: Moves return values into designated return registers (e.g. `RAX`, `RDX`, `XMM0`) or handles indirect SRET (Structure Return) buffer pointers.
  - `processCallReturnBlock()`: Extracts return values post-call from physical registers.
- **`MirAbiLowererPass`**:
  - Implements `IMirTransformPass` iterating over functions to identify and lower all ABI boundary sequences.

---

### 3. Graph-Coloring Register Allocator (`EzTriple/include/RegisterAllocator/`)

`EzTriple` implements a global **Chaitin-Briggs style graph-coloring register allocator**:

```
      ┌─────────────────────────────────────────────────────────────┐
      │                   Build Interference Graph                  │
      │   (Traverse instructions & liveness to populate m_iGraph)   │
      └──────────────────────────────┬──────────────────────────────┘
                                     │
      ┌──────────────────────────────▼──────────────────────────────┐
      │                       Simplify Graph                        │
      │    (Remove nodes with degree < K and push to selectStack)   │
      └──────────────────────────────┬──────────────────────────────┘
                                     │
      ┌──────────────────────────────▼──────────────────────────────┐
      │                        Select Colors                        │
      │   (Pop stack and assign available physical register colors) │
      └──────────────┬───────────────────────────────┬──────────────┘
                     │ Success                       │ Spill Needed
                     │                               ▼
                     │                 ┌────────────────────────────┐
                     │                 │     Optimistic Spilling    │
                     │                 │   (Allocate spill slots,   │
                     │                 │    emit spill & reloads)   │
                     │                 └─────────────┬──────────────┘
                     │                               │
                     │                               ▼
                     │                 ┌────────────────────────────┐
                     │                 │ Rebuild Interference Graph │
                     │                 └────────────────────────────┘
                     ▼
      ┌─────────────────────────────────────────────────────────────┐
      │                   Rewrite Register Colors                   │
      │   (Replace virtual register operands with physical colors)  │
      └─────────────────────────────────────────────────────────────┘
```

- **`RegisterAllocatorCtx`**:
  - Owns per-function allocation state using `std::pmr::memory_resource`.
  - Tracks the interference graph (`m_iGraph`), node degrees (`m_degree`), select stack (`m_selectStack`), assigned physical registers (`m_allocatedRegs`), and allocated spill slots (`m_spilledRegs`).
- **`MirRegisterAllocator`**:
  - `buildInterferenceGraph()`: Ingests `LivenessResult` (live-in/live-out sets) to connect concurrently live virtual and physical registers.
  - `simplify()`: Iteratively peels non-constrained nodes ($\text{degree} < K_{\text{class}}$) onto the coloring stack.
  - `selectColors()`: Pops nodes in reverse order, selecting the first available hardware register color that does not conflict with neighbor assignments.
  - `calculateSpillCost()` / `rewriteSpilledRegisters()`: Emits target reload (`emitReload`), spill store (`emitSpill`), or rematerialization (`reMaterialize`) operations when registers must be spilled to stack memory.
  - `rewriteColors()`: Replaces all allocated virtual register operands with physical register references.
- **`MirRegisterAllocatorPass`**: Transform pass integrating the allocator into `MirPassManager`.

---

### 4. Prologue/Epilogue & Frame Lowering (`EzTriple/include/FrameLowerer/`)

The Frame Lowering subsystem performs Prologue/Epilogue Insertion (PEI) and resolves abstract stack layout offsets:

- **`FrameLowererCtx`**: Encapsulates the function, builder context, and target descriptor.
- **`MirFrameLowerer`**:
  - `calculateFrameLayout()`: Computes total stack frame size, aligning to ABI stack boundaries (e.g. 16-byte alignment), reserving callee-saved register save areas and ABI shadow spaces (e.g. 32-byte Windows x64 shadow store).
  - `insertPrologue()`: Emits target instructions setting up stack and frame pointers (e.g. `push rbp`, `mov rbp, rsp`, `sub rsp, frameSize`) and saving callee-saved registers.
  - `insertEpilogue()`: Emits instructions restoring callee-saved registers, resetting the stack pointer, and returning (`ret`).
  - `lowerAlloc()` / `lowerDAlloc()`: Lowers static and dynamic (`alloca`) stack allocations.
  - `lowerStackObjectReferences()`: Scans function instructions, rewriting abstract `MirReference(StackFrameObject)` operands into concrete base-plus-displacement memory operands (`MirMemory([FP/SP + offset])`).
- **`MirFrameLowererPass`**: Transform pass executing frame layout and insertion as the final step before code emission.

---

## Complete Pipeline Usage Example

```cpp
#include "Builder/MirBuilderContext.h"
#include "MirPasses/MirPassManager.h"
#include "MirPasses/Passes/CodeFlowAnalysisPass.h"
#include "MirPasses/Passes/LivenessAnalysisPass.h"
#include "AbiLowerer/MirAbiLowererPass.h"
#include "RegisterAllocator/MirRegisterAllocatorPass.h"
#include "FrameLowerer/MirFrameLowererPass.h"
#include "Descriptors/TargetDesc.h"
#include "Descriptors/TargetBinaryDesc.h"

void RunBackendPipeline(MirBuilderContext *context, TargetDesc *targetDesc)
{
    DiagnosticCollector *diagCollector = context->getDiagCollector();
    std::pmr::memory_resource *arena = context->getGlobalAllocator();

    // 1. Setup Pass Manager
    MirPassManager passManager(diagCollector, arena);

    // 2. Add Control Flow & ABI Lowering
    passManager.addPass<CodeFlowAnalysisPass>(context);
    passManager.addPass<MirAbiLowererPass>(context);

    // 3. Add Liveness Analysis & Register Allocation
    passManager.addPass<LivenessAnalysisPass>(context);
    passManager.addPass<MirRegisterAllocatorPass>(context, targetDesc);

    // 4. Add Prologue/Epilogue Insertion & Frame Lowering
    passManager.addPass<MirFrameLowererPass>(context, targetDesc);

    // 5. Generate and Run Pipeline
    passManager.generatePipeline();
    passManager.runPipeline(context);
}
```

---

## Building and Linking

`EzTriple` is built as a static CMake library.

```cmake
target_link_libraries(YourTarget PRIVATE EzTriple EzMir EzCore)
target_include_directories(YourTarget PRIVATE ${EZPACKER_ROOT}/EzTriple/include)
```
