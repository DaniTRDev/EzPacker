# EzTriple Subproject Documentation

[EzPacker Documentation Index](../index.md) > **EzTriple**

---

## 1. Overview & Architectural Role

`EzTriple` is the target-independent backend lowering and machine transformation engine of EzPacker. It sits between middle-end SSA MIR and the final binary code emitter.

EzTriple executes the critical five-stage lowering pipeline that transforms generic, abstract MIR into concrete, hardware-mapped machine instructions:
1. **Legalization**: Rewriting unsupported types and opcodes.
2. **ABI Lowering**: Translating abstract argument/return tokens into calling-convention registers and stack slots.
3. **Instruction Selection**: Replacing generic operations with target hardware instructions via Bottom-Up Maximal Munch.
4. **Register Allocation**: Assigning physical hardware registers to virtual registers using Chaitin-Briggs Graph Coloring.
5. **Frame Lowering**: Calculating stack frame layout and inserting target function prologues and epilogues (PEI).

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

#### 3-Tier Architecture (`LegalizerInfo.h`)
- **Tier 1 (Dense Primary Matrix)**: An $O(1)$ lookup table of size `OPCODE_COUNT × MAX_COMPACT_TYPES`. Queries return a `LegalityResponse` with an action kind:
  - `Legal`: Supported directly by target hardware.
  - `WidenScalar`: Promotes to a wider integer type (e.g. `i1` -> `i32`).
  - `NarrowScalar`: Splits into multiple smaller operations (e.g. `i128` -> two `i64`).
  - `Lower`: Rewrites into other generic opcodes.
  - `Libcall`: Calls a runtime software emulation function (e.g. `__divti3`).
  - `Custom`: Dispatches to a target-specific C++ callback.
  - `Unsupported`: Reports a compilation error.
- **Tier 2 (Signature Matchers)**: Evaluates multi-slot operations with differing input/output types (e.g. type conversions, truncated loads).
- **Tier 3 (Declarative Rewrite Rules)**: Evaluates algebraic and strength-reduction rules defined in `.lrd` files (e.g. converting division by power-of-two to an arithmetic right shift).

#### Built-In Action Handlers (`include/Legalizer/Actions/`)
- `LegalizeWidenScalarAction`: Generates extension instructions and bit masks.
- `LegalizeNarrowScalarAction`: Decomposes large operations using `MERGE_VALUES` and `UNMERGE_VALUES`.
- `LegalizeBitcastAction`: Reinterprets bit representations without conversion.
- `LegalizeLibcallAction`: Emits function calls to standard compiler-rt / libgcc routines.

---

### 2.2 ABI Lowerer (`include/AbiLowerer/`)
Translates high-level procedural boundaries into target calling convention rules:
- **Token-Bound Sequences**:
  - `PUSH_ARG` + `CALL`: Binds arguments to caller registers (`rdi`, `rsi`... on Linux; `rcx`, `rdx`... on Windows) or pushes them to stack slots.
  - `POP_RET`: Retrieves return values from `rax` / `xmm0`.
  - `POP_ARG` + `END_ARG`: Extracts function parameters at the entry block into local virtual registers.
  - `PUSH_RET` + `RET`: Places return values into return registers.
- **Shadow Space & Red Zone**:
  - Allocates 32 bytes of shadow space (homing space) for Microsoft Win64.
  - Protects the 128-byte red zone under the stack pointer for System V AMD64.
- **Caller-Saved Clobber Tracking**: Identifies all caller-saved registers clobbered by calls and marks them dead or preserved.

---

### 2.3 Instruction Selector (`include/InstructionSelector/`)
Converts generic MIR opcodes into concrete hardware instructions:
- **Bottom-Up Maximal Munch**:
  Traverses the instructions in each basic block in reverse order (bottom-up). Matches the largest possible subtrees against patterns generated from `.isf` files.
- **Addressing Mode Matcher (`MirAddressingModeMatcher.h`)**:
  Identifies memory access expressions and synthesizes complex addressing modes:
  ```text
  Effective Address = Base + (Index * Scale) + Displacement
  ```
- **Load-Folding Optimization**:
  Automatically folds a load instruction into a consuming ALU instruction (e.g. `ADD dst, src1, (LOAD addr)` -> `ADD64rm dst, src1, addr`) provided the load result has a single use and no intervening store modifies the memory location.
- **Register Class Assignment**:
  Assigns `MirRegisterClass*` constraints to virtual register operands based on target instruction operand descriptors.

---

### 2.4 Register Allocator (`include/RegisterAllocator/`)
EzTriple implements a production-grade **Chaitin-Briggs Graph-Coloring Register Allocator**:

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
              | Any uncolorable spill nodes?    |
              +---------------------------------+
                /                              \
        [Yes]  /                                \  [No]
              v                                  v
    +------------------------+        +------------------------+
    | Spill & Rewrite:       |        | Coalesce:              |
    | - Assign Stack Slot    |        | Eliminate redundant    |
    | - Insert Store & Loads |        | copy instructions      |
    +------------------------+        +------------------------+
              |                                  |
              v                                  v
      Restart Allocation               Allocation Complete!
```

1. **Interference Graph Construction**: Nodes represent virtual registers; undirected edges represent overlapping live ranges computed by `LivenessAnalysisPass`.
2. **Spill Cost Calculation**: Computes the cost of spilling each register based on loop nesting depth (10^depth) and instruction count.
3. **Simplify Phase**: Removes nodes with degree < K (where K is the count of allocatable physical registers in that register class) and pushes them onto `m_selectStack`.
4. **Optimistic Spill Phase**: When all remaining nodes have degree >= K, selects the node with the lowest spill cost and pushes it to the stack.
5. **Select Phase**: Pops nodes from the stack in reverse order and assigns the first available physical register that does not conflict with already-colored neighbors.
6. **Spill & Rewrite Phase**: If an optimistic spill cannot be colored, an actual stack slot (`StackFrameObject`) is allocated. Stores are inserted immediately after definitions, loads are inserted before uses, and the allocation loop repeats.
7. **Coalescing**: Identifies register copies (`MOV %vreg1, %vreg2`) whose live ranges do not interfere and merges them into a single virtual register, eliminating redundant copy instructions.

---

### 2.5 Frame Lowerer (`include/FrameLowerer/`)
Responsible for Prologue/Epilogue Insertion (PEI) and stack frame layout:
- **`calculateFrameLayout(FrameLowererCtx &ctx)`**:
  - Aggregates local variables, spilled registers, and parameter save areas.
  - Aligns the stack frame to 16 bytes.
  - Computes final positive/negative offsets from the frame pointer (`rbp`) or stack pointer (`rsp`).
- **`insertPrologue(FrameLowererCtx &ctx)`**:
  - Emits frame pointer setup (`push rbp; mov rbp, rsp`).
  - Emits stack pointer adjustment (`sub rsp, FrameSize`).
  - Emits saves for all callee-saved registers clobbered by the function.
- **`insertEpilogue(FrameLowererCtx &ctx)`**:
  - Emits restores for callee-saved registers in reverse order.
  - Emits stack pointer reset (`mov rsp, rbp` or `add rsp, FrameSize`).
  - Emits `pop rbp; ret`.
- **`lowerAlloc` & `lowerDAlloc`**:
  - Replaces abstract `ALLOC` instructions with effective address calculations (e.g. `LEA rsp + offset`).

---

## 3. Architecture Descriptors (`include/Descriptors/`)

- **`TargetDesc`**: Abstract interface for hardware CPUs (`getName()`, `getFrameLowerer()`, `getInstructionSelector()`, `getLegalizer()`, `getRegisterAllocator()`, `getAvailableRegisterBanks()`, `createCodeEmitter()`).
- **`TargetBinaryDesc`**: Represents the intersection of CPU and binary format (e.g. `X86_64ElfBinaryDesc`, `X86_64CoffBinaryDesc`).
- **`TargetExtensionSet`**: Tracks enabled CPU features (`+avx`, `+sse4.1`) and validates dependency implications.
- **`TargetRelocationResolver`**: Architecture-specific handler for patching in-place relocations.

---

## 4. API Reference & Further Reading

- Generated Doxygen API documentation: [Doxygen Documentation Index](../doxygen/index.html)
- Next subproject: [EzCompiler Subproject Documentation](EzCompiler.md)
- Return to [EzPacker Landing Page](../index.md)
