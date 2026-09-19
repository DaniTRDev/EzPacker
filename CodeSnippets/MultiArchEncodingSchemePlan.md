# Target-Neutral Encoding Scheme Plan

## 1. Executive Summary

This document specifies a new encoding scheme that lets the DSL describe the machine
encoding of **multiple architectures** without bloating the shared DSL AST with each
architecture's encoding operands.

Today the `.idf` `ENCODING { ... }` block parses into an x86-64-shaped AST
(`EncForm`, `EncSlotKind`, `EncodingDecl` with `m_rexW`, `m_prefixes`, `m_sseOpcode`,
`m_byteRex`, `m_shiftByCL`, ...). Adding an ISA such as AArch64 or RISC-V would require
unioning that ISA's fields into the shared AST, parser, sema and runtime.

The proposed scheme replaces the shared, typed encoding node with a **generic,
arch-neutral encoding IR** whose operand bindings reference **target-defined field
names** rather than a shared slot enum. All architecture-specific meaning lives in a
per-target **encoding dialect** (validation + code generation) and a per-target
**runtime encoder**. Adding an architecture therefore touches only that architecture's
dialect/backend module, never the shared AST or parser.

### Scope

- In scope: generic encoding AST/parser, per-target encoding dialect (sema + codegen),
  runtime generalization into per-target modules, emitter seam cleanup, phased
  migration of x86-64 with byte-identical output.
- Out of scope (this effort): an actual AArch64/RISC-V dialect; only the design must
  accommodate fixed-width bitfield ISAs for a later implementation.

### Settled decisions

1. **Surface syntax**: keep the current `ENCODING { ... }` syntax; the container becomes
   generic and keys/fields become target-defined identifiers. Zero `.idf` churn.
2. **Location**: encodings stay inline in `.idf`; the generic IR is designed so it can
   be extracted to separate per-target files later unchanged.
3. **Runtime**: generalize the runtime now — move x86-specific runtime out of the
   shared `TableGen` namespace and make the emitter seam target-agnostic.
4. **Fixed-width ISAs**: design for bitfield encodings now; implement only the x86-64
   dialect in this effort.

## 2. Current Implementation Audit

| Concern | Location | Problem |
| --- | --- | --- |
| Typed encoding AST | `EzDsl/Lexer/include/Ast/TargetInstDefLangAst.h:34-112` | `EncForm`, `EncSlotKind`, `EncOperandBinding` and a flat `EncodingDecl` are x86-64 specific. |
| Typed encoding parser | `EzDsl/Lexer/include/Parser/TargetInstDefLang.h:182-455` | One production and symbol table per x86 field (`FormItem`, `RexWItem`, `SseOpcodeItem`, ...). |
| Hard-coded lowering | `EzDsl/CodeGenerators/src/CodeGenerators/CppTargetEncodingGenerator.cpp` | Maps `EncForm`/`EncSlotKind` enums directly to `EncodingDesc`. |
| x86-centric runtime | `EzCodeEmitter/include/TableGen/EncodingDesc.h`, `src/TableGen/InstructionEncoder.cpp` | Assumes ModR/M + REX + SIB + prefix bytes, yet lives in a namespace called `TableGen`. |
| x86-typed generic seam | `EzTriple/include/Descriptors/TargetDesc.h` | `getEncodingDesc`/`findEncodingDesc` return `EzCodeEmitter::TableGen::EncodingDesc`. |

## 3. Design Goals & Invariants

1. **Arch-neutral shared AST.** The shared encoding node carries only generic
   directives and target-defined identifiers; it never grows when an ISA is added.
2. **Target-defined operands.** Operand bindings map `operand => field` where `field` is
   a target-owned name, not a shared enum.
3. **Isolated dialects.** Validation and lowering for an ISA live in that ISA's dialect
   and codegen backend.
4. **Typed generated tables.** Generated encoding tables remain `constexpr` and statically
   typed; no runtime string parsing of encodings.
5. **Byte-identical x86-64.** The `.idf` files are unchanged and emitted bytes match the
   current encoder exactly.
6. **Fixed-width ready.** The IR can express bitfield directives via nested blocks without
   an AST change.

## 4. Shared DSL — Generic Encoding IR

### 4.1 AST (`EzDsl/Lexer/include/Ast/EncodingDefLangAst.h`, new)

Namespace `DSL::Ast::Encoding`:

```cpp
struct OperandBinding
{
    Common::Identifier m_operand; // Declared instruction operand name.
    Common::Identifier m_field;   // Target-defined field/slot name.
};

using Value = std::variant<
    std::monostate,
    bool,
    int64_t,
    Common::Identifier,
    std::pmr::vector<uint8_t>,                 // [0x0F, 0x58]
    std::pmr::vector<Common::Identifier>,      // prefix / identifier lists
    std::pmr::vector<OperandBinding>,          // operands { op => field; ... }
    std::pmr::vector<Directive>>;              // nested block (bitfields later)

struct Directive
{
    Common::Identifier m_key;   // e.g. "form", "opcode", "rex_w", "rd", "shamt"
    Value m_value;
};

struct EncodingDecl
{
    std::optional<Common::Identifier> m_backend;      // e.g. "x86_64"
    std::pmr::vector<Directive> m_directives;
};
```

`TargetInstDecl::m_encoding` becomes this generic node. `EncForm`, `EncSlotKind` and the
typed `EncodingDecl` are removed from the shared AST.

### 4.2 Parser (`EzDsl/Lexer/include/Parser/EncodingDefLang.h`, new)

A single production parses `ENCODING [backend] { key : value ; ... }` where `value` is one
of: `bool`, integer, identifier, `[bytes]`, `operands { op => field; ... }`, or a nested
`{ ... }` directive block. The parser accepts any key/field identifier; it has no
architecture knowledge.

`TargetInstDefLang.h` reuses this production and drops `ByteLiteral`, `ByteList`,
`EncFormSymbol`, `EncSlotSymbol`, `PrefixSymbol` and every typed `Tag*Item`.

The existing `.idf` text remains valid: `form: rr;`, `opcode: [0x01];`,
`operands { src2 => reg; dst => rm_reg; };`, `prefixes: F3;`, `sse_opcode: [0x0F, 0x10];`
all parse because `rr`, `reg`, `F3` are identifiers.

### 4.3 Semantics (sema)

- **Neutral checks** in `TargetInstPass`: bound operand names exist in the signature;
  value shapes are well formed.
- **Dialect checks.** A per-target `EncodingDialect` owns the retired vocabulary and the
  current x86 `validateEncoding` rules (opcode length 1..3, `opcode_digit` in 0..7,
  Jcc/Setcc require `cond`, branch forms require a `rel32` binding).
- **Selection.** The dialect is chosen by an explicit `backend` key, falling back to the
  `.tdesc`/`--target`; an unknown backend produces a diagnostic.

## 5. Per-Target Encoding Dialect

### 5.1 Sema dialect

```cpp
namespace Sema::Encoding
{
class EncodingDialect
{
  public:
    virtual ~EncodingDialect() = default;
    virtual std::string_view name() const = 0;
    virtual bool validate(const DSL::Ast::Encoding::EncodingDecl &encoding,
                          const DSL::Ast::TargetInstDef::TargetInstDecl &inst,
                          DiagnosticCollector &diag) = 0;
};

void registerEncodingDialect(std::string_view name, EncodingDialect *dialect);
EncodingDialect *findEncodingDialect(std::string_view name);
}
```

`X86_64EncodingDialect` maps `rr/rm/mr/ri/movri/movzx/movsx/lea/unary/test/shift/imul_rr/
imul_ri/div/jcc/jmp/call/ret/push/pop/nop/syscall/setcc/sse/cvt` to the x86 form model and
validates the x86 field names.

### 5.2 Codegen backend

```cpp
namespace CodeGenerators
{
class EncodingCodegenBackend
{
  public:
    virtual ~EncodingCodegenBackend() = default;
    virtual std::string includeHeader() const = 0;
    virtual std::string namespaceName() const = 0;
    virtual std::string arrayType() const = 0;
    virtual std::string row(const Symbols::TargetInstructionSymbol &sym) const = 0;
};

void registerEncodingBackend(std::string_view name, EncodingCodegenBackend *backend);
}
```

`CppEncodingTableGenerator` (renamed and generalized from `CppTargetEncodingGenerator`)
emits the boilerplate — include guard, includes, `s_encodings[]`, `s_encodingCount`,
`s_encodingNames[]`, `getEncodingDesc`, `findEncodingDesc` — and delegates rows and the
array type to the backend.

`X86_64EncodingCodegenBackend` reproduces today's `EncodingDesc` emission, replacing the
`formToString`/`slotToString` switch statements.

## 6. Runtime Generalization

The hand-written runtime is moved out of the misleadingly generic `TableGen` namespace:

| Current | New |
| --- | --- |
| `include/TableGen/EncodingDesc.h` | `include/X86_64/Encoding/X86_64EncodingDesc.h` |
| `include/TableGen/InstructionEncoder.h` | `include/X86_64/Encoding/X86_64InstructionEncoder.h` |
| `src/TableGen/InstructionEncoder.cpp` | `src/X86_64/Encoding/X86_64InstructionEncoder.cpp` |
| `EzCodeEmitter::TableGen` (runtime types) | `EzCodeEmitter::X86_64` |

`ConditionCode`, `EncodingDesc`, `EncForm`, `EncSlotKind`, `EncOperandBinding`, `EncMemory`
and `ResolvedOperand` move into the x86 runtime. The generated encoding table is emitted
into `EzCodeEmitter::X86_64` (e.g. `EzCodeEmitter::X86_64::getEncodingDesc`).

A small shared module `include/Encoding/EncodeResult.h` keeps the arch-neutral relocation
result type (`m_hasReloc`, `m_relocOffset`, `m_relocBits`, `m_isBranch`). A documented
extension point (`BitfieldEncodingDesc` / `BitfieldWriter`) is reserved for the first
fixed-width target and is intentionally not implemented now.

### 6.1 Emitter seam

- Remove `TargetDesc::getEncodingDesc`/`findEncodingDesc` (they return an x86 type).
- `X86_64TargetDesc::createCodeEmitter()` installs the x86 resolver internally.
- `EmissionEngine` obtains its emitter via `m_ctx.getTargetDesc()->createCodeEmitter()`
  and no longer includes a concrete target emitter or registers a resolver.
- Add `beginFunction(ctx, MirFunction*)` / `endFunction(ctx, MirFunction*)` overloads to
  `GenericCodeEmitter` (default-delegating to the existing `std::string_view` overloads)
  so `EmissionEngine` can drive any target emitter.
- Move `BranchRelaxer` under the x86 module
  (`include/X86_64/BranchRelaxation/`, namespace `EzCodeEmitter::X86_64`).

## 7. Fixed-Width Readiness (design now, implement later)

- The generic IR already supports nested directive blocks, so a future dialect can express
  `bits { field: rd; range: 0..4; }` with no AST change.
- Each dialect owns operand resolution and its own descriptor/encoder; the only shared
  runtime types are `EncodeResult` and the `EncodingBackend` extension point.
- Adding AArch64/RISC-V later means adding a dialect, a codegen backend, a runtime
  descriptor/encoder and a `.idf` target — no shared-file edits.

## 8. File-Level Deliverables

| Area | Files |
| --- | --- |
| Generic AST/parser | new `Ast/EncodingDefLangAst.h`, `Parser/EncodingDefLang.h`; edits to `TargetInstDefLangAst.h`, `TargetInstDefLang.h` |
| Sema dialect | new `Sema/Encoding/EncodingDialect.h`, `X86_64EncodingDialect.{h,cpp}`; edits to `Sema/Symbols/TargetSymbols.h`, `SemaPasses/TargetInstPass.cpp` |
| Codegen | new `CodeGenerators/EncodingCodegenBackend.h`, `CppEncodingTableGenerator.{h,cpp}`, `X86_64EncodingCodegenBackend.{h,cpp}`; remove/replace `CppTargetEncodingGenerator.*` |
| Runtime | move x86 runtime to `X86_64/Encoding/*`; new `Encoding/EncodeResult.h`; table namespace `EzCodeEmitter::X86_64` |
| Emitter seam | edits to `Descriptors/TargetDesc.h`, `Targets/X86_64/X86_64TargetDesc.cpp`, `GenericCodeEmitter.h`, `EzCompiler/src/EmissionEngine.cpp` |
| Driver/dumper | edits to `Cli/InfoDumper.*` for generic encoding AST; CMake file lists |
| Tests | update `T_TargetInstDefLang`, `T_Sema_TargetInstPass`, `T_CppTargetEncodingGenerator` (→ generic), `T_TableGenEncoding` (→ x86 runtime), `T_X86_64CodeEmitter`; add generic-parser, dialect, and stub-dialect tests |

## 9. Phased Roadmap (each phase builds and all suites stay green)

1. **Generic IR & parser.** Add the generic AST/parser; switch `TargetInstDecl` to the
   generic node; keep `.idf` unchanged. Gate: lexer/parser tests.
2. **x86 dialect + backend.** Implement validation and codegen; switch `TargetInstPass`
   and the generator. Gate: `T_TableGenEncoding` exact-byte assertions and
   `T_CompilerEndToEndCompilation`.
3. **Runtime generalization.** Relocate the x86 runtime and `BranchRelaxer`, add the
   shared `EncodeResult`, generalize the `TargetDesc`/`EmissionEngine`/`GenericCodeEmitter`
   seam. Gate: full build + all suites, byte-identical output.
4. **Cleanup.** Delete the typed encoding AST/parser remnants and `TableGen` x86
   leftovers; update `InfoDumper`; refresh CMake.
5. **Proof.** Add a stub second dialect (test-only) demonstrating that a new ISA requires
   only a dialect + backend.

## 10. Verification & Acceptance Criteria

- `.idf` target instruction files are unchanged; x86-64 generation is byte-identical.
- Adding a dialect/backend requires edits only inside that dialect's module (proved by the
  stub-dialect test).
- `EmissionEngine` and `TargetDesc` contain no x86 encoding types.
- The generic AST has no architecture-specific fields; adding an ISA does not change
  `EncodingDefLangAst.h`, `EncodingDefLang.h`, `TargetInstDefLangAst.h` or
  `TargetInstDefLang.h`.
- Full build and all test suites pass after every phase.

## 11. Risks & Mitigations

| Risk | Mitigation |
| --- | --- |
| Loss of compile-time typing for encoding fields | Per-dialect `validate()` with precise diagnostics and dialect-internal typed spec structs. |
| Generic value parsing ambiguity | Ordered alternatives: `[` → byte list, `{` → block, digit → integer, bool keyword, otherwise identifier. |
| Byte drift during migration | Exact-byte emitter tests gate phases 1-3; `.idf` files are not modified. |
| Emitter seam churn | Add `MirFunction*` overloads to `GenericCodeEmitter` with default delegation; no behavior change. |
| Target naming inconsistency (`target AMD64;` vs `--target x86_64`) | Explicit `backend` key plus an alias registry mapping legacy names to dialects. |

## 12. Alternatives Considered

- **Per-arch typed AST + `std::variant`.** Type-safe, but the shared encoding node and
  parser enumerate architectures, so adding one edits shared files. Rejected.
- **Separate per-target `.enc` files.** Cleanest separation (`.idf` purely semantic) but
  adds cross-file linking; can be layered on top of the generic IR later without rework.
- **Shared bitfield mini-language.** Fully expressive for dense fixed-width ISAs, but
  trends toward a general language. Prefer a dialect-local bitfield directive.
