# Target Descriptor DSL & Target-Agnostic Emitter Plan

## 1. Executive Summary

This document specifies a new `EzDsl` dialect — `.tdesc` — that declaratively describes a
**target** (its register file, instruction file, calling conventions, object formats,
stack slot size, instruction-pointer register, libcall table) and generates the
`{Target}TargetDesc` that currently exists only as hand-written C++
(`EzTriple/src/Targets/X86_64/X86_64TargetDesc.cpp`).

In the same breath, it generalizes the **emitter boundary** so that `EmissionEngine`
(`EzCompiler/src/EmissionEngine.cpp`) stops hardcoding `X86_64CodeEmitter` and x86 branch
patching, and instead drives a target-supplied `GenericCodeEmitter` + relocation resolver.

### Scope

- In scope: `.tdesc` grammar/AST/sema, `CppTargetDescGenerator`, generated
  `{Target}TargetDesc`, target-agnostic `GenericCodeEmitter`/`EmissionEngine` refactor,
  `TargetRelocationResolver` hook, `TargetResolver` registry.
- Out of scope (core-scope boundary): making register allocation strategy, frame
  lowering, the instruction-selection *engine*, and branch *relaxation* fully
  data-driven. These remain hand-written per target and are wired by the generated
  `TargetDesc`, not eliminated.

## 2. Current Implementation Audit

### 2.1 Hand-written target descriptor (`X86_64TargetDesc.cpp`)

`EzTriple/src/Targets/X86_64/X86_64TargetDesc.cpp:28` (`initialize`) performs, in order:

1. create `GPR`/`FPR` banks and `GPR8/16/32/64`, `FPR32/64` classes (→ replaced by
   `RegisterBankDSLPlan.md`);
2. register 16 GPRs + 16 XMMs (→ replaced by register info);
3. instantiate calling conventions `SysV_AMD64CallingConvDesc` / `Win64CallingConvDesc`
   (already generated from `.ezcc`);
4. `initializeTargetInstructionTable(this)` (generated from `.idf`);
5. create legalizer info + `MirLegalizer`;
6. create `X86_64TargetInstructionSelector` + `X86AddressingModeMatcher`;
7. create `X86_64RegisterAllocator` + `X86_64FrameLowerer`;
8. create `X86_64ElfBinaryDesc` / `X86_64CoffBinaryDesc`.

Steps 3–8 reference concrete C++ classes, some generated (convs, instruction table) and
some hand-written (isel, regalloc, frame lowerer, binary descs).

### 2.2 Target selection (`TargetResolver.cpp`)

`EzCompiler/src/TargetResolver.cpp:20` hardcodes `if (triple.isX86_64()) { make
X86_64TargetDesc; pick win/elf conv+bin }`. There is no registry: adding a target means
touching this function.

### 2.3 Emission is x86-specific (`EmissionEngine.cpp`)

`EzCompiler/src/EmissionEngine.cpp:46` constructs `EzCodeEmitter::X86_64::X86_64CodeEmitter`
directly. The relocation/branch-patching pass (lines 232-343) hardcodes x86 opcodes
(`0xE9` JMP, `0x0F 0x8x` Jcc, `0xE8` CALL) and `disp = target - (instOffset + 5/6)`.
Making emission configurable requires (a) obtaining the emitter from the target and
(b) moving these opcode/patching rules behind a target hook.

### 2.4 `GenericCodeEmitter` interface

`EzCodeEmitter/include/GenericCodeEmitter.h` is already target-agnostic
(`beginFunction`, `bindLabel`, `endFunction`, `emitInst`) and is the correct seam. The
problem is purely that nothing *provides* an emitter to `EmissionEngine` — it builds the
x86 one itself.

## 3. Design Goals & Invariants

1. **One `.tdesc` per target.** Registers, instructions, convs, formats, and target
   constants are named in a single top-level file.
2. **Generated `{Target}TargetDesc`.** The `initialize()` wiring is generated; the
   hand-written per-target strategy objects (isel/regalloc/frame lowerer) stay, but are
   *referenced* by the generated descriptor rather than being the descriptor.
3. **Target-agnostic emission.** `EmissionEngine` depends only on
   `TargetDesc`/`TargetBinaryDesc`/`GenericCodeEmitter`, never on a concrete target.
4. **Relocation patching is a target hook.** `TargetRelocationResolver` owns the
   "given a relocation + section bytes, patch the right field" logic; x86 is the first
   implementation.
5. **Registry, not `if`-chain.** `TargetResolver` looks up a registered target factory by
   `TargetTriple`.

## 4. `.tdesc` Dialect Syntax

File: `EzTriple/targets/x86_64/x86_64_target.tdesc`.

```
target x86_64 {
    registers:     "x86_64_registers.reg";
    instructions:  "x86_64_instructions.idf";
    calling_convs: ["x86_64_calling_conv.ezcc"];

    pointer_size: 8;                 // stack slot size / pointer width
    stack_slot:   8;

    instruction_pointer: rip;        // name from registers 'special'
    mem_disp_type: i64;

    object_formats: [ELF, COFF];     // binary descs to instantiate
    default_calling_conv: SysV_AMD64;

    libcalls {
        __returnNothing: "__returnNothing";
        __divdi3:        "__divdi3";
        __udivdi3:       "__udivdi3";
        ...
    }

    components {                    // hand-written strategy classes to wire
        frame_lowerer:        X86_64FrameLowerer;
        instruction_selector: X86_64TargetInstructionSelector;
        addressing_mode:      X86AddressingModeMatcher;
        register_allocator:   X86_64RegisterAllocator;
        legalizer_info:       x86_64LegalizerInfo;     // generated from .lad
    }
}
```

The `.tdesc` does not itself *encode* anything; it is a manifest that names other config
files and target constants. Cross-file references (`registers`, `instructions`,
`calling_convs`) are resolved by `CppTargetDescGenerator` by reading those files in the
same `Driver` invocation (mirroring how `LegalizeAction` already ingests `.tyf`/`.irdf`
in `Cli/Driver.cpp:650`).

## 5. AST / Parser / Sema

- `EzDsl/Lexer/include/Ast/TargetDescDefLangAst.h` (new): `TargetDescDecl` with
  string/path fields, `ObjectFormat` list, `LibcallEntry`, `ComponentBinding`.
- `EzDsl/Lexer/include/Parser/TargetDescDefLang.h` (new): lexy rules; new
  `LanguageDialect::TargetDesc` mapped from `.tdesc` in `Cli/Driver.cpp:detectDialect`.
- `EzDsl/Sema/include/Sema/Symbols/TargetDescSymbols.h` (new) +
  `EzDsl/Sema/include/SemaPasses/TargetDescPass.h` (new): validates referenced files
  exist and, if loaded, that `instruction_pointer` names a declared `special` register,
  that `pointer_size`/`stack_slot` are consistent, and that `libcalls` ids are unique.

## 6. `CppTargetDescGenerator`

New generator emitting `{Target}TargetDesc.h/.cpp` under `EzTriple::TableGen::{Target}`.
It produces a concrete `TargetDesc` subclass whose `initialize()`:

1. calls the generated `initializeRegisterBanks` (see `RegisterBankDSLPlan.md`);
2. instantiates the `.ezcc`-generated `CallingConvDesc` classes (names derived from the
   `.ezcc` `calling_convention` blocks — the same convention already used by
   `X86_64TargetDesc.cpp:115`);
3. calls `initializeTargetInstructionTable(this)` (`.idf`);
4. constructs the legalizer info / legalizer;
5. constructs the `components.*` strategy objects by type name;
6. constructs `{Target}BinaryDesc` objects per `object_formats`.

The generated class keeps the same accessors used by `TargetResolver`
(`getSysVCallingConv()`, `getWin64CallingConv()`, `getElfBinaryDesc()`,
`getCoffBinaryDesc()`) or, better, replaces those ad-hoc accessors with a generic
`getAvailableCallingConventions()`/`getAvailableBinaryDescriptors()` pairing plus a
`getDefaultCallingConv()` selector — `TargetResolver` then stops depending on
x86-specific method names.

### 6.1 Generated name lookup

The `.tdesc` references strategy classes (`components.*`) and conv/bin classes by name.
The generator emits `#include` of the corresponding headers and constructs them by
symbol. The `EzTriple/CMakeLists.txt` must therefore add the referenced headers to the
target's include path (already the case: `include/` + generated dirs).

## 7. Target-Agnostic Emitter Refactor

### 7.1 `GenericCodeEmitter` remains the seam

No interface change required beyond what exists; the target supplies a concrete
`GenericCodeEmitter`. Add to `TargetDesc`:

```cpp
// EzTriple/include/Descriptors/TargetDesc.h
virtual class GenericCodeEmitter *createCodeEmitter() = 0;
```

`EmissionEngine::emitModule` replaces:

```cpp
EzCodeEmitter::X86_64::X86_64CodeEmitter emitter;   // hardcoded
```

with:

```cpp
std::unique_ptr<GenericCodeEmitter> emitter(
    m_ctx.getTargetDesc()->createCodeEmitter());
```

`createCodeEmitter()` on the generated `{Target}TargetDesc` returns the generated
table-driven emitter (`{Target}CodeEmitter` from `InstructionEncodingDSLPlan.md` +
`TargetDescDSLPlan.md`), passing it the register-encoding function from
`{Target}RegisterInfo`.

### 7.2 `TargetRelocationResolver` hook

The x86 branch/reloc patching in `EmissionEngine.cpp:232-343` moves behind a hook owned
by the target binary desc or target desc:

```cpp
// EzTriple (or EzCodeEmitter) new interface
class TargetRelocationResolver {
  public:
    virtual ~TargetRelocationResolver() = default;
    // Given the text section bytes, a relocation, and resolved target symbol
    // offsets, patch the encoded field in place. Returns false if unhandled.
    virtual bool patch(std::span<uint8_t> text,
                       const CodeRelocation &reloc,
                       uint64_t targetOffset,
                       TargetCodeRelocationType type) = 0;
};
```

`X86_64TargetDesc`/`X86_64BinaryDesc` supplies the first implementation (the `0xE9`/
`0x0F 0x8x`/`0xE8` logic, unchanged). `EmissionEngine` iterates relocations and calls
`resolver->patch(...)` instead of matching opcodes itself. This is the minimal
target-agnosticization; full relaxation remains x86-hardcoded inside the resolver.

### 7.3 Relocation bookkeeping at emit time

With table-driven encoding, `Rel8/Rel32` fields are emitted as zero placeholders (see
`InstructionEncodingDSLPlan.md` §5.2). The generated `{Target}CodeEmitter`'s `emitInst`
must, after encoding, inspect the `EncodingDesc` and register the appropriate relocation
for `MirReference`/global/function operands (the logic currently inline in
`X86_64CodeEmitter.cpp` `emitJccHelper`, `JMP`, `CALL`, `LOAD/STORE`, `LEA`). This stays
in the (generated or hand-written per-target) emitter, keyed off
`EncodingDesc::m_hasRelocField`.

## 8. `TargetResolver` Registry

Replace the hardcoded `if (triple.isX86_64())` with a registry:

```cpp
using TargetFactory = std::function<std::unique_ptr<TargetDesc>(MirBuilderContext *)>;
void registerTarget(std::string_view arch, TargetFactory factory);
ResolvedTarget resolve(const TargetTriple &triple, MirBuilderContext *ctx);
```

Registration happens in a small per-target TU (generated alongside `{Target}TargetDesc`),
so `TargetResolver` has no `#include "Targets/X86_64/..."`. Binary/conv selection inside
`resolve` becomes generic: iterate `getAvailableBinaryDescriptors()` /
`getAvailableCallingConventions()` and match on the triple's OS via each descriptor's
`getName()` + the `.tdesc` `default_calling_conv`, rather than hardcoded getters.

## 9. Worked Example (x86-64)

`x86_64_target.tdesc` (above) generates `EzTriple::TableGen::X86_64::X86_64TargetDesc`
that:

- builds `GPR`/`FPR` banks + classes from `x86_64_registers.reg`;
- instantiates `SysV_AMD64CallingConvDesc` and `Win64CallingConvDesc`;
- initializes the `x86_64TargetInst` table and the `x86_64LegalizerInfo`;
- wires `X86_64TargetInstructionSelector`, `X86AddressingModeMatcher`,
  `X86_64RegisterAllocator`, `X86_64FrameLowerer`, `X86_64ElfBinaryDesc`,
  `X86_64CoffBinaryDesc`;
- exposes `getInstructionPtrReg() = MirRegisterRef(gpr64, ripId=16)`,
  `getStackSlotSize() = 8`, and the libcall strings.

`EmissionEngine` obtains the table-driven `X86_64CodeEmitter` via
`createCodeEmitter()` and patches relocations via the x86 `TargetRelocationResolver`.

## 10. Step-by-Step Implementation Roadmap

1. **Emitter seam** — add `TargetDesc::createCodeEmitter()`; refactor `EmissionEngine`
   to use it (temporarily returning the existing `X86_64CodeEmitter`).
2. **Relocation hook** — introduce `TargetRelocationResolver`; move x86 patching into it;
   make `EmissionEngine` delegate.
3. **`.tdesc` dialect** — AST/parser/sema + `detectDialect`.
4. **`CppTargetDescGenerator`** — generate `{Target}TargetDesc` (bank setup from register
   info, conv/inst/legalizer wiring, component construction, binary descs).
5. **Registry** — `TargetResolver` registry + per-target factory registration.
6. **Emitter integration** — generated table-driven `{Target}CodeEmitter` (finalizes
   `InstructionEncodingDSLPlan.md`); retire `X86_64CodeEmitter` name-dispatch.
7. **Tests** — `EzTripleTestSuite` (generated descriptor parity), `EzCodeEmitterTestSuite`
   (byte-identical), `EzCompilerTestSuite` (end-to-end emission via registry).

## 11. Verification & Acceptance Criteria

- `.tdesc` parses/semas; `--dump-ast` shows the manifest.
- Generated `X86_64TargetDesc` produces the same banks/classes/convs/tables as the
  hand-written one (asserted in `EzTripleTestSuite`).
- `EmissionEngine` contains no `X86_64` include or opcode literal.
- `TargetResolver` contains no target-specific include; adding a target = new
  `.tdesc` + `.reg` + `.idf` + factory registration.
- Full-pipeline object output is byte-identical to the pre-refactor build.

## 12. Deliverables Summary

| File | Change |
| --- | --- |
| `EzDsl/Lexer/include/Ast/TargetDescDefLangAst.h` | new AST |
| `EzDsl/Lexer/include/Parser/TargetDescDefLang.h` | new parser |
| `EzDsl/Sema/include/Sema/Symbols/TargetDescSymbols.h` | new symbols |
| `EzDsl/Sema/include/SemaPasses/TargetDescPass.h` + `src` | new sema pass |
| `EzDsl/CodeGenerators/*/CppTargetDescGenerator.*` | new generator |
| `EzDsl/Cli/src/Cli/Driver.cpp` | `.tdesc` dialect + generator |
| `EzTriple/include/Descriptors/TargetDesc.h` | `createCodeEmitter`, `createRegisterBank` |
| `EzTriple/include/Descriptors/TargetRelocationResolver.h` | new hook |
| `EzCodeEmitter/include/GenericCodeEmitter.h` | (no change; used as seam) |
| `EzCompiler/src/EmissionEngine.cpp` | target-agnostic emit + reloc delegate |
| `EzCompiler/src/TargetResolver.cpp` | registry |
| `EzTriple/targets/x86_64/x86_64_target.tdesc` | new config |
