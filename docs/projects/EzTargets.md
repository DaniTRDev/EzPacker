# EzTargets Subproject Documentation

[EzPacker Documentation Index](../index.md) > **EzTargets**

---

## 1. Overview & Architectural Role

`EzTargets` hosts architecture-specific backend target implementations as self-contained projects. Each target directory contains:
- Its runtime target descriptor (`TargetDesc`).
- Target DSL definitions (`.tdesc`, `.ezcc`, `.lad`, `.lrd`, `.idf`, `.isf`).
- Synthesized C++ lookup tables and pattern matchers.
- Build-time DSL plugins (e.g. custom encoding dialects and codegen backends).
- Target auto-registration hooks for `EzCompiler`.

Generic compiler infrastructure (`EzMir`, `EzTriple`, `EzCodeEmitter`, `EzDsl`) remains strictly target-agnostic; only `EzTargets` references concrete hardware architectures and instruction sets.

```
       +-------------------------------------------------------------+
       |                          EzTargets                          |
       +-------------------------------------------------------------+
                                       |
                                       v
               +-----------------------------------------------+
               |                 EzTargets/X86_64              |
               +-----------------------------------------------+
                 |                      |                    |
                 v                      v                    v
          +--------------+       +--------------+     +--------------+
          | Runtime Core |       |  DSL Targets |     | DSL Plugin   |
          | X86_64Target |       | x86_64.tdesc |     | Encoding     |
          | Lowerer, PEI |       | .ezcc, .lad  |     | Dialect      |
          | CodeEmitter  |       | .idf, .isf   |     | Codegen      |
          +--------------+       +--------------+     +--------------+
                 |                      |
                 +----------+-----------+
                            |
                            v
               +--------------------------+
               |  Target Registration     |
               |  "x86_64" -> Resolver    |
               +--------------------------+
```

---

## 2. The x86-64 Target (`EzTargets/X86_64`)

The primary target implemented in EzPacker is `X86_64` (AMD64), supporting 64-bit computation across Linux and Windows with SSE/AVX vector extensions.

### 2.1 Target Descriptor: `X86_64TargetDesc` (`include/X86_64TargetDesc.h`)
Implements `TargetDesc` to coordinate all x86-64 subcomponents:
- **Pointer Size**: 8 bytes (64 bits).
- **Stack Slot Size**: 8 bytes.
- **Instruction Pointer**: `rip`.
- **Memory Displacement Type**: `i64`.
- **Register Banks**:
  - `GPR`: 64-bit General-Purpose Registers (`rax`, `rcx`, `rdx`, `rbx`, `rsp`, `rbp`, `rsi`, `rdi`, `r8`..`r15`).
    - Sub-registers: `GPR64 <: GPR32 <: GPR16 <: GPR8`. For example, `rax` contains `eax` (32-bit), `ax` (16-bit), and `al` (8-bit).
  - `FPR`: 128-bit Vector / Floating-Point Registers (`xmm0`..`xmm15`).
    - Sub-registers: `VR128 <: FPR64 <: FPR32`.
- **CPU Extensions & Implication Graph**:
  - `sse` -> `sse2` -> `sse3` -> `ssse3` -> `sse4_1` -> `sse4_2` -> `avx` -> `avx2`.
  - Feature strings like `+avx` automatically enable all prerequisite SSE extensions.

---

### 2.2 Calling Conventions
Declared in `targets/x86_64/x86_64_calling_conv.ezcc`:

#### System V AMD64 (Linux / macOS / BSD)
- **Integer Argument Registers**: `rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9` (in order). Additional arguments pass on the stack.
- **Floating-Point Argument Registers**: `xmm0` through `xmm7`.
- **Return Registers**: `rax`, `rdx` (scalar integers); `xmm0`, `xmm1` (floating-point / vectors).
- **Callee-Saved Registers**: `rbx`, `rsp`, `rbp`, `r12`, `r13`, `r14`, `r15`.
- **Red Zone**: 128 bytes below `rsp` reserved for leaf functions without stack adjustment.

#### Microsoft Win64 (Windows)
- **Argument Registers (Slots 1-4)**:
  - Slot 1: `rcx` (integer) or `xmm0` (float/vector)
  - Slot 2: `rdx` (integer) or `xmm1` (float/vector)
  - Slot 3: `r8` (integer) or `xmm2` (float/vector)
  - Slot 4: `r9` (integer) or `xmm3` (float/vector)
- **Shadow Space**: 32 bytes (4 stack slots) allocated by the caller immediately above the return address.
- **Return Registers**: `rax` (scalar integer); `xmm0` (floating-point / vector).
- **Callee-Saved Registers**: `rbx`, `rbp`, `rdi`, `rsi`, `r12`, `r13`, `r14`, `r15`, `xmm6`..`xmm15`.

---

### 2.3 Frame Lowering: `X86_64FrameLowerer` (`include/X86_64FrameLowerer.h`)
- **Stack Alignment**: Enforces mandatory 16-byte stack alignment before all function calls.
- **Prologue Generation**:
  ```nasm
  push    rbp
  mov     rbp, rsp
  sub     rsp, FrameSize
  ; Save callee-saved registers
  mov     [rbp - 8], rbx
  mov     [rbp - 16], r12
  ```
- **Epilogue Generation**:
  ```nasm
  ; Restore callee-saved registers
  mov     r12, [rbp - 16]
  mov     rbx, [rbp - 8]
  mov     rsp, rbp
  pop     rbp
  ret
  ```
- **`ALLOC` Lowering**: Resolves local stack buffers into frame-relative offsets:
  `ALLOC 64` -> effective address `rbp - SlotOffset`.

---

### 2.4 Instruction Selection: `X86_64TargetInstructionSelector`
Drives Bottom-Up Maximal Munch using the decision trees generated from `x86_64_patterns.isf`:
- Recognizes 32-bit and 64-bit integer ALU instructions (`ADD64rr`, `SUB32ri`, `IMUL64rr`).
- Matches ModR/M and SIB memory addressing modes (`[base + index*scale + disp]`).
- Automatically folds memory loads into binary operations (`Select_ADD64rm`).

---

### 2.5 Machine Code Emission & Encoding

#### `X86_64InstructionEncoder` (`include/Encoding/X86_64InstructionEncoder.h`)
Encodes instructions into raw x86-64 machine bytes:
1. **Legacy Prefixes**:
   - `0x66`: Operand-size override (e.g. 16-bit operations).
   - `0x67`: Address-size override.
2. **REX Prefix (`0100WRXB`)**:
   - `REX.W`: 1 for 64-bit operand sizes.
   - `REX.R`: Extension of ModR/M `reg` field (accesses `r8`-`r15`, `xmm8`-`xmm15`).
   - `REX.X`: Extension of SIB `index` field.
   - `REX.B`: Extension of ModR/M `r/m` or SIB `base` field.
3. **Opcode Bytes**: 1-byte, 2-byte (`0x0F`), or 3-byte (`0x0F 0x38` / `0x0F 0x3A`) opcodes.
4. **ModR/M Byte**:
   - `mod` (2 bits): Addressing mode (register-direct `11`, displacement-only `00`, disp8 `01`, disp32 `10`).
   - `reg` / `opcode_digit` (3 bits): Register operand or opcode extension.
   - `r/m` (3 bits): Register or memory operand.
5. **SIB Byte (Scale-Index-Base)**: Emitted when memory operands require complex indexing or use `rsp`/`r12` as base.
6. **Displacement Bytes**: 8-bit sign-extended or 32-bit sign-extended displacements.
7. **Immediate Bytes**: 8-bit, 16-bit, 32-bit, or 64-bit immediate values.

#### `BranchRelaxer` (`include/BranchRelaxation/BranchRelaxer.h`)
Iterative branch relaxation pass:
- Inspects all conditional (`Jcc`) and unconditional (`JMP`) jump instructions.
- Measures the displacement from the jump site to the target basic block label.
- If the displacement fits within $[-128, +127]$, emits an 8-bit short jump (`EB cb` or `7x cb`).
- If the displacement exceeds the 8-bit limit, relaxes the instruction to a 32-bit near jump (`E9 cd` or `0F 8x cd`).
- Re-runs until all branch offsets stabilize.

---

### 2.6 Binary Descriptors & Relocations
- **`X86_64ElfBinaryDesc`**: Binds the target to the System V ELF64 format. Registers relocations:
  - `R_X86_64_64`: 64-bit absolute address.
  - `R_X86_64_PC32`: 32-bit PC-relative displacement.
  - `R_X86_64_PLT32`: 32-bit PLT-relative call address.
- **`X86_64CoffBinaryDesc`**: Binds the target to the Microsoft PE/COFF format. Registers relocations:
  - `IMAGE_REL_AMD64_ADDR64`: 64-bit absolute address.
  - `IMAGE_REL_AMD64_REL32`: 32-bit PC-relative call displacement.

---

### 2.7 Build-Time Plugin: `EzTargetsX86_64Dsl`
Located in `EzTargets/X86_64/Dsl/`:
- **`X86_64EncodingDialect`**: Validates the syntax of `ENCODING { form: rr; rex_w: true; ... }` blocks in `x86_64_instructions.idf`.
- **`X86_64EncodingCodegenBackend`**: Synthesizes the runtime byte-encoding tables consumed by `X86_64InstructionEncoder`.

---

### 2.8 Target Registration
Defined in `Registration/X86_64TargetRegistration.cpp`:
```cpp
void registerTarget() {
    EzCompiler::TargetResolver::registerTarget("x86_64",
        [](const EzCompiler::TargetTriple &triple,
           MirBuilderContext *mirCtx,
           bool isPositionIndependent,
           const std::vector<std::string> &features) -> EzCompiler::ResolvedTarget
        {
            auto target = std::make_unique<X86_64TargetDesc>(mirCtx);
            target->setPositionIndependent(isPositionIndependent);
            target->applyFeatures(features);
            target->initialize();

            EzCompiler::ResolvedTarget result;
            if (triple.isCoff()) {
                result.m_callingConv = target->getWin64CallingConv();
                result.m_binaryDesc = target->getCoffBinaryDesc();
            } else if (triple.isElf()) {
                result.m_callingConv = target->getSysVCallingConv();
                result.m_binaryDesc = target->getElfBinaryDesc();
            }
            result.m_targetDesc = std::move(target);
            return result;
        });
}
```
A static instance of `X86_64TargetRegistration` ensures the factory is automatically registered when `EzTargetsX86_64` is linked.

---

## 3. API Reference & Further Reading

- Generated Doxygen API documentation: [Doxygen Documentation Index](../doxygen/index.html)
- Guide on adding a new target: [How to Build a Target Architecture Guide](../how_to_build_a_target.md)
- Return to [EzPacker Landing Page](../index.md)
