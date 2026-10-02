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
   | - VEX Prefix Engine (2-byte 0xC5 & 3-byte 0xC4)  |
   | - BranchRelaxer (rel8 -> rel32 expansion)        |
   | - X86_64CodeEmitter                              |
   +--------------------------------------------------+
```

### 1.1 Modular Subsystem Organization

`EzTargets/X86_64` is partitioned into distinct architectural subsystems under `include/` and `src/`:

- **`InstructionSelector/`**: `X86_64TargetInstructionSelector.h/.cpp` — Custom target instruction selection algorithms (e.g. calls, exception handling, dynamic memory moves), operating alongside tablegen-generated ISel patterns without legacy fallback routines.
- **`FrameLowerer/`**: `X86_64FrameLowerer.h/.cpp` — Stack layout, frame pointer handling, prologue/epilogue generation, 16-byte call alignment, Windows 32-byte shadow space, and System V Red Zone.
- **`RegisterAllocator/`**: `X86_64RegisterAllocator.h/.cpp` — Physical register allocator integration, calling convention register constraints, and spill slot resolution.
- **`Lowering/`**: `X86_64Lowering.h/.cpp` — High-level ABI call/return lowering, SjLj exception unwinding shims (`AMD64ThrowLowering`, `AMD64CatchLowering`), and algebraic rewrite rule predicates/transforms (`isPowTwo`, `log2Pow2`).
- **`Descriptors/`**: `X86_64ElfBinaryDesc.h/.cpp`, `X86_64CoffBinaryDesc.h/.cpp` — Target binary format descriptors for System V ELF64 and Microsoft PE-COFF.
- **`Relocation/`**: `X86_64RelocationResolver.h/.cpp` — Relocation calculation, PC-relative offset fixups, and format-specific relocation type translation.
- **`CodeEmitter/`**: `X86_64CodeEmitter.h/.cpp` — Binary machine code emission pipeline, section layout, fixup processing, and object writer dispatch.
- **`BranchRelaxation/`**: `BranchRelaxer.h/.cpp` — Fixed-point iterative relaxation pass expanding short 8-bit jumps (`rel8`) into near 32-bit jumps (`rel32`).
- **`Encoding/`**: `X86_64InstructionEncoder.h/.cpp`, `X86_64EncodingDesc.h` — Data-driven instruction encoder interpreting ModR/M, SIB, REX, and VEX prefixes.

Forwarding headers are maintained in `EzTargets/X86_64/include/` (e.g., `X86_64TargetInstructionSelector.h`, `X86_64FrameLowerer.h`, `X86_64EncodingDesc.h`) to ensure seamless backward compatibility.

---

## 2. X86-64 Target Architecture Implementation

### 2.1 `X86_64TargetDesc` (Generated via `EzDslGenTargetDesc`)

`X86_64TargetDesc` is automatically synthesized from `targets/x86_64/x86_64.tdesc` by `EzDslGenTargetDesc` (`CppTargetDescGenerator`). It is defined in namespace `EzTargets::TableGen::X86_64` with a type alias `EzTargets::X86_64::X86_64TargetDesc` for seamless integration.

Inheriting from `TargetDesc` (`EzTriple`), it serves as the central factory and descriptor for the AMD64 architecture:

- **Architecture Name**: `"x86_64"`.
- **Register Banks**:
  - `GPR`: 16 general-purpose 64-bit integer registers (`rax`, `rcx`, `rdx`, `rbx`, `rsp`, `rbp`, `rsi`, `rdi`, `r8`..`r15`).
  - `FPR`: 16 vector / floating-point registers, supporting 32-bit (`FPR32`), 64-bit (`FPR64`), 128-bit (`VR128`, `xmm0`..`xmm15`), and 256-bit (`VR256`, `ymm0`..`ymm15`).
- **Register Classes**:
  - `GR64` (`getGprClass()`): 64-bit integer registers.
  - `VR128` (`getVr128Class()`): 128-bit SIMD registers (`xmm0`..`xmm15`).
  - `VR256` (`getVr256Class()`): 256-bit AVX/AVX2 SIMD registers (`ymm0`..`ymm15`).
  - **Register Hierarchy**: `VR256 <: VR128 <: FPR64 <: FPR32`.
- **Stack Slot Size**: 8 bytes.
- **Memory Displacement Type**: `i64` (`getMemOperandDisplacementType()`).
- **Binary Descriptors**:
  - `X86_64ElfBinaryDesc` for System V Linux ELF64 objects.
  - `X86_64CoffBinaryDesc` for Microsoft Windows PE-COFF objects.
- **Calling Conventions**:
  - `SysV_AMD64`: Scalar arguments in `rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9`; vector/float arguments in `xmm0`..`xmm7` / `ymm0`..`ymm7`; returns in `rax`, `rdx` (integers) and `xmm0`, `xmm1` / `ymm0`, `ymm1` (vectors).
  - `Win64`: Arguments in `rcx`, `rdx`, `r8`, `r9` / `xmm0`..`xmm3` / `ymm0`..`ymm3`; 32-byte shadow space; returns in `rax` / `xmm0` / `ymm0`.
- **Runtime Components**:
  - `X86_64FrameLowerer` (`getFrameLowerer()`).
  - `X86_64InstructionSelector` (`getInstructionSelector()`).
  - `X86_64RegisterAllocator` (`getRegisterAllocator()`).
  - `X86_64CodeEmitter` (`createCodeEmitter()`).
  - `X86_64RelocationResolver` (`getRelocationResolver()`).

---

### 2.2 Legalization Action Table & Libcalls (`x86_64_legalize.lad` & `x86_64_rules.lrd`)

The x86-64 target's legality rules are declaratively specified in `targets/x86_64/x86_64_legalize.lad` and synthesized into `X86_64LegalizerActionTable`:

- **Scalar Operations**: Native 8, 16, 32, and 64-bit integer arithmetic are `LEGAL`. Sub-byte conditions (`i1`) are widened to `i32`.
- **Vector Operations**:
  - 128-bit vector types (`VECTOR_128 = (v4f32, v2f64, v4i32, v2i64, v8i16, v16i8)`): `LEGAL` for basic movement (`MOV`), loads, stores, and arithmetic (`ADD`, `SUB`, `MUL`, `DIV`, `AND`, `OR`, `XOR`).
  - 256-bit vector types (`VECTOR_256 = (v8f32, v4f64, v8i32, v4i64)`): `LEGAL` for AVX/AVX2 vector operations.
- **Compiler-RT Libcalls**:
  Operations on 128-bit integers (`i128`) that lack single-instruction hardware support are declared as `LIBCALL` in `x86_64_legalize.lad`, mapping directly to standard compiler runtime routines:
  - `MUL i128` -> `__multi3`
  - `SDIV i128` -> `__divti3`
  - `UDIV i128` -> `__udivti3`
  - `SMOD i128` -> `__modti3`
  - `UMOD i128` -> `__umodti3`
  These symbols are registered in `X86_64TargetDesc` via `libcall_mapping` and resolved during link time by `EzLinker`.
- **Algebraic Rewrite Rules (`.lrd`)**:
  Declarative optimization rules lower power-of-two divisions to arithmetic/logical shifts (`SAR`, `SHR`), power-of-two modulo to bitwise AND masks, and eliminate identity operations (`x * 1 -> x`, `x * 0 -> 0`).

#### Hand-Written Lowering Shims (`X86_64/include/Lowering/X86_64Lowering.h`)

The generated legalizer action table references hand-written x86-64 lowering functions and predicate/transform helpers:

```cpp
// Canonical declarations matching DSL-visible contracts:
LegalizationResult AMD64CallLowering(LegalizeCtx &ctx);
LegalizationResult AMD64ReturnLowering(LegalizeCtx &ctx);
LegalizationResult AMD64ThrowLowering(LegalizeCtx &ctx);
LegalizationResult AMD64CatchLowering(LegalizeCtx &ctx);

// Predicate helpers for rewrite rules
bool isPowTwo(int64_t val);        // True when val is a positive power of two
bool isPositiveConst(int64_t val); // True when val is strictly positive

// Transform helpers for rewrite rules
int64_t log2Pow2(int64_t val); // Returns floor(log2(val)), i.e. trailing zero count
int64_t sub1(int64_t val);     // Returns val - 1 (used for power-of-two minus one masks)
```

#### Exception Lowering Details
- `AMD64ThrowLowering`: Lowers high-level `THROW` instructions into a runtime libcall to `__ez_throw`:
  - 1 operand (`THROW %payload`): Automatically appends `@__ez_default_rtti`, lowering to `CALL @__ez_throw, %payload, @__ez_default_rtti` so that the second argument register (`rsi` in SysV, `rdx` in Win64) is always initialized with valid canonical RTTI.
  - 2 operands (`THROW %payload, @CustomRtti`): Preserves custom RTTI and lowers to `CALL @__ez_throw, %payload, @CustomRtti`.
  - 0 operands (`THROW`): Synthesizes default arguments, lowering to `CALL @__ez_throw, @__ez_default_payload, @__ez_default_rtti`.
- `AMD64CatchLowering`: Lowers high-level `CATCH(dst)` instructions into a runtime libcall to `__ez_get_current_exception()`, binding the caught payload to the destination register.
- `X86_64TargetInstructionSelector::selectTRY`: Emits an unconditional jump to the try body basic block, integrating with the SjLj exception landing pad structure.
- `X86_64TargetInstructionSelector::selectCALL`: Automatically resolves `MirRuntimeSymbol` operands into `MirReference` objects and external declarations, generating standard branch relocations (`IMAGE_REL_AMD64_REL32` / `R_X86_64_PLT32`) for all runtime symbols.
- `X86_64TargetInstructionSelector::selectMOV`: Supports loading addresses of functions, global variables, and `MirRuntimeSymbol` instances into registers via `LEA64r %dst, @ref`, generating PC-relative data relocations (`PCRel32`).

---

### 2.3 Table-Driven Machine Instruction Encoder & VEX Engine (`X86_64/include/Encoding/`)

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
- `VexReg`: Operand placed into the inverted 4-bit `vvvv` field of the VEX prefix (`~reg_idx`).
- `Imm8`, `Imm16`, `Imm32`, `Imm64`: Immediate constants appended to the instruction byte stream.
- `Imm8Signed`: Sign-extended 1-byte immediate.
- `Rel8`, `Rel32`: PC-relative branch displacements.
- `CondCode`: Condition code digit folded into the opcode byte.

#### Instruction Forms (`EncForm`)
A form selects the structural encoding algorithm:
`Rr` (reg-reg), `Rm` (reg-mem load), `Mr` (mem-reg store), `Ri` (reg-imm ALU), `MovRI` (immediate move / MOVABS), `Movzx` (zero-extend), `Movsx` (sign-extend), `Lea` (load effective address), `Unary` (NOT/NEG), `Test` (TEST), `Shift` (SHL/SHR), `ImulRR`, `ImulRI`, `Div`, `Jcc`, `Jmp`, `Call`, `Ret`, `Push`, `Pop`, `Nop`, `Syscall`, `Setcc`, `Sse`, `Cvt`, `Vex`.

#### VEX Prefix Encoding Engine (`EncForm::Vex`)
The encoder supports AVX and AVX2 vector instructions using 2-byte (`0xC5`) and 3-byte (`0xC4`) VEX prefixes:
- **Automatic Prefix Compression**: Emits the compact 2-byte VEX prefix (`0xC5`) whenever the opcode map is `0F` (`map == 1`), `X == 0`, `B == 0`, and `W == 0`. Emits the 3-byte VEX prefix (`0xC4`) when extended registers (`r8`..`r15`, `ymm8`..`ymm15`) are used in `ModR/M.rm` or SIB index positions (`B=1` or `X=1`), when 64-bit operand sizing is required (`W=1`), or for non-`0F` opcode maps (`0F38`, `0F3A`).
- **Non-Destructive 3-Operand Syntax**: 3-register AVX operations (e.g. `VADDPS dst, src1, src2`) encode `src1` in the inverted `vvvv` field:
  $$\text{VEX.vvvv} = (\sim \text{reg\_idx}) \ \& \ 0\text{xF}$$
- **Vector Length (`L`)**: Encodes `L = 0` for 128-bit vector operations (`vex_l: false`) and `L = 1` for 256-bit operations (`vex_l: true`).
- **Opcode Map & Prefix Bits (`m-mmmm` & `pp`)**: Maps legacy prefixes (`0x66`, `0xF3`, `0xF2`) into 2-bit `pp` fields and maps escape byte sequences (`0x0F`, `0x0F 0x38`, `0x0F 0x3A`) into `m-mmmm` fields without emitting extra prefix bytes.
- **256-bit Register Copies**: `encodeRegisterMove` natively emits `VMOVAPS` for 256-bit register-to-register transfers (`ymm1` -> `ymm2`).

---

### 2.4 AVX & AVX2 Vector Extensions

EzTargets provides full AVX and AVX2 instruction selection and register management:

- **Register Bank Hierarchy**:
  - `VR256` represents 256-bit YMM registers (`ymm0`..`ymm15`).
  - Sub-register containment is fully modeled: each `ymmN` register contains `xmmN` (`VR128`), which in turn contains the 64-bit scalar float `xmmN` (`FPR64`) and 32-bit scalar float `xmmN` (`FPR32`).
- **Pattern Matching Priority**:
  - Bottom-Up Maximal Munch in EzDSL evaluates patterns grouped by opcode in descending order of cost (`m_cost`).
  - To prefer AVX instructions over legacy SSE when AVX is enabled, AVX patterns specify `[cost = 2]` guarded by `when { hasExtension("avx"); }`, while fallback SSE patterns specify `[cost = 1]`.
- **Supported Operations**:
  - 128-bit and 256-bit vector moves: `VMOVAPS`, `VMOVUPS`.
  - 128-bit and 256-bit vector floating-point arithmetic: `VADDPS`, `VSUBPS`, `VMULPS`, `VDIVPS`, `VADDPD`, `VSUBPD`, `VMULPD`, `VDIVPD`.
  - 128-bit and 256-bit vector integer arithmetic: `VPADDD`, `VPSUBD`, `VPMULLD`, `VPADDQ`, `VPSUBQ`.
  - 128-bit and 256-bit bitwise logic: `VANDPS`, `VORPS`, `VXORPS`.

---

### 2.5 Branch Relaxation Pass (`X86_64/include/BranchRelaxation/BranchRelaxer.h`)

During early emission, conditional and unconditional jumps are optimistically emitted using compact 2-byte short encodings (`JMP rel8`, `Jcc rel8`).

However, if basic block sizes grow large (exceeding signed 8-bit displacement $[-128, +127]$), `BranchRelaxer` runs:
1. Computes exact byte addresses of every basic block label.
2. Identifies all short jumps whose branch target displacement exceeds $[-128, 127]$ bytes.
3. Rewrites them to near 32-bit displacement jumps:
   - `JMP rel8` (2 bytes) -> `JMP rel32` (`0xE9`, 5 bytes).
   - `Jcc rel8` (2 bytes) -> `Jcc rel32` (`0x0F 0x80+cc`, 6 bytes).
4. Iterates to a fixed point until all branch offsets are valid and stable.

---

### 2.6 Frame Lowering (`X86_64/include/FrameLowerer/X86_64FrameLowerer.h`)

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

### 2.7 Target Registration (`X86_64/Registration/`)

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
| Target Descriptor | Generated (`generated/x86_64/X86_64TargetDesc.h`) | `X86_64TargetDesc` |
| Instruction Selector | `EzTargets/X86_64/include/InstructionSelector/X86_64TargetInstructionSelector.h` | `X86_64TargetInstructionSelector` |
| Lowering Shims | `EzTargets/X86_64/include/Lowering/X86_64Lowering.h` | `AMD64CallLowering`, `AMD64ReturnLowering`, `isPowTwo`, `log2Pow2` |
| Instruction Encoder | `EzTargets/X86_64/include/Encoding/X86_64InstructionEncoder.h` | `InstructionEncoder` |
| Encoding Descriptors | `EzTargets/X86_64/include/Encoding/X86_64EncodingDesc.h` | `EncodingDesc`, `EncSlotKind`, `EncForm`, `ConditionCode` |
| Frame Lowerer | `EzTargets/X86_64/include/FrameLowerer/X86_64FrameLowerer.h` | `X86_64FrameLowerer` |
| Register Allocator | `EzTargets/X86_64/include/RegisterAllocator/X86_64RegisterAllocator.h` | `X86_64RegisterAllocator` |
| Branch Relaxation | `EzTargets/X86_64/include/BranchRelaxation/BranchRelaxer.h` | `BranchRelaxer` |
| Relocation Resolver | `EzTargets/X86_64/include/Relocation/X86_64RelocationResolver.h` | `X86_64RelocationResolver` |
| Binary Descriptors | `EzTargets/X86_64/include/Descriptors/X86_64ElfBinaryDesc.h` | `X86_64ElfBinaryDesc` |
| Binary Descriptors | `EzTargets/X86_64/include/Descriptors/X86_64CoffBinaryDesc.h` | `X86_64CoffBinaryDesc` |
| Code Emitter | `EzTargets/X86_64/include/CodeEmitter/X86_64CodeEmitter.h` | `X86_64CodeEmitter` |
| Registration | `EzTargets/X86_64/Registration/include/EzTargetsX86_64Registration.h` | `registerTarget()` |
