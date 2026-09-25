# EzTargets Subproject Documentation

[EzPacker Documentation Index](../index.md) > [Subprojects](EzTargets.md) > **EzTargets** | [Doxygen API Reference](../doxygen/index.html)

---

## 1. Overview & Architectural Role

`EzTargets` provides concrete hardware CPU target implementations for EzPacker. It integrates target-specific code generation, machine instruction definitions, register banks, calling conventions, binary object writers, branch relaxation, and hardware instruction encoders.

The primary target architecture currently implemented in EzTargets is **x86-64 (AMD64)**, with native support for both **System V AMD64** (Linux, macOS, BSD) and **Microsoft x64** (Windows) ABIs.

```
       +-------------------------------------------------------------+
       |                          EzCompiler                         |
       +-------------------------------------------------------------+
                                       |
                                       v
                    +-------------------------------------+
                    |            TargetResolver           |
                    +-------------------------------------+
                                       |
                                       v
       +-------------------------------------------------------------+
       |                     EzTargets::X86_64                       |
       +-------------------------------------------------------------+
         |                     |                      |
         v                     v                      v
   +--------------+     +---------------+      +----------------+
   | TargetDesc   |     | BinaryDesc    |      | CallingConv    |
   | - Legalizer  |     | - ELF64       |      | - SysV AMD64   |
   | - ISel       |     | - PE-COFF     |      | - Microsoft x64|
   | - RegAlloc   |     +---------------+      +----------------+
   | - FrameLower |
   +--------------+
         |
         v
   +--------------------------------------------------+
   | Hardware Machine Code Generation                 |
   | - InstructionEncoder (Table-Driven ModR/M & SIB) |
   | - BranchRelaxer (rel8 -> rel32 expansion)        |
   | - X86_64CodeEmitter                              |
   +--------------------------------------------------+
```

---

## 2. X86-64 Target Architecture Implementation

### 2.1 `X86_64TargetDesc` (`X86_64/include/X86_64TargetDesc.h`)

`X86_64TargetDesc` inherits from `TargetDesc` (`EzTriple`) and serves as the central factory and descriptor for the AMD64 architecture:

- **Architecture Name**: `"x86_64"`.
- **Register Banks**:
  - `GPR`: 16 general-purpose 64-bit integer registers (`rax`, `rcx`, `rdx`, `rbx`, `rsp`, `rbp`, `rsi`, `rdi`, `r8`..`r15`).
  - `FPR`: 16 128-bit vector/floating-point registers (`xmm0`..`xmm15`).
- **Register Classes**:
  - `GR64` (`getGprClass()`): 64-bit integer registers.
  - `VR128` (`getVr128Class()`): 128-bit SIMD registers.
- **Stack Slot Size**: 8 bytes.
- **Binary Descriptors**:
  - `X86_64ElfBinaryDesc` for System V Linux ELF64 objects.
  - `X86_64CoffBinaryDesc` for Microsoft Windows PE-COFF objects.
- **Calling Conventions**:
  - `SysV_AMD64`: Arguments in `rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9`; returns in `rax`, `rdx`.
  - `Win64`: Arguments in `rcx`, `rdx`, `r8`, `r9`; 32-byte shadow space; returns in `rax`.

---

### 2.2 Hand-Written Lowering Shims (`X86_64/include/X86_64Lowering.h`)

The generated legalizer action table and rewrite rules (`.lrd`) reference hand-written x86-64 lowering functions and predicate/transform helpers:

```cpp
// Canonical declarations matching DSL-visible contracts:
LegalizationResult AMD64CallLowering(LegalizeCtx &ctx);
LegalizationResult AMD64ReturnLowering(LegalizeCtx &ctx);

// Predicate helpers for rewrite rules
bool isPowTwo(int64_t val);        // True when val is a positive power of two
bool isPositiveConst(int64_t val); // True when val is strictly positive

// Transform helpers for rewrite rules
int64_t log2Pow2(int64_t val); // Returns floor(log2(val)), i.e. trailing zero count
int64_t sub1(int64_t val);     // Returns val - 1 (used for power-of-two minus one masks)
```

---

### 2.3 Table-Driven Machine Instruction Encoder (`X86_64/include/Encoding/`)

Rather than maintaining a massive hard-coded switch statement for emitting machine bytes, EzTargets features a data-driven, table-interpreted instruction encoder (`InstructionEncoder`):

```cpp
namespace EzTargets::X86_64
{
class InstructionEncoder
{
public:
    static bool encode(const EncodingDesc &desc,
                       std::span<const ResolvedOperand> operands,
                       std::vector<uint8_t> &out,
                       EzCodeEmitter::EncodeResult &result);

    static constexpr uint8_t encodeModRM(uint8_t mod, uint8_t reg, uint8_t rm);
    static constexpr uint8_t encodeSIB(uint8_t scalePower, uint8_t index, uint8_t base);
    static constexpr uint8_t rexByte(bool w, bool r, bool x, bool b);

    static void encodeRegisterMove(const ResolvedOperand &dst, const ResolvedOperand &src, std::vector<uint8_t> &out);

    static void emitJmpShort(std::vector<uint8_t> &out, int8_t disp);
    static void emitJmpNear(std::vector<uint8_t> &out, int32_t disp);
    static void emitJccShort(std::vector<uint8_t> &out, ConditionCode cc, int8_t disp);
    static void emitJccNear(std::vector<uint8_t> &out, ConditionCode cc, int32_t disp);
};
}
```

#### Operand Slot Classifications (`EncSlotKind`)
`EncSlotKind` defines the structural role an operand plays within the x86-64 instruction encoding:
- `Reg`: Operand placed into the `ModR/M.reg` field.
- `RmReg`: Operand placed into the `ModR/M.rm` field (register direct mode, `mod = 11b`).
- `RmMem`: Memory operand generating `ModR/M.rm` plus optional SIB byte and displacement bytes.
- `Imm8`, `Imm16`, `Imm32`, `Imm64`: Immediate constants appended to the instruction byte stream.
- `Imm8Signed`: Sign-extended 1-byte immediate.
- `Rel8`, `Rel32`: PC-relative branch displacements.
- `CondCode`: Condition code digit folded into the opcode byte.

#### Instruction Forms (`EncForm`)
A form selects the structural encoding algorithm:
`Rr` (reg-reg), `Rm` (reg-mem load), `Mr` (mem-reg store), `Ri` (reg-imm ALU), `MovRI` (immediate move / MOVABS), `Movzx` (zero-extend), `Movsx` (sign-extend), `Lea` (load effective address), `Unary` (NOT/NEG), `Test` (TEST), `Shift` (SHL/SHR), `ImulRR`, `ImulRI`, `Div`, `Jcc`, `Jmp`, `Call`, `Ret`, `Push`, `Pop`, `Nop`, `Syscall`, `Setcc`, `Sse`, `Cvt`.

---

### 2.4 Branch Relaxation Pass (`X86_64/include/BranchRelaxation/BranchRelaxer.h`)

During early emission, conditional and unconditional jumps are optimistically emitted using compact 2-byte short encodings (`JMP rel8`, `Jcc rel8`).

However, if basic block sizes grow large (exceeding signed 8-bit displacement $[-128, +127]$), `BranchRelaxer` runs:
1. Computes exact byte addresses of every basic block label.
2. Identifies all short jumps whose branch target displacement exceeds $[-128, 127]$ bytes.
3. Rewrites them to near 32-bit displacement jumps:
   - `JMP rel8` (2 bytes) -> `JMP rel32` (`0xE9`, 5 bytes).
   - `Jcc rel8` (2 bytes) -> `Jcc rel32` (`0x0F 0x80+cc`, 6 bytes).
4. Iterates to a fixed point until all branch offsets are valid and stable.

---

### 2.5 Frame Lowering (`X86_64/include/X86_64FrameLowerer.h`)

`X86_64FrameLowerer` translates abstract stack operations into concrete x86-64 stack adjustments:

- **Prologue Generation**:
  ```asm
  push rbp
  mov  rbp, rsp
  sub  rsp, StackSize
  ```
  *(Callee-saved registers like `rbx`, `r12`-`r15` are pushed as required).*
- **Epilogue Generation**:
  ```asm
  mov  rsp, rbp
  pop  rbp
  ret
  ```
- **ABI Compliance**:
  - Enforces 16-byte stack alignment prior to executing any `CALL` instruction.
  - Windows x64 ABI: Allocates 32 bytes of shadow space (homing space) above the return address.
  - System V AMD64 ABI: Respects the 128-byte Red Zone under `%rsp` for leaf functions when optimization permits.

---

### 2.6 Target Registration (`X86_64/Registration/`)

The target registers itself into the global `TargetResolver` through `EzTargetsX86_64Registration.h`:

```cpp
#include "Registration/EzTargetsX86_64Registration.h"

// Registers the x86-64 architecture factory
EzTargets::X86_64::registerTarget();
```

---

## 3. Header & Class Index

| Component | Header Location | Key Classes / Structs |
|---|---|---|
| Target Descriptor | `EzTargets/X86_64/include/X86_64TargetDesc.h` | `X86_64TargetDesc` |
| Lowering Shims | `EzTargets/X86_64/include/X86_64Lowering.h` | `AMD64CallLowering`, `AMD64ReturnLowering`, `isPowTwo`, `log2Pow2` |
| Instruction Encoder | `EzTargets/X86_64/include/Encoding/X86_64InstructionEncoder.h` | `InstructionEncoder` |
| Encoding Descriptors | `EzTargets/X86_64/include/Encoding/X86_64EncodingDesc.h` | `EncodingDesc`, `EncSlotKind`, `EncForm`, `ConditionCode` |
| Frame Lowerer | `EzTargets/X86_64/include/X86_64FrameLowerer.h` | `X86_64FrameLowerer` |
| Branch Relaxation | `EzTargets/X86_64/include/BranchRelaxation/BranchRelaxer.h` | `BranchRelaxer` |
| Binary Descriptors | `EzTargets/X86_64/include/X86_64ElfBinaryDesc.h` | `X86_64ElfBinaryDesc` |
| Binary Descriptors | `EzTargets/X86_64/include/X86_64CoffBinaryDesc.h` | `X86_64CoffBinaryDesc` |
| Code Emitter | `EzTargets/X86_64/include/X86_64CodeEmitter.h` | `X86_64CodeEmitter` |
| Registration | `EzTargets/X86_64/Registration/include/EzTargetsX86_64Registration.h` | `registerTarget()` |
