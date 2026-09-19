# Register Bank & Register Class DSL Plan

## 1. Executive Summary

This document specifies a new `EzDsl` dialect — `.reg` — that declaratively describes a
target's **register banks**, **register classes**, **physical registers**, their
**hardware encodings**, and **sub-register aliasing** (e.g. `rax`/`eax`/`ax`/`al`).

Today all of this is hardcoded twice:

1. `EzCodeEmitter/include/X86_64/X86_64Registers.h` — a `Reg` enum (RAX=0 .. XMM15=31)
   plus helper functions (`getLow3Bits`, `getExtBit`, `isXmmReg`, `getRegName`) that the
   encoder uses to compute ModR/M/SIB and REX bits.
2. `EzTriple/src/Targets/X86_64/X86_64TargetDesc.cpp:28` (`initialize`) — creates
   `MirRegisterBank` (`GPR`, `FPR`), `MirRegisterClass` (`GPR8/16/32/64`, `FPR32/64`),
   and registers the 16 GPRs (4 sizes each) + 16 XMMs, with sub-register relations.

The plan replaces both with a single generated `{Target}RegisterInfo` consumed from two
places: the target descriptor (to build banks/classes) and the table-driven encoder (to
map a physical `MirRegisterDescriptor` to a hardware encoding number).

### Scope

- In scope: `.reg` grammar, AST/parser/sema, `MirRegisterDescriptor` hardware-encoding
  field, `CppRegisterInfoGenerator`, `mapRegToEncoding`, emitter integration.
- Out of scope: allocation order/preferences, callee-saved lists (already covered by
  `.ezcc`), frame-pointer/stack-pointer *behavior* (covered by `TargetDescDSLPlan.md`).

## 2. Current Implementation Audit

### 2.1 `MirRegisterBank` / `MirRegisterClass` (EzMir)

- `MirRegisterBank` (`EzMir/include/Operand/MirRegisterBank.h`): `addClass(name, class)`,
  `getClass(name)`, `getName()`.
- `MirRegisterClass` (`EzMir/include/Operand/MirRegisterClass.h`): `addRegister(name,
  bitSize, partOffsetInBits, subParts)`, `getReg(name)`, `getRegs()`.
- `MirRegisterDescriptor` (`MirRegisterDescriptor` in the same header): `m_name`,
  `m_owner`, `m_bitSize`, `m_id`, `m_partOffsetInBits`, `m_subParts`. **There is no
  hardware-encoding field** — the encoder currently recovers the encoding from `m_id`
  by assuming GPRs are identity-mapped 0..15 and XMMs are 16..31.

### 2.2 Hardcoded register topology (`X86_64TargetDesc.cpp:28`)

- Banks: `GPR`, `FPR`.
- Classes: `GPR8`, `GPR16`, `GPR32`, `GPR64` (sub-part chain `GPR8 <: GPR16 <: GPR32 <:
  GPR64`), `FPR32`, `FPR64` (`FPR32 <: FPR64`).
- GPRs registered in exact hardware-encoding order with `addRegister(name, bits, 0,
  { subReg })`, producing `al`→`ax`→`eax`→`rax` aliasing.
- XMMs registered `xmm0..xmm15` with `fpr32`→`fpr64` aliasing.

### 2.3 Encoder register mapping (`X86_64CodeEmitter.cpp:63`)

`mapRegister` maps a `MirRegister*` to `X86_64::Reg` using `reg->getRegId()` and a
string-match heuristic on the class name (`clsName.find("FPR")`) to offset XMMs by 16.
This is fragile (string sniffing) and target-specific.

## 3. Design Goals & Invariants

1. **Single source of truth.** Register names, encodings, and aliasing live in one
   `.reg` file; no parallel `Reg` enum and no string-based class sniffing.
2. **Hardware encoding is explicit.** Each physical register declares its encoder
   number (the value placed in ModR/M `reg`/`rm`, SIB, or opcode+rd fields).
3. **EzMir stays target-agnostic.** Add one generic field (`m_hwEncoding`) to
   `MirRegisterDescriptor`; do not encode x86 specifics into EzMir.
4. **Reusable by both consumers.** The generated `{Target}RegisterInfo` is header-only
   EzMir-dependent data, visible to `EzTriple` (bank/class construction) and
   `EzCodeEmitter` (encoding lookup) without a `EzCodeEmitter → EzTriple` dependency.
5. **Sub-register aliasing is data.** `al <: ax <: eax <: rax` is expressed as
   declarations, not as `addRegister` call order.

## 4. `.reg` Dialect Syntax

File: `EzTriple/targets/x86_64/x86_64_registers.reg`.

```
target AMD64;

register_bank GPR {
    classes { GPR8: 8, GPR16: 16, GPR32: 32, GPR64: 64 }
    sub_register { GPR16 <: GPR8, GPR32 <: GPR16, GPR64 <: GPR32 }
    registers {
        rax enc 0  names { rax: GPR64, eax: GPR32, ax: GPR16, al: GPR8 }
        rcx enc 1  names { rcx: GPR64, ecx: GPR32, cx: GPR16, cl: GPR8 }
        ...
        r15 enc 15 names { r15: GPR64, r15d: GPR32, r15w: GPR16, r15b: GPR8 }
    }
}

register_bank FPR {
    classes { FPR32: 32, FPR64: 64 }
    sub_register { FPR64 <: FPR32 }
    registers {
        xmm0 enc 0  names { xmm0: FPR32, xmm0: FPR64 }
        ...
        xmm15 enc 15 names { xmm15: FPR32, xmm15: FPR64 }
    }
}

special {
    rip: 16            // pseudo register, id/encoding 16, used by getInstructionPtrReg
}
```

### 4.1 Grammar Notes

- `register_bank <Name> { ... }` declares a bank and its classes.
- `classes { NAME: BITS, ... }` declares register classes with bit width.
- `sub_register { WIDE <: NARROW, ... }` declares that a register in `WIDE` has a
  same-encoding slice in `NARROW` (builds `m_subParts`).
- `registers { NAME enc N names { n: CLASS, ... } }` declares one physical register:
  - `NAME` is the canonical (widest) name.
  - `enc N` is the hardware encoding (0..31 for x86-64).
  - `names` maps each *class* to the printable assembly name for that width. The widest
    class's name equals the canonical register; narrower names become sub-registers
    (e.g. `eax` with `partOffsetInBits=0`, bit size 32, sub-parts `[ax]`).
- `special { rip: 16 }` declares non-allocatable pseudo registers referenced by the ABI
  (instruction pointer) with a fixed id; these are exposed to `TargetDesc` but not
  inserted into any allocatable class.

The `sub_register` relation and per-class `names` map are the data from which the
generator reconstructs exactly the `addRegister` call sequence currently hardcoded in
`X86_64TargetDesc.cpp`.

## 5. AST / Parser / Sema Additions

- `EzDsl/Lexer/include/Ast/RegisterDefLangAst.h` (new): `RegisterBankDecl`,
  `RegisterClassDecl`, `SubRegisterEdge`, `RegisterDecl`, `SpecialRegDecl`,
  `RegisterFile`.
- `EzDsl/Lexer/include/Parser/RegisterDefLang.h` (new): lexy rules mirroring the
  existing `CallingConvDefLang`/`TypeDefLang` patterns, with a new `LanguageDialect::RegisterDef`
  mapped from the `.reg` extension in `Cli/Driver.cpp:detectDialect`.
- `EzDsl/Sema/include/Sema/Symbols/RegisterSymbols.h` (new): `RegisterBankSymbol`,
  `RegisterClassSymbol`, `RegisterSymbol`.
- `EzDsl/Sema/include/SemaPasses/RegisterPass.h` + `src` (new): validates
  - unique bank/class/register names,
  - unique `enc` within a bank,
  - every `sub_register` edge references declared classes,
  - every `names` entry references a declared class and is width-consistent,
  - `special` ids do not collide with allocatable register encodings.

## 6. `MirRegisterDescriptor` Extension (EzMir)

Add a single field to `MirRegisterDescriptor` (`EzMir/include/Operand/MirRegisterClass.h:16`):

```cpp
struct MirRegisterDescriptor {
    // ... existing ...
    uint32_t m_hwEncoding;   // hardware encoding used by the encoder (ModR/M/SIB/opcode+rd)
};
```

The existing `addRegister(name, bitSize, partOffsetInBits, subParts)` signature is
extended with an optional `hwEncoding` (defaulting to `m_id` to preserve current behavior
for any code that constructs registers directly). The generated register-info code passes
the declared `enc`.

## 7. `CppRegisterInfoGenerator`

New generator `EzDsl/CodeGenerators/{include,src}/CodeGenerators/CppRegisterInfoGenerator.{h,cpp}`.
Emits `{Target}RegisterInfo.h` (header-only) under `EzCodeEmitter::TableGen::{Target}`:

```cpp
namespace EzCodeEmitter::TableGen::X86_64 {

struct RegisterInfoEntry {
    const char *bankName;
    const char *className;
    uint32_t bitSize;
    uint32_t hwEncoding;
    const char *asmName;          // per-class printable name
};

// Flat, dependency-free tables consumed by EzTriple and EzCodeEmitter:
const RegisterInfoEntry *getRegisterEntries();
size_t getRegisterEntryCount();

// Sub-register edges: (parentClass, parentAsm, childClass, childAsm)
struct SubRegEdge { const char *parentClass; const char *parentAsm; const char *childClass; const char *childAsm; };
const SubRegEdge *getSubRegEdges();
size_t getSubRegEdgeCount();

// Special (pseudo) registers: name -> id
uint32_t getSpecialRegId(const char *name);   // e.g. "rip" -> 16
}
```

### 7.1 Bank/Class Construction (consumed by EzTriple)

`CppTargetDescGenerator` (see `TargetDescDSLPlan.md`) emits an
`initializeRegisterBanks(TargetDesc *target)` helper that reads these tables and performs
the equivalent of the current `X86_64TargetDesc::initialize()` register setup:

```cpp
// generated: EzTriple::TableGen::X86_64::initializeRegisterBanks
for (bank in unique bankNames)            target->createBank(bankName);
for (class in unique classes)             bank.addClass(className, new MirRegisterClass(...));
for (entry in registerEntries)            class.addRegister(asmName, bitSize, 0, subParts, hwEncoding);
for (edge in subRegEdges)                 wideClass.getReg(parent)->addSubPart(child);
```

This requires a small `TargetDesc` surface addition: a way to create/register banks so the
generated code doesn't reach into `X86_64TargetDesc` internals. Minimal addition:

```cpp
// TargetDesc (EzTriple/include/Descriptors/TargetDesc.h)
virtual MirRegisterBank *createRegisterBank(const char *name) = 0;
```

The generated `{Target}TargetDesc` implements it (and its existing
`getAvailableRegisterBanks()` returns the created banks).

### 7.2 Encoding Lookup (consumed by EzCodeEmitter)

The generated `{Target}RegisterInfo` exposes the encoding map; the emitter builds a
`RegEncodingFn` (see `InstructionEncodingDSLPlan.md`) that, given a
`MirRegisterDescriptor*`, returns `m_hwEncoding` directly. This replaces both
`X86_64Registers.h`'s `Reg` enum and the `X86_64CodeEmitter::mapRegister` FPR-name
heuristic.

Because the encoding is now a field on `MirRegisterDescriptor`, the encoder can simply
use `desc->m_hwEncoding` for ModR/M/SIB/REX computation and `getRegName` for printing.
`getRegName(Reg, size)` in `X86_64Registers.cpp` is replaced by `desc->m_name` (the
per-class asm name already resolved at construction).

## 8. Emitter Integration

- `X86_64CodeEmitter::mapRegister` (`EzCodeEmitter/src/X86_64/X86_64CodeEmitter.cpp:63`)
  is replaced by: return `reg->getRegDescriptor()->m_hwEncoding` (or a
  `RegEncodingFn` supplied by the generated register info). The `Reg` enum and
  `isXmmReg`/`getXmmId`/`getLow3Bits`/`getExtBit` helpers are deleted or reimplemented
  generically in `EzCodeEmitter::TableGen::InstructionEncoder` (low-3 bits = `enc & 7`,
  ext bit = `enc >> 3`).
- `X86_64Registers.h/.cpp` are retired (see `EncoderEmitterMigrationPlan.md`).

## 9. Worked Example (x86-64 excerpt)

The `.reg` file in §4 generates the following topology, byte-identical to today:

- Bank `GPR` with classes `GPR8(8)`, `GPR16(16)`, `GPR32(32)`, `GPR64(64)`.
- Bank `FPR` with classes `FPR32(32)`, `FPR64(64)`.
- `rax` (enc 0): `al` (GPR8), `ax` (GPR16, sub `al`), `eax` (GPR32, sub `ax`),
  `rax` (GPR64, sub `eax`).
- `xmm0` (enc 0): `xmm0` in `FPR32` and `FPR64` (sub-part relation `FPR32 <: FPR64`).
- `special rip` id 16 → `X86_64TargetDesc::getInstructionPtrReg()` returns
  `MirRegisterRef(m_gpr64, 16)` unchanged.

## 10. Step-by-Step Implementation Roadmap

1. **EzMir** — add `m_hwEncoding` to `MirRegisterDescriptor`; extend `addRegister`.
2. **AST/parser** — `RegisterDefLangAst.h`, `RegisterDefLang.h`; register `.reg` in
   `detectDialect`/`resolveGeneratorKind`.
3. **Sema** — `RegisterPass`, `RegisterSymbols`.
4. **Generator** — `CppRegisterInfoGenerator`; register in `Driver.cpp`
   (`GeneratorKind::RegisterInfo`).
5. **TargetDesc surface** — add `createRegisterBank` to `TargetDesc`.
6. **Emitter** — replace `mapRegister`/`Reg` enum usage with `m_hwEncoding`.
7. **Tests** — `EzDslCodeGeneratorsTestSuite` (table shape) and
   `EzTripleTestSuite` (bank/class/alias reconstruction matches `MockTargetDesc`
   expectations).

## 11. Verification & Acceptance Criteria

- `.reg` parses/semas; `--dump-ast` shows the register file.
- `CppRegisterInfoGenerator` emits `X86_64RegisterInfo.h` with 16 GPR entries (4 names
  each) + 16 XMM entries + 2 sub-register edges per GPR + `rip` special.
- Generated `initializeRegisterBanks` produces banks/classes/aliasing identical to
  `X86_64TargetDesc::initialize()` (verified by `EzTripleTestSuite`).
- `X86_64CodeEmitter::mapRegister` no longer string-matches `"FPR"`.
- Encoder ModR/M/SIB/REX output is byte-identical (`T_X86_64Encoding`).

## 12. Deliverables Summary

| File | Change |
| --- | --- |
| `EzMir/include/Operand/MirRegisterClass.h` | add `m_hwEncoding` |
| `EzDsl/Lexer/include/Ast/RegisterDefLangAst.h` | new AST |
| `EzDsl/Lexer/include/Parser/RegisterDefLang.h` | new parser |
| `EzDsl/Sema/include/Sema/Symbols/RegisterSymbols.h` | new symbols |
| `EzDsl/Sema/include/SemaPasses/RegisterPass.h` + `src` | new sema pass |
| `EzDsl/CodeGenerators/*/CppRegisterInfoGenerator.*` | new generator |
| `EzDsl/Cli/src/Cli/Driver.cpp` | `.reg` dialect + generator |
| `EzTriple/include/Descriptors/TargetDesc.h` | add `createRegisterBank` |
| `EzCodeEmitter/include/TableGen/*` | generic reg helpers |
| `EzTriple/targets/x86_64/x86_64_registers.reg` | new config |
