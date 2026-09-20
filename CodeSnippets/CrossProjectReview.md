# CrossProject Review

Findings that span two or more subprojects. Per-project files link back here via `XPR-*` IDs.

All leads are unverified until checked. See `ReviewProcess.md`.

## 1. Shared helpers to centralize

- [x] **P1 · XPR-01 — One generic name registry instead of three**
  - Where: `EzDsl/Sema/src/Sema/Encoding/EncodingDialect.cpp:24-59`,
    `EzDsl/CodeGenerators/src/CodeGenerators/CppEncodingTableGenerator.cpp:31-85`,
    `EzCompiler/src/TargetResolver.cpp:30-54`
  - Why: identical "normalize + function-local static map + register/find" pattern. The x86-64
    alias list (`x86_64`, `amd64`, `x86-64`) is registered twice
    (`X86_64EncodingDialect.cpp:365-377`, `X86_64EncodingCodegenBackend.cpp:224-235`).
  - Fix: a `template <class T> NameRegistry` (case-insensitive, alias-aware) in `EzCore`;
    register aliases once. Beware static-init order (see EzDsl WEI-10).
  - Status: fixed

- [x] **P1 · XPR-02 — Safe string helpers in `EzCore/StringUtils.h`; adopt them**
  - Where: `EzCore/include/StringUtils.h:10-30` (`StrToLower`/`StrToUpper` lack an
    `unsigned char` cast — UB for non-ASCII), plus four `sanitizeIdentifier` copies in EzDsl
    (`CppTargetDescGenerator.cpp:40-65`, `CppRegisterInfoGenerator.cpp:47-72`,
    `CppEncodingTableGenerator.cpp:38-63`, `Cli/Driver.cpp:65-90`), five `ToUpper` copies, and
    inline `std::transform(tolower)` in `Cli/CommandLineOptions.cpp:221,259`,
    `Cli/Driver.cpp:392,483`, `EzCompiler/src/TargetResolver.cpp:14-27`.
  - Fix: fix the core helpers, add `SanitizeCppIdentifier(view, fallback)` and
    `NormalizeKey(view)` (optional `-`→`_`); delete the copies.
  - Status: fixed

- [x] **P1 · XPR-03 — Symbol-table collection helper**
  - Where: the `getSymbols()` → `getType()` → `getIf<T>()` idiom appears ~13× in EzDsl generators
    (see `EzDslReview.md` DUP-04).
  - Fix: `SymbolTable::collect<T>(SymbolType)` (or a templated free helper).
  - Status: fixed

- [x] **P1 · XPR-04 — PCH/common include lists overlap and drift**
  - Where: `EzDsl/Lexer/include/EzDslLexerCommon.h:4-24` vs
    `EzDsl/CodeGenerators/include/EzDslCodeGeneratorsCommon.h:4-24` (identical block);
    `EzDsl/Sema/include/EzDslSemaCommon.h:4-8` is a subset; `EzCoreCommon.h`, `EzMirCommon.h`,
    `EzTripleCommon.h`, `EzCodeEmitterCommon.h` repeat overlapping std include sets.
  - Why: Sema/CodeGenerators do not include `EzCoreCommon.h`/`StringUtils.h`, which is why helper
    duplication (XPR-02) happened.
  - Fix: shared base include header; trim the PCH (see EzDsl LEG-12).
  - Status: fixed

- [x] **P2 · XPR-05 — `escapeString` vs `InfoDumper::escapeJson`**
  - Where: `EzDsl/CodeGenerators/src/CodeGenerators/CppTargetDescGenerator.cpp:68-94` vs
    `EzDsl/Cli/src/Cli/InfoDumper.cpp:23-66`
  - Why: same structure and `reserve(size+8)` idiom, differing only in escape tables.
  - Fix: one `EscapeString(view, mode)`.
  - Status: fixed

- [x] **P2 · XPR-06 — Diagnostic `warn` convenience missing**
  - Where: `EzCore/include/Diagnostics/DiagnosticCollector.h` has `error`/`trace` but no `warn`,
    forcing the hand-rolled `CodeGenerator::warn`
    (`EzDsl/CodeGenerators/include/CodeGenerators/CodeGenerator.h:105-115`).
  - Fix: add `DiagnosticCollector::warn` and use it.
  - Status: fixed

- [x] **P2 · XPR-07 — Sema pass driver duplication**
  - Where: nine passes under `EzDsl/Sema/src/SemaPasses/*` repeat trace/null-guard/`hasErrors`
    scaffolding with inconsistent sender names and messages (`TypePass` vs `Sema::Xxx`).
  - Fix: a shared pass driver/base; standardize the diagnostic sender.
  - Status: fixed

## 2. Duplication that crosses project boundaries

- [x] **P0 · XPR-08 — x86 branch patching exists in EzCompiler and EzTriple**
  - Where: `EzCompiler/src/EmissionEngine.cpp:251-294` vs
    `EzTriple/src/Targets/X86_64/X86_64RelocationResolver.cpp:25-84`
  - Why: the compiler re-implements target-specific relocation semantics while the target
    resolver is never called in production. See `EzCompilerReview.md` DUP-01 / `EzTripleReview.md`
    WEI-10.
  - Fix: emit through `TargetDesc::getRelocationResolver()`.
  - Status: fixed

- [x] **P1 · XPR-09 — ELF/COFF section + writer logic spread across three projects**
  - Where: section creation in `EzCodeEmitter/src/Helpers.cpp:25,94,154-166`; descriptor wrappers
    in `EzTriple/src/Targets/X86_64/X86_64ElfBinaryDesc.cpp` / `X86_64CoffBinaryDesc.cpp`;
    writer feed loops in `EzCompiler/src/EmissionEngine.cpp:357-388`; duplicate `alignTo` in
    `Elf64Writer.cpp:118-123` and `CoffWriter.cpp:101-106`.
  - Why: near-identical COFF/ELF mapping and writer wiring in three places.
  - Fix: shared base descriptor + `IObjectWriter`.
  - Status: fixed

- [x] **P1 · XPR-10 — x86 encoding vocabulary duplicated across Sema and CodeGenerators**
  - Where: `EzDsl/Sema/src/Sema/Encoding/X86_64EncodingDialect.cpp:37-67` vs
    `EzDsl/CodeGenerators/src/CodeGenerators/X86_64EncodingCodegenBackend.cpp:14-95`
  - Why: forms/fields must be kept in sync; a mismatch emits `EncForm::None` silently.
  - Fix: a single shared x86 vocabulary table. See `EzDslReview.md` DUP-06.
  - Status: fixed

- [x] **P2 · XPR-11 — CLI scaffold duplicated EzDsl/Cli vs EzCompiler**
  - Where: `EzDsl/Cli/src/Cli/CommandLineOptions.cpp` vs
    `EzCompiler/src/CommandLineOptions.cpp`; entry-point error handling differs
    (`EzDsl/Cli/src/Main.cpp:9-47` has try/catch + exit 2; `EzCompiler/src/Main.cpp` has none).
  - Fix: share the argparse scaffold; align exit codes.
  - Status: fixed-progress

- [x] **P2 · XPR-12 — Frontend stub (`EzFrontend`) is outside the build but referenced**
  - Where: `EzFrontend/EzLexer/include/EzLexerCommon.h:21` includes a removed `<EzCore.h>`;
    `EzFrontend/EzLexer/src/Tokenizer/BasicTokenizer.cpp:74,344,420` calls a 4-arg
    `createReference` the current manager no longer exposes; `CMakeLists.txt:44-50` does not add
    `EzFrontend`.
  - Fix: decide to delete `EzFrontend` or bring it back in with a fixed API. See
    `EzCoreReview.md` LEG-07 / `EzCompilerReview.md` LEG-08.
  - Status: fixed

## 3. Repo-wide hygiene checks

- [x] **P1 · XPR-13 — Orphaned target source-of-truth files**
  - Where: `EzTriple/targets/x86_64/x86_64_registers.reg`, `x86_64_target.tdesc`,
    `x86_64_types.tyf`, and `EzTriple/data/{SysV_AMD64,Win64,AAPCS64}.ezcc`
  - Why: none referenced by any CMake target; some point at the removed `TableGen` namespace.
  - Fix: reconnect to the build or delete. See `EzTripleReview.md` LEG-02/03/04.
  - Status: fixed

- [x] **P2 · XPR-14 — Stale comments/docs after the emitter + encoding refactors**
  - Where: e.g. `EzDsl/CodeGenerators/CppRegisterInfoGenerator.cpp:204` and
    `CppTargetDescGenerator.cpp:295` (`EzCodeEmitter::TableGen::`), `EzDsl/README.md`,
    `EzCompiler/EmissionEngine.h:37`.
  - Fix: sweep comments/docs against the current namespaces and APIs.
  - Status: fixed-progress

- [x] **P2 · XPR-15 — Dead locals / build warnings sweep**
  - Where: `EzMir`/`EzTriple`/`EzCore` dead `bool modified`-style flags (see project files
    OPT items). Run a warning-clean build (`-Wall -Wextra`) and delete.
  - Status: fixed-progress

## 4. Marker scan (baseline)

Repo-wide, the codebase is clean of `FIXME`/`HACK`/`XXX`/`WORKAROUND` and of `#if 0` /
commented-out code. Actionable markers:

| Marker | Location |
| --- | --- |
| TODO | `EzMir/include/Function/MirFunctionBuilder.h:8` ("ugly list constructor … add a MirModule") |
| TODO | `EzTriple/src/RegisterAllocator/MirRegisterAllocatorPass.cpp:138` ("FIll with instruction selector pass") |
| "for now" | `EzCompiler/src/FrontendAdapter.cpp:40` (stub frontend) |
| "legacy target-name aliases" | `X86_64EncodingDialect.cpp:364`, `X86_64EncodingCodegenBackend.cpp:223` |

Keep this table current when new markers are added.

## 5. Verification gate

Use the standard gate in `ReviewProcess.md`. Cross-project changes (XPR-01/02/08/09) must keep the
generated x86-64 encoding table byte-identical and the end-to-end object output hash unchanged,
unless the change is explicitly intended to alter output.

## 6. Findings tracker

| ID | Severity | Category | Status | Owner | Notes |
| --- | --- | --- | --- | --- | --- |
| XPR-08 | P0 | Duplication | fixed | — | branch patching duplicated |
| XPR-01..03 | P1 | Duplication | fixed | — | `NameRegistry`, `StringUtils`, `SymbolTable::collect` |
| XPR-09..10 | P1 | Duplication | fixed | — | shared `BasicBinaryDesc`/`IObjectWriter`, x86-64 vocabulary table |
| XPR-13 | P1 | Legacy | fixed | — | orphaned target source-of-truth files deleted |
| XPR-06, XPR-12 | P2 | mixed | fixed | — | `DiagnosticCollector::warn`; out-of-build `EzFrontend` stub deleted |
| XPR-04..05, XPR-07 | P2 | mixed | fixed | — | shared `EzCommonStd.h` PCH, `EscapeString`, Sema pass driver |
| XPR-11, XPR-14..15 | P2 | mixed | fixed | — | shared CLI exit codes, doc sweep, dead-local/warning cleanup |
