# Encoder/Emitter Migration & Build Integration Plan

## 1. Executive Summary

This is the execution roadmap that ties together the three design plans
(`InstructionEncodingDSLPlan.md`, `RegisterBankDSLPlan.md`, `TargetDescDSLPlan.md`). It
covers: new CMake code-generation functions, migration of the `x86_64` target to the new
config files, retirement of the hardcoded encoder/emitter/register/target-desc code,
byte-identical verification, and phased rollout with acceptance gates.

The migration is deliberately **staged** so the project always builds and all existing
tests stay green at every step.

## 2. Namespace & Layout Summary

| Concern | Namespace / location | Kind |
| --- | --- | --- |
| Generic encoder runtime | `EzCodeEmitter::TableGen` | hand-written, once |
| Per-target encoding table | `EzCodeEmitter::TableGen::X86_64` | generated |
| Per-target register info | `EzCodeEmitter::TableGen::X86_64` | generated (EzMir-only) |
| Per-target bank/class setup | `EzTriple::TableGen::X86_64` | generated |
| Per-target target descriptor | `EzTriple::TableGen::X86_64` | generated |

Generated outputs land in `build/EzTriple/generated/x86_64/` (current convention) plus a
new `build/EzCodeEmitter/generated/x86_64/` for the encoder-side tables.

## 3. Build Integration (CMake)

### 3.1 New / extended CMake functions

- `EzTriple/CMake/EzDslGenRegisters.cmake` (new): wraps `EzDslCli -i <file.reg>
  --emit-registers --target x86_64`, producing `X86_64RegisterInfo.h` into
  `EzCodeEmitter`'s generated dir (or a shared dir both libs add to their include path).
- `EzTriple/CMake/EzDslGenTargetDesc.cmake` (new): wraps `--emit-target-desc`,
  producing `X86_64TargetDesc.h/.cpp`.
- `EzTriple/CMake/EzDslGenTargetInstructions.cmake` (extend): additionally run
  `--emit-target-encodings` (or fold it into the `.idf` run) to produce
  `X86_64EncodingTable.h/.cpp`.

Each function follows the existing pattern (`EzDslGenCallingConv.cmake`):
`add_custom_command(OUTPUT ... COMMAND $<TARGET_FILE:EzDslCli> ... DEPENDS EzDslCli
<input> ...)`, then `target_sources` + `target_include_directories` for the generated
dir.

### 3.2 `EzTriple/CMakeLists.txt` additions

```cmake
include(CMake/EzDslGenRegisters.cmake)
include(CMake/EzDslGenTargetDesc.cmake)

EzDslGenRegisters(TARGET EzTriple
    INPUT "${CMAKE_CURRENT_SOURCE_DIR}/targets/x86_64/x86_64_registers.reg"
    TARGET_NAME "x86_64"
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/x86_64")

EzDslGenTargetDesc(TARGET EzTriple
    INPUT "${CMAKE_CURRENT_SOURCE_DIR}/targets/x86_64/x86_64_target.tdesc"
    TARGET_NAME "x86_64"
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/x86_64")
```

The `.tdesc` references the `.reg`, `.idf`, and `.ezcc` files; `CppTargetDescGenerator`
reads them in one `EzDslCli` invocation (or the CMake function adds them as `DEPENDS` and
passes them as `--registers/--instructions/--calling-convs` flags).

## 4. x86-64 Migration (File by File)

### 4.1 New config files

1. `EzTriple/targets/x86_64/x86_64_registers.reg` — banks/classes/registers/encodings
   (full 16 GPRs × 4 names, 16 XMMs, `special rip`).
2. `EzTriple/targets/x86_64/x86_64_instructions.idf` — add `ENCODING { ... }` to every
   `target_inst`. Coverage checklist (must reproduce every branch in
   `X86_64CodeEmitter.cpp:159`):
   - ALU rr/ri/rm (`ADD/SUB/AND/OR/XOR/CMP` × 32/64) — opcode byte tables mirroring
     `AluOp` digits.
   - `IMUL rr/ri`, `IDIV/DIV r`, `NEG/NOT r`.
   - shifts `SHL/SHR/SAR` ri + rCL.
   - `TEST rr`.
   - `SETcc` (10 condition codes).
   - `MOV rr/ri` 8/16/32/64 (incl. `movabs` special-case for `MOV64ri`).
   - `LOAD/STORE` 8/16/32/64 (incl. XMM `F3/F2 0F 10/11 /r` forms).
   - `MOVSX/MOVZX` variants, `LEA64r`.
   - `JMP` (short/near/r), `Jcc` × 10, `CALL`, `RET`, `PUSH64r`, `POP64r`.
   - SSE `ADDSS/ADDSD/SUBSS/.../CVT*`, `UCOMISS/UCOMISD`.
   - `SYSCALL`, `NOP`.
3. `EzTriple/targets/x86_64/x86_64_target.tdesc` — manifest wiring the above plus
   calling convs, object formats, libcalls, pointer/stack sizes, `rip`, components.

### 4.2 Retirement of hardcoded files

| Retire | Replaced by |
| --- | --- |
| `EzCodeEmitter/include/X86_64/X86_64Registers.h/.cpp` | `EzCodeEmitter::TableGen::X86_64::X86_64RegisterInfo` |
| `EzCodeEmitter/include/X86_64/X86_64Encoding.h/.cpp` (`InstructionEncoder`, `RexPrefix`, `AluOp`, `ConditionCode`, `MemoryOperand`) | `EzCodeEmitter::TableGen::InstructionEncoder` + `EncodingDesc` |
| `EzCodeEmitter/src/X86_64/X86_64CodeEmitter.cpp` name-dispatch | generated `{Target}CodeEmitter` + encoding table |
| `EzTriple/src/Targets/X86_64/X86_64TargetDesc.cpp:28` register/conv/wiring body | generated `EzTriple::TableGen::X86_64::X86_64TargetDesc` |
| `EzCompiler/src/TargetResolver.cpp` x86 branch | registry + generic conv/bin selection |
| `EzCompiler/src/EmissionEngine.cpp` x86 opcode patching | `TargetRelocationResolver` hook |

`X86_64CodeEmitter.h/.cpp`, `X86_64Encoding.*`, `X86_64Registers.*` are either deleted
or reduced to thin wrappers during the transition (see §7 phasing — wrappers die in the
final phase).

## 5. Verification Strategy (Byte-Identical)

The correctness bar is **bit-for-bit identical object output** before/after migration.

1. **Unit: encoder** — `tests/EzCodeEmitterTestSuite/tests/T_X86_64Encoding.cpp` is
   re-pointed from `InstructionEncoder::emitX(...)` to
   `TableGen::InstructionEncoder::encode(EncodingDesc, ...)`; the same expected byte
   vectors must pass.
2. **Unit: emitter** — `tests/EzCodeEmitterTestSuite/tests/T_X86_64CodeEmitter.cpp` and
   `T_StressAndSanitizers.cpp` continue to exercise `emitInst` and must pass unchanged.
3. **Unit: target desc** — `tests/EzTripleTestSuite` asserts the generated
   `X86_64TargetDesc` exposes identical banks/classes/registers/convs/instruction table.
4. **End-to-end** — `tests/EzCompilerTestSuite` and the example `.mir`/frontend programs
   in `examples/` produce identical ELF/COFF bytes (diff the `.o` against a pre-migration
   snapshot; the repo already keeps `scratch_multiret.o/.obj` artifacts for this style of
   check).
5. **Sanitizers** — run the existing ASan/UBSan configs (referenced by
   `ReviewPlan.md` §6) to catch lifetime issues in the new table lifetime/PMR usage.

## 6. Phased Implementation Roadmap

> Dependencies: registers (Phase 1) → encoding (Phase 2) → target desc + emitter seam
> (Phase 3) → migration/retirement (Phase 4) → cleanup + new-target proof (Phase 5).

### Phase 1 — Register data (foundation)
1. Add `m_hwEncoding` to `MirRegisterDescriptor`.
2. Implement `.reg` AST/parser/sema + `CppRegisterInfoGenerator`.
3. Generate `X86_64RegisterInfo.h`; keep `X86_64TargetDesc::initialize()` unchanged
   (parallel existence).
4. Gate: `EzDslCodeGeneratorsTestSuite` register cases pass; `EzTriple` builds.

### Phase 2 — Encoding table
1. Add `EncodingDesc`/`InstructionEncoder` runtime + `MirTargetInstructionDesc::encodingId`.
2. Implement `.idf` `ENCODING` + `CppTargetEncodingGenerator`.
3. Generate `X86_64EncodingTable`; keep `X86_64CodeEmitter` name-dispatch (parallel).
4. Gate: `T_X86_64Encoding` passes via the new encoder path (feature-toggled).

### Phase 3 — Target descriptor + emitter seam
1. Add `TargetDesc::createCodeEmitter()`/`createRegisterBank()`.
2. Implement `.tdesc` + `CppTargetDescGenerator`.
3. Refactor `EmissionEngine` to `createCodeEmitter()`; add `TargetRelocationResolver`.
4. Generate `X86_64TargetDesc`; switch `TargetResolver` to the registry.
5. Gate: `EzTripleTestSuite` + `EzCompilerTestSuite` pass with the generated descriptor.

### Phase 4 — Migration & retirement
1. Rewrite `x86_64_instructions.idf` with full `ENCODING` blocks.
2. Replace `X86_64CodeEmitter::emitInst` with table-driven `{Target}CodeEmitter`.
3. Delete `X86_64Registers.*`, `X86_64Encoding.*` emit methods, and the
   `X86_64TargetDesc::initialize()` body.
4. Gate: byte-identical ELF/COFF output; all suites green.

### Phase 5 — Cleanup & new-target proof
1. Remove any transitional wrappers/toggles.
2. Document the authoring flow (`.reg` + `.idf` + `.ezcc` + `.tdesc`).
3. *Stretch proof*: author a trivial second target (e.g. a minimal fixed-width RISC
   subset) using only config files, exercising the registry end-to-end.

## 7. Acceptance Scorecards

- **Encodability**: every instruction in `x86_64_instructions.idf` has an `ENCODING`
  block; `OPCODE_COUNT` == encoding-table size.
- **Dependency direction**: no `EzCodeEmitter` TU includes an `EzTriple` header.
- **No hardcoded dispatch**: `X86_64CodeEmitter::emitInst` has no `if (name == ...)`
  chain; `EmissionEngine` has no opcode literals.
- **No hardcoded register enum**: `X86_64Registers.h` deleted.
- **Registry**: `TargetResolver.cpp` has no target-specific include.
- **Byte-identical**: ELF + COFF outputs match pre-migration artifacts.
- **Tests**: `EzDslLexer/Sema/CodeGenerators`, `EzTriple`, `EzCodeEmitter`,
  `EzCompiler` suites all pass.

## 8. Risk Register & Mitigations

| Risk | Mitigation |
| --- | --- |
| x86 encoding irregularity (movabs, `83/81` imm-width choice, SIB `rsp` sentinel, `opcode+rd` forms) | Encode per-form rules in `InstructionEncoder`; cover with golden-byte tests before removing hand code. |
| `getRegName` print output drift | Keep per-class `asmName` in register info; assert assembler-print tests (`T_X86_64CodeEmitter` prints). |
| Relocation offsets off-by-one after table-driven emit | Reuse existing `addReloc`/`addRelocAt`; assert `BranchRel32`/`PCRel32` offsets equal pre-migration values. |
| Generated-table lifetime across PMR arenas | Store tables as static const arrays (like `s_descs[]`); avoid runtime PMR allocation for encodings. |
| `TargetResolver` binary/conv selection regressions | Keep `getName()`-based matching; add explicit `EzCompilerTestSuite` cases for win64 vs sysv triples. |
| Cross-file `.tdesc` resolution complexity | Resolve referenced files in the single `EzDslCli` invocation (same as `.lad`→`.tyf`/`.irdf` today). |

## 9. Deliverables Summary

| Artifact | Plan source |
| --- | --- |
| `x86_64_registers.reg`, extended `x86_64_instructions.idf`, `x86_64_target.tdesc` | §4.1 |
| `EzCodeEmitter::TableGen::{InstructionEncoder,EncodingDesc}` | `InstructionEncodingDSLPlan.md` |
| `{Target}RegisterInfo` + `m_hwEncoding` | `RegisterBankDSLPlan.md` |
| `{Target}TargetDesc`, `TargetRelocationResolver`, registry | `TargetDescDSLPlan.md` |
| CMake functions + `EzTriple/CMakeLists.txt` changes | §3 |
| Deleted legacy files | §4.2 |
| Updated test suites | §5 |
