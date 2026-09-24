# Plan Part 4: Comprehensive Guide: Adding a Target Architecture

## 1. Objective
Author an exhaustive, definitive tutorial and architectural guide on **How to Add a New Target Architecture** to EzPacker (`docs/pages/adding_a_target.md`), using an end-to-end case study (e.g., RISC-V 64 / ARM64 compared against the x86-64 reference implementation) spanning declarative DSL modeling, generated stub integration, C++ lowering and emission components, target registration, and verification.

---

## 2. Deliverables & File Locations

| File | Purpose |
|:---|:---|
| [`docs/pages/adding_a_target.md`](file:///E:/Repos/EzPacker/docs/pages/adding_a_target.md) | Dedicated retargeting manual integrated into Doxygen via `@page adding_a_target Adding a New Target Architecture`. |

---

## 3. Detailed Technical Content Outline

### A. The EzPacker Retargeting Philosophy
- Architectural separation between target-independent middle-end (`EzMir`), declarative domain-specific specifications (`EzDsl`), and concrete architecture backends (`EzTargets`).
- Target repository layout standards:
  ```
  EzTargets/
  └── <Arch>/
      ├── CMakeLists.txt
      ├── include/
      │   ├── <Arch>TargetDesc.h
      │   ├── <Arch>FrameLowerer.h
      │   ├── <Arch>TargetInstructionSelector.h
      │   ├── <Arch>RelocationResolver.h
      │   └── <Arch>CodeEmitter.h
      ├── src/
      │   ├── <Arch>TargetDesc.cpp
      │   ├── <Arch>FrameLowerer.cpp
      │   ├── <Arch>TargetInstructionSelector.cpp
      │   ├── <Arch>RelocationResolver.cpp
      │   ├── <Arch>CodeEmitter.cpp
      │   ├── <Arch>ElfBinaryDesc.cpp
      │   └── <Arch>CoffBinaryDesc.cpp
      ├── Registration/
      │   └── <Arch>TargetRegistration.cpp
      └── targets/
          └── <arch>/
              ├── <arch>.tdesc
              ├── <arch>_instructions.idf
              ├── <arch>_calling_conv.ezcc
              ├── <arch>_legalize.lad
              ├── <arch>_rules.lrd
              └── <arch>_patterns.isf
  ```

---

### B. Phase 1: Declarative Target Modeling (`EzDsl`)
1. **Target Descriptor (`.tdesc`)**:
   - Defining target metadata: `target <arch>`, `pointer_size`, `stack_slot`, `instruction_pointer`, `mem_disp_type`.
   - Declaring supported object formats (`elf64`, `coff`).
   - Defining inline register banks:
     - Declaring register classes (`gpr64`, `gpr32`, `fpr64`, `vr128`).
     - Sub-register hierarchy relationships (`<:`).
     - Physical register encodings (`enc`) and display names (`names`).
     - Special architectural registers (`sp`, `fp`, `ip`, `zero`, `lr`).
2. **Instruction Definitions (`.idf`)**:
   - Instruction grammar: `target_inst <mnemonic> { ... }`.
   - Operand definitions with directional flow (`IN`, `OUT`, `INOUT`).
   - Opcode behavioral flags (`IsCommutative`, `IsBranch`, `IsCall`, `IsReturn`, `ReadsMemory`, `WritesMemory`, `HasSideEffect`).
   - Implicit register definitions (`IMPLICIT_DEFS`) and uses (`IMPLICIT_USES`).
3. **Calling Convention Specifications (`.ezcc`)**:
   - Stack configuration: `growth downward`, `align 16`, `shadow_space`, `red_zone`.
   - Register preservation sets: `callee_saved`, `caller_saved`.
   - Classification rules: primitive types, structs, floating-point arguments.
   - Pass & return strategies: argument registers, stack fallback, return registers.
4. **Legalization Directives & Rules (`.lad` / `.lrd`)**:
   - Declaring operation legality: `LEGAL`, `WIDENS`, `NARROWS`, `LIBCALL`, `CUSTOM`, `UNSUPPORTED`.
   - Scalar clamping: `CLAMP_SCALAR i32, i64`.
   - Rewrite rules: splitting 128-bit operations into 64-bit pairs with carry/borrow.
5. **Instruction Selection Patterns (`.isf`)**:
   - Pattern matching tree structures: `match (ADD gpr64:$lhs, gpr64:$rhs) -> select (ADD_RR ...)`.
   - Addressing mode definitions (`addrmode base_disp ...`).
   - Immediate constraints: `simm12`, `uimm20`, `imm_zero`.
   - Cost-based matching and variant selection.

---

### C. Phase 2: Code Generation & Build Integration
- Running `ezdsl` or invoking the CMake helper macro:
  ```cmake
  ezdsl_target(
      TARGET_NAME EzTargets<Arch>
      TDESC targets/<arch>/<arch>.tdesc
      IDF targets/<arch>/<arch>_instructions.idf
      EZCC targets/<arch>/<arch>_calling_conv.ezcc
      LAD targets/<arch>/<arch>_legalize.lad
      LRD targets/<arch>/<arch>_rules.lrd
      ISF targets/<arch>/<arch>_patterns.isf
      OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated
  )
  ```
- Generated C++ interfaces:
  - `<Arch>TargetInstructionTable.h`
  - `<Arch>InstructionSelector.h` (base selector class)
  - `<Arch>RegisterInfo.h`
  - `<Arch>CallingConvTable.h`
  - `<Arch>LegalizerInfo.h`

---

### D. Phase 3: Implementing Concrete C++ Backend Classes
1. **Concrete Target Descriptor (`<Arch>TargetDesc`)**:
   - Subclassing `TargetDesc`.
   - Instantiating and initializing register banks, register classes, and calling conventions.
   - Overriding factory methods: `getFrameLowerer()`, `getInstructionSelector()`, `getLegalizer()`, `createCodeEmitter()`.
2. **Target Instruction Selector (`<Arch>TargetInstructionSelector`)**:
   - Inheriting from the generated base selector `<Arch>InstructionSelector`.
   - Implementing `select(ctx, inst)`: dispatching to generated selector first, then falling back to hand-written selection for complex patterns, floating-point math, vector operations, and control-flow branches.
3. **Frame Lowering (`<Arch>FrameLowerer`)**:
   - Computing stack frame layout: local variable slots, spill slots, parameter areas, alignment padding.
   - Emitting function prologue: saving frame pointer/link register, decrementing stack pointer, pushing callee-saved registers.
   - Emitting function epilogue: popping callee-saved registers, incrementing stack pointer, restoring frame pointer/link register, emitting return instruction.
4. **Machine Code Emitter & Instruction Encoder (`<Arch>CodeEmitter`)**:
   - Inheriting from `GenericCodeEmitter`.
   - Converting selected target `MirInstruction` nodes into machine code byte streams inside `CodeSection` via `DataNode`, `LabelNode`, and `AlignNode`.
5. **Relocation Resolver (`<Arch>RelocationResolver`)**:
   - Handling intra-section and inter-section relocation fixups (branch displacements, PC-relative data references).
6. **Object Binary Descriptors (`<Arch>ElfBinaryDesc` & `<Arch>CoffBinaryDesc`)**:
   - Target machine constants (`EM_RISCV`, `EM_AARCH64`, `IMAGE_FILE_MACHINE_ARM64`).
   - Relocation type translation for ELF and COFF.

---

### E. Phase 4: Registration, Triple Binding, and Verification
1. **Target Registration**:
   - Implementing `registerTarget()` calling `EzCompiler::TargetResolver::registerTarget("<arch>", factory)`.
   - Static initializer hook to register during executable startup.
2. **EzTriple Support**:
   - Adding target architecture identifier to `TargetTriple`.
3. **Unit & End-to-End Testing**:
   - Writing test fixtures in `tests/EzTargets<Arch>TestSuite`.
   - Validating execution through `CompilationPipeline`.
   - Comparing generated machine code with reference disassemblers.

---

## 4. Verification & Acceptance Criteria
1. The guide provides complete, practical C++ and DSL code snippets reflecting the real `TargetDesc`, `MirFrameLowerer`, and `TargetResolver` APIs.
2. Cross-references link directly to Doxygen symbol documentation.
3. The page renders with syntax-highlighted code blocks, architecture flowcharts, and clear step-by-step headings.
