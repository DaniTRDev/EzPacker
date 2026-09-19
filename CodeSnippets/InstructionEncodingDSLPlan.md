# Instruction Encoding DSL & Table-Driven Encoder Plan

## 1. Executive Summary

This document specifies how to move instruction **encoding** out of the hand-written
`EzCodeEmitter/include/X86_64/X86_64Encoding.h/.cpp` (`InstructionEncoder` and its ~60
`emit*` methods) and into declarative `EzDsl` config files, generating a **table-driven
generic encoder** under a new `EzCodeEmitter::TableGen` namespace.

Today the `.idf` file only captures the *existence* and *semantics* of a target instruction
(`MNEMONIC`, `FLAGS`, operand directions). The actual byte encodings — opcodes, REX bits,
ModR/M/SIB layout, immediate and displacement widths, mandatory prefixes — are buried in
C++ dispatch code. This makes every new target require a bespoke emitter.

The goal: extend the `.idf` dialect with an `ENCODING { ... }` block, generate a
`{Target}EncodingTable` keyed by the existing `OpCode` enum, and let a small hand-written
runtime (`EzCodeEmitter::TableGen::InstructionEncoder`) interpret that table to produce
bytes. Per the agreed direction, no per-instruction C++ is emitted.

### Scope

- In scope: `.idf` encoding grammar, encoding AST/sema, `EncodingDesc`/`EncodingOperand`
  runtime schema, table-driven `InstructionEncoder`, `CppTargetEncodingGenerator`,
  `MirTargetInstructionDesc` encoding-ID extension, worked x86-64 examples.
- Out of scope (covered by sibling plans): register definition (`RegisterBankDSLPlan.md`),
  target descriptor + emitter wiring (`TargetDescDSLPlan.md`), build/migration
  (`EncoderEmitterMigrationPlan.md`).

## 2. Current Implementation Audit

### 2.1 The Hardcoded Encoder (`InstructionEncoder`)

`EzCodeEmitter/include/X86_64/X86_64Encoding.h` declares a static-method-only class
`InstructionEncoder` (line 145) with one method per *encoding shape*, not per instruction:

- `emitMovRR/RM/MR/RI/MI`, `emitMovzxRR/RM`, `emitMovsxRR/RM`
- `emitAluRR/RM/MR/RI/MI` (shared by ADD/SUB/AND/OR/XOR/CMP via `AluOp` digit)
- `emitTestRR/RI`, `emitLea`
- `emitPushR/Imm8/Imm32/M`, `emitPopR/M`
- `emitJmpShort/Near/R`, `emitJccShort/Near`, `emitCallNear/R`, `emitRet/RetImm`, `emitNop`
- `emitShl/Shr/SarRI/RCL`, `emitNegR`, `emitNotR`
- `emitImulRR/RI`, `emitIdivR`, `emitDivR`
- `emitSetcc`
- `emitAddss/Addsd/...` (a dedicated method per SSE mnemonic)

These methods hardcode: REX prefix construction, ModR/M (`encodeModRM`), SIB
(`encodeSIB`), operand-size/opcode-byte selection, and the "MOV r64,imm → C7 /0 or B8+rd
MOVABS" special case.

### 2.2 The Name-Dispatch Emitter

`EzCodeEmitter/src/X86_64/X86_64CodeEmitter.cpp:159` (`emitInst`) is a ~850-line
`if (name == "ADD32rr") { ... }` chain that maps `MirTargetInstructionDesc::getName()` to
the appropriate `InstructionEncoder` call. It also duplicates logic: operand→register
mapping, memory mapping, relocation bookkeeping (`addReloc`, `addRelocAt`), and inline
SSE byte emission (e.g. `LOAD32` → `0xF3 0F 10 /r`).

### 2.3 What the `.idf` Currently Produces

`CppTargetInstructionGenerator` (`EzDsl/CodeGenerators/src/CodeGenerators/CppTargetInstructionGenerator.cpp`)
emits `{Target}TargetInstructionTable.h/.cpp` containing:

- `enum OpCode : size_t { ADD32rr = 1, ..., OPCODE_COUNT }`
- `static MirTargetInstructionDesc s_descs[]` (name, id, operand flags, classes, defs/uses, flags)
- `getTargetDesc(OpCode)`, `initializeTargetInstructionTable(TargetDesc*)`

`MirTargetInstructionDesc` (`EzMir/include/Instruction/MirTargetInstructionDesc.h`) stores
no encoding data whatsoever — the emitter is forced to recover it from the name string.

## 3. Design Goals & Invariants

1. **Declarative byte description.** The `.idf` file must describe *how* an instruction is
   encoded (opcode, ModR/M fields, operand slots, immediate widths) without any C++.
2. **One runtime encoder.** All targets share `EzCodeEmitter::TableGen::InstructionEncoder`;
   per-target differences are *data* (`{Target}EncodingTable`), not code.
3. **Stable public surface.** The `OpCode` enum and `MirTargetInstructionDesc` remain the
   link between instruction selection and emission; we only add an encoding ID/pointer.
4. **Relocation-agnostic encoding.** The encoder emits `Rel8/Rel32` fields as zero
   placeholders; the *emitter* layer (see `TargetDescDSLPlan.md`) is responsible for
   `addReloc` bookkeeping and final patching.
5. **Byte-identical output.** Migrating x86-64 must reproduce the existing
   `T_X86_64Encoding` / `T_X86_64CodeEmitter` golden bytes exactly (see migration plan).
6. **No EzTriple dependency in the encoder.** Encoding tables and the runtime depend only
   on `EzMir` (operand/register descriptors) so `EzCodeEmitter` stays below `EzTriple`.

## 4. `.idf` Dialect Extension — `ENCODING { ... }`

### 4.1 Grammar (lexy, extended from `Parser/TargetInstDefLang.h`)

Add a new body item `EncodingDecl` to `BodyItem` (currently `MnemonicDecl`, `FlagsDecl`,
`ImplicitDefsDecl`, `ImplicitUsesDecl`, lines 41-111). The `BodyItem::ItemVariant` gains a
`TagEncoding` alternative and `TargetInstDecl::value` copies it into a new
`TargetInstDecl::m_encoding` member.

```
target_inst ADD64ri(GPR64:dst OUT, GPR64:src1 IN, i64:imm IN) {
    MNEMONIC("addq");
    ENCODING {
        form: rr;                       // or: ri / rm / mr / mi / r / m / jcc / call / ret / ...
        opcode: [0x83];                 // one to three bytes
        opcode_digit: 0;                // ModR/M.reg extension (/0)
        rex_w: true;                    // REX.W when operand size is 64
        operands {
            dst  => reg;                // ModR/M.rm  (ADD r/m64, imm)
            imm  => imm8_signed;        // sign-extended imm8
        };
    };
};
```

Lexical/keyword additions (mirror the `Common::Keyword<"...">` pattern used throughout):

- `ENCODING`, `form`, `opcode`, `opcode_digit`, `rex_w`, `operands`
- form names: `rr`, `ri`, `rm`, `mr`, `mi`, `r`, `m`, `ri_imm`, `jcc`, `call`,
  `ret`, `movabs`, `sse`, `sse_rm`, `sse_mr`, `shift_ri`, `shift_rcl`, `unary`,
  `setcc`, `push`, `pop`, `nop`, `syscall`, `none`
- operand slot bindings: `reg`, `rm_reg`, `rm_mem`, `imm8`, `imm8_signed`, `imm16`,
  `imm32`, `imm64`, `disp8`, `disp32`, `rel8`, `rel32`, `fpref` (F2/F3/F2+F3), `cc`
- operand reference syntax: `=>` maps an operand *name* (from the `target_inst` signature)
  to a slot kind; `$`-free, since operand names are in scope.

### 4.2 AST Additions (`Ast/TargetInstDefLangAst.h`)

```cpp
namespace DSL::Ast::TargetInstDef {

enum class EncForm : uint8_t { Rr, Ri, Rm, Mr, Mi, R, M, RiImm, Jcc, Call, Ret,
                               Movabs, Sse, SseRm, SseMr, ShiftRi, ShiftRcl,
                               Unary, Setcc, Push, Pop, Nop, Syscall, None };

enum class EncSlotKind : uint8_t { Reg, RmReg, RmMem, Imm8, Imm8Signed, Imm16,
                                   Imm32, Imm64, Disp8, Disp32, Rel8, Rel32,
                                   CondCode, FpPrefF2, FpPrefF3 };

struct EncOperandBinding { Common::Identifier m_name; EncSlotKind m_slot; };

struct EncodingDecl {
    EncForm m_form;
    std::pmr::vector<uint8_t> m_opcode;      // 1..3 bytes
    std::optional<uint8_t> m_opcodeDigit;    // /0../7
    bool m_rexW;
    std::pmr::vector<EncOperandBinding> m_operands;
};

// appended to TargetInstDecl:
struct TargetInstDecl {
    // ... existing fields ...
    std::optional<EncodingDecl> m_encoding;
};
} // namespace
```

### 4.3 Sema (`SemaPasses/TargetInstPass.cpp`)

Extend `TargetInstPass` and `Symbols::TargetInstructionSymbol` with an
`std::optional<EncodingDecl> m_encoding` (or a flattened `EncodingSymbol`). Sema checks:

- `ENCODING` requires a valid `form`.
- Every bound operand name exists in the instruction signature.
- At most one `reg`, `rm_reg`, `rm_mem` binding (structural) unless the form permits more.
- `opcode_digit` is in `[0,7]`.
- `opcode` length in `[1,3]`.
- `rex_w` only meaningful for 64-bit forms.
- For `jcc`/`call` forms, a `rel8`/`rel32` (or `condcode`+`rel32`) operand is present.

Diagnostics are emitted through the existing `DiagnosticCollector`.

## 5. Runtime Schema — `EzCodeEmitter::TableGen`

New hand-written headers under `EzCodeEmitter/include/TableGen/`:

### 5.1 `EncodingDesc.h`

```cpp
namespace EzCodeEmitter::TableGen {

enum class EncSlotKind : uint8_t {
    None, Reg, RmReg, RmMem, Imm8, Imm8Signed, Imm16, Imm32, Imm64,
    Disp8, Disp32, Rel8, Rel32, CondCode, FpPrefF2, FpPrefF3
};

enum class EncPrefix : uint32_t {
    None   = 0,
    P66    = 1u << 0,
    P67    = 1u << 1,
    F2     = 1u << 2,
    F3     = 1u << 3,
    F0     = 1u << 4,
    RexW   = 1u << 5,
    RexWBySize = 1u << 6,   // W set when operand size == 8
};

struct EncodingOperand {
    EncSlotKind m_kind{ EncSlotKind::None };
    uint8_t m_operandIndex;   // index into MirTargetInstructionDesc operands
};

struct EncodingDesc {
    uint32_t m_prefixes{ 0 };     // EncPrefix bitmask
    uint8_t m_opcode[3]{ 0 };
    uint8_t m_opcodeLen{ 0 };
    bool m_hasModRM{ false };
    uint8_t m_opcodeDigit{ 0 };   // /0../7; 0xFF = use Reg operand
    // operand slots, in emission order
    std::vector<EncodingOperand> m_operands;
    // relocation hints for the emitter
    bool m_hasRelocField{ false };
    size_t m_relocOperandIndex{ 0 };
};
} // namespace
```

### 5.2 `InstructionEncoder.h` (runtime)

A single, target-agnostic class replacing the x86-specific one:

```cpp
namespace EzCodeEmitter::TableGen {

class InstructionEncoder {
  public:
    // Encodes one instruction described by desc + resolved operands.
    // `regEncoding` maps a MirRegisterDescriptor* to a hardware encoding number
    // (supplied by the generated {Target}RegisterInfo; see RegisterBankDSLPlan.md).
    // `is64BitOperand`/`operandSize` provide per-operand size hints.
    static void encode(const EncodingDesc &desc,
                       std::span<MirOperand *> operands,
                       const RegEncodingFn &regEncoding,
                       std::vector<uint8_t> &out);

    // Primitives (shared across targets, still useful for movabs/sse):
    static void emitModRM(std::vector<uint8_t> &out, uint8_t mod, uint8_t reg, uint8_t rm);
    static void emitSIB(std::vector<uint8_t> &out, uint8_t scale, uint8_t index, uint8_t base);
    static uint8_t rexByte(bool w, bool r, bool x, bool b);
};
} // namespace
```

The `encode` algorithm:

1. Emit mandatory prefixes in canonical order (`F0`, `F2`, `F3`, `66`, `67`).
2. Collect REX.W/R/X/B from `Reg`/`RmReg` operands (via `regEncoding`), set `W` from
   `RexW` or `RexWBySize` + operand size, emit REX if any bit set.
3. Emit opcode bytes.
4. If `hasModRM`, compute `mod`, `reg` (opcode digit or encoded Reg), and `rm`:
   - `RmMem` operand → resolve base/index/scale/displacement into ModR/M (+SIB when
     index/base require it), emitting displacement (`disp8`/`disp32`) after.
   - `RmReg` operand → register-direct form.
5. Emit remaining fixed/immediate/relative operands in order (`imm8/16/32/64`,
   `rel8/rel32` as zero placeholders, `condcode` folded into the second opcode byte).
6. `Reg` operands in `opcode+rd` forms (e.g. `PUSH r64` = `0x50+rd`) fold their low-3 bits
   into the final opcode byte when `m_opcodeLen` has an `opcode+rd` form.

`encode` never touches relocations; it writes `0` for `Rel8/Rel32` and sets
`desc.m_hasRelocField` so the emitter layer can register the fixup.

## 6. `CppTargetEncodingGenerator`

New generator `EzDsl/CodeGenerators/{include,src}/CodeGenerators/CppTargetEncodingGenerator.{h,cpp}`
(registered in `Cli/Driver.cpp` alongside `CppTargetInstructionGenerator`). It emits
`{Target}EncodingTable.h/.cpp` under `EzCodeEmitter::TableGen::{Target}`.

Generated header:

```cpp
namespace EzCodeEmitter::TableGen::X86_64 {
const EncodingDesc *getEncodingDesc(size_t opCode);        // indexed by OpCode enum
void initializeEncodingTable(::MirBuilderContext *ctx);    // optional one-time init
}
```

Generated source emits `static const EncodingDesc s_encodings[]` parallel to `s_descs[]`,
using the same index discipline (`OpCode` value − 1). For an instruction with no
`ENCODING` block, a `EncodingDesc{}` (empty) is emitted and the emitter falls back to
"unencodable" (diagnostic + skip), so the table can be authored incrementally.

### 6.1 Linking OpCode ↔ EncodingDesc ↔ MirTargetInstructionDesc

`MirTargetInstructionDesc` gains an optional `const void *m_encoding` (or
`uint32_t m_encodingId`). The simplest, least-invasive option is an `encodingId` slot:
`CppTargetInstructionGenerator` already iterates instructions in order, so it can stamp
each `MirTargetInstructionDesc` with the same index that `CppTargetEncodingGenerator`
uses. The emitter does `getEncodingDesc(desc->getEncodingId())`.

## 7. Worked x86-64 Examples

### 7.1 Register–register ALU (`ADD64rr` → `48 01 /r`, reg=src2, rm=dst)

```
target_inst ADD64rr(GPR64:dst OUT, GPR64:src1 IN, GPR64:src2 IN) {
    MNEMONIC("addq");
    FLAGS(IsCommutative);
    ENCODING {
        form: rr;
        opcode: [0x01];
        rex_w: true;
        operands { src2 => reg; dst => rm_reg; };
    };
};
```

### 7.2 Register–immediate ALU (`ADD64ri` → `48 83 /0 ib`)

```
    ENCODING {
        form: ri;
        opcode: [0x83];
        opcode_digit: 0;
        rex_w: true;
        operands { dst => rm_reg; imm => imm8_signed; };
    };
```

### 7.3 Register–memory (`ADD64rm` → `48 03 /r`)

```
    ENCODING {
        form: rm;
        opcode: [0x03];
        rex_w: true;
        operands { dst => reg; src2 => rm_mem; };
    };
```

### 7.4 `MOV r64, imm64` (`48 C7 /0 id` or `48 B8+rd io` MOVABS)

```
target_inst MOV64ri(GPR64:dst OUT, i64:imm IN) {
    MNEMONIC("movq");
    ENCODING {
        form: movabs;                 // encoder picks C7 /0 imm32 vs B8+rd imm64
        opcode: [0xC7];
        opcode_digit: 0;
        rex_w: true;
        operands { dst => rm_reg; imm => imm64; };
    };
};
```

### 7.5 `Jcc rel32` (`0F 8x cd`, cc folded into second opcode byte)

```
target_inst JE(i64:target IN) {
    MNEMONIC("je");
    FLAGS(IsTerminator, IsBranch, ReadsCPUFlags);
    ENCODING {
        form: jcc;
        opcode: [0x0F, 0x80];
        operands { cc => cc_EQ; target => rel32; };
    };
};
```

The `cc` binding is a *constant* `cc_EQ` (a new `CondCodeConst` slot) folded as
`opcode[1] |= cc`. The `rel32` field is emitted as `0x00000000`; the emitter registers
`BranchRel32` when the operand is a `MirReference`.

### 7.6 SSE `MOVSS [m], r` (`F3 0F 11 /r`)

```
target_inst STORE32(Mem32:addr IN, FPR32:val IN) {
    MNEMONIC("movss");
    FLAGS(WritesMemory);
    ENCODING {
        form: sse_mr;
        opcode: [0x0F, 0x11];
        operands { addr => rm_mem; val => reg; };
        prefixes: F3;
    };
};
```

These seven forms demonstrate the schema covers every shape currently hand-emitted in
`X86_64CodeEmitter.cpp` (the migration plan enumerates the full `.idf` rewrite).

## 8. Step-by-Step Implementation Roadmap

1. **Runtime schema** — add `EzCodeEmitter/include/TableGen/EncodingDesc.h` and
   `InstructionEncoder.h/.cpp` (EzMir-only deps).
2. **AST + parser** — add `EncodingDecl`, `EncForm`, `EncSlotKind`, `EncOperandBinding`
   to `TargetInstDefLangAst.h`; add `EncodingDecl` body item to `TargetInstDefLang.h`.
3. **Sema** — extend `TargetInstPass`/`TargetInstructionSymbol` with encoding validation.
4. **Desc extension** — add `encodingId` to `MirTargetInstructionDesc` and stamp it in
   `CppTargetInstructionGenerator`.
5. **Generator** — implement `CppTargetEncodingGenerator` and register it in `Driver.cpp`
   (`GeneratorKind::TargetInstructions` emits the table as a companion artifact).
6. **Emitter integration** — (deferred to `TargetDescDSLPlan.md`) replace the name-dispatch
   with `getEncodingDesc(desc->getEncodingId())` → `InstructionEncoder::encode`.
7. **Tests** — new `EzDslCodeGeneratorsTestSuite` cases asserting the emitted encoding
   table; new `EzCodeEmitterTestSuite` cases feeding `EncodingDesc` through
   `InstructionEncoder::encode` and comparing bytes.

## 9. Verification & Acceptance Criteria

- `EzDsl` parses/semas the extended `.idf` and dumps the encoding AST (`--dump-ast`).
- `CppTargetEncodingGenerator` emits `X86_64EncodingTable` with one entry per `OpCode`.
- `InstructionEncoder::encode` reproduces, byte-for-byte, the output of the existing
  `X86_64Encoding` methods for every instruction in `x86_64_instructions.idf`.
- `T_X86_64Encoding.cpp` and `T_X86_64CodeEmitter.cpp` pass unchanged after migration.
- No `EzCodeEmitter` file includes `EzTriple` headers (dependency direction preserved).

## 10. Deliverables Summary

| File | Change |
| --- | --- |
| `EzCodeEmitter/include/TableGen/EncodingDesc.h` | new runtime schema |
| `EzCodeEmitter/include/TableGen/InstructionEncoder.h/.cpp` | new runtime encoder |
| `EzMir/include/Instruction/MirTargetInstructionDesc.h` | add `encodingId` |
| `EzDsl/Lexer/include/Ast/TargetInstDefLangAst.h` | add `EncodingDecl` etc. |
| `EzDsl/Lexer/include/Parser/TargetInstDefLang.h` | add `EncodingDecl` parser |
| `EzDsl/Sema/include/Sema/Symbols/TargetSymbols.h` | carry encoding symbol |
| `EzDsl/Sema/src/SemaPasses/TargetInstPass.cpp` | validate encoding |
| `EzDsl/CodeGenerators/*/CppTargetEncodingGenerator.*` | new generator |
| `EzDsl/CodeGenerators/*/CppTargetInstructionGenerator.cpp` | stamp `encodingId` |
| `EzDsl/Cli/src/Cli/Driver.cpp` | register generator + emit flag |
