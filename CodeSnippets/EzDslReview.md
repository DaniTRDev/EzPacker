# EzDsl Review

## 1. Scope & entry points

- `EzDsl/Lexer/` — AST (`include/Ast/*`) and lexy parsers (`include/Parser/*DefLang.h`).
- `EzDsl/Sema/` — symbol table/scope, `SemaPasses/*`, and the `Sema/Encoding` dialect registry.
- `EzDsl/CodeGenerators/` — ten `Cpp*Generator` classes + `CodeGenerator`/`CppSourceEmitter`.
- `EzDsl/Cli/` — `Driver`, `CommandLineOptions`, `InfoDumper`.
- Build: `EzDsl/{Lexer,Sema,CodeGenerators,Cli}/CMakeLists.txt`.

All leads are unverified until checked. See `ReviewProcess.md`.

## 2. Duplicated code

- [x] **P1 · DUP-01 — `sanitizeIdentifier` copied four times**
  - Where: `CppTargetDescGenerator.cpp:40-65`, `CppRegisterInfoGenerator.cpp:47-72`,
    `CppEncodingTableGenerator.cpp:38-63`, `Cli/Driver.cpp:65-90` (`sanitizeTargetIdentifier`)
  - Why: byte-for-byte identical except the fallback constant.
  - Fix: one shared helper (see `CrossProjectReview.md` XPR-02).
  - Status: fixed

- [x] **P1 · DUP-02 — `ToUpper` copied five times**
  - Where: `CppTargetInstructionGenerator.cpp:16-24`, `CppInstructionSelectorGenerator.cpp:18-26`,
    `CppLegalizeRuleGenerator.cpp:22-30`, `CppLegalizerGenerator.cpp:44-52`,
    `CppCallingConvGenerator.cpp:19-27`
  - Why: each is used only for include guards.
  - Fix: shared helper.
  - Status: fixed

- [ ] **P1 · DUP-03 — Generator boilerplate repeated across all ten generators**
  - Where: `run()` bodies (`CppCallingConvGenerator.cpp:46`, `CppLegalizeRuleGenerator.cpp:49`,
    `CppLegalizerGenerator.cpp:71`, `CppInstructionSelectorGenerator.cpp:85`,
    `CppTargetInstructionGenerator.cpp:98`, `CppTargetDescGenerator.cpp:422`,
    `CppMirTypeTableGenerator.cpp:565`, `CppEncodingTableGenerator.cpp:196`,
    `CppMirInstructionGenerator.cpp:300`, `CppRegisterInfoGenerator.cpp:425`);
    empty-target normalization in 8 ctors (`CppCallingConvGenerator.cpp:39`,
    `CppEncodingTableGenerator.cpp:95`, …).
  - Fix: `GeneratorConfig` / base-class template method.
  - Status: deferred

- [x] **P1 · DUP-04 — `collect*Symbols` scan reimplemented 13×**
  - Where: `CppMirInstructionGenerator.cpp:175-191`, `CppTargetInstructionGenerator.cpp:78-95`,
    `CppInstructionSelectorGenerator.cpp:45-62,65-82`, `CppEncodingTableGenerator.cpp:125-136`,
    `CppLegalizerGenerator.cpp:139-148`, `CppLegalizeRuleGenerator.cpp:90-99,150-159`, …
  - Fix: `SymbolTable::collect<T>(SymbolType)`.
  - Status: fixed

- [x] **P1 · DUP-05 — Registry pattern duplicated (encoding dialect vs codegen backend)**
  - Where: `Sema/src/Sema/Encoding/EncodingDialect.cpp:24-59` vs
    `CodeGenerators/src/CodeGenerators/CppEncodingTableGenerator.cpp:31-85`
  - Why: identical normalize + function-local-static map; both register `x86_64`/`amd64`/`x86-64`
    aliases (`X86_64EncodingDialect.cpp:365-377`, `X86_64EncodingCodegenBackend.cpp:224-235`).
  - Fix: generic `Registry<T>` (see `CrossProjectReview.md` XPR-01).
  - Status: fixed

- [x] **P1 · DUP-06 — x86 vocabulary stored twice (must stay in sync)**
  - Where: `Sema/src/Sema/Encoding/X86_64EncodingDialect.cpp:37-52` (`isKnownForm`, 25 forms) and
    `:54-67` (`isKnownField`, 11 fields) vs
    `CodeGenerators/src/CodeGenerators/X86_64EncodingCodegenBackend.cpp:14-67` (`formToString`)
    and `:70-95` (`slotToString`)
  - Why: a form passing sema but missing in codegen emits `EncForm::None` silently.
  - Fix: a single shared table of (name, EncForm, EncSlotKind).
  - Status: fixed

- [ ] **P1 · DUP-07 — `Driver.cpp` keeps five parallel `GeneratorKind` chains**
  - Where: `Driver.cpp:413-430` (detect dialect), `:443-465` (resolve kind), `:516-750`
    (`computeExpectedOutputs`), `:1275-1310` (`generatorName`), `:1333-1538` (dispatch)
  - Why: a new generator must be added in five places.
  - Fix: one table (name, dialect, factory, output spec).
  - Status: deferred

- [ ] **P1 · DUP-08 — Driver/generator path-resolution logic duplicated and divergent**
  - Where: `Cli/src/Cli/Driver.cpp:475-499,502-514` vs
    `CodeGenerators/src/CodeGenerators/CodeGenerator.cpp:39-46,49-76`
  - Why: same intent, different extension handling (see WEI-06).
  - Fix: call `CodeGenerator`'s helpers from the driver.
  - Status: deferred

- [ ] **P1 · DUP-09 — Enum→string mapping duplicated between InfoDumper and generators**
  - Where: `Cli/src/Cli/InfoDumper.cpp:68-84,87-111,114-126` vs
    `CppMirTypeTableGenerator.cpp:13-32`, `CppMirInstructionGenerator.cpp:14-56`
  - Fix: one shared enum-string utility.
  - Status: deferred

- [ ] **P1 · DUP-10 — Sema pass preamble/loop scaffolding repeated in all nine passes**
  - Where: `CallingConvPass.cpp:94`, `InstructionSelectPass.cpp:51`, `IrInstructionPass.cpp:13`,
    `LegalizeActionPass.cpp:106`, `LegalizeRulePass.cpp:126`, `RegisterPass.cpp:19-28`,
    `TargetDescPass.cpp:23-30`, `TargetInstPass.cpp:19`, `TypePass.cpp:10-19`
  - Why: same trace + `hasErrors` + loop; inconsistent null-guard diagnostics and sender names
    (`TypePass` vs `Sema::Xxx`).
  - Fix: shared pass driver + consistent diagnostic sender.
  - Status: deferred

- [x] **P2 · DUP-11 — `SymbolTable::getSymByName` implemented twice**
  - Where: `Sema/src/Sema/SymbolTable.cpp:40-56` vs `:58-74`
  - Fix: one implementation with an optional `SymbolType`.
  - Status: fixed

- [x] **P2 · DUP-12 — Hand-written banner in `CppLegalizeRuleGenerator`**
  - Where: `CppLegalizeRuleGenerator.cpp:77,135` vs `CppSourceEmitter::emitBanner`
  - Fix: use `emitBanner`.
  - Status: fixed

## 3. Legacy / un-removed code

- [x] **P1 · LEG-01 — `SemaContext` is a dead class**
  - Where: `Sema/include/Sema/SemaContext.h`, `Sema/src/Sema/SemaContext.cpp` (compiled at
    `Sema/CMakeLists.txt:5,29`)
  - Why: referenced nowhere; passes take `(collector, table, ast)` directly.
  - Fix: delete.
  - Status: fixed

- [x] **P1 · LEG-02 — Six unused free-function generator wrappers**
  - Where: `GenerateEncodingTable`, `GenerateCallingConvDesc`, `GenerateLegalizerActionTable`,
    `GenerateLegalizerRules`, `GenerateTargetInstructionTable`, `GenerateInstructionSelector`
    (headers + cpp; e.g. `CppEncodingTableGenerator.h:59`, `.cpp:225`)
  - Why: zero callers (only four other `Generate*` are used, and only by tests).
  - Fix: delete.
  - Status: fixed

- [ ] **P1 · LEG-03 — Register semantic payload mostly write-only**
  - Where: `Sema/src/SemaPasses/RegisterPass.cpp:97-107,230-251,296-313`
  - Why: production reads only `RegisterFileSymbol::m_astNode`; the rest is read by tests only.
  - Fix: trim or document.
  - Status: deferred

- [ ] **P1 · LEG-04 — Unused `TargetInstructionSymbol` fields and method**
  - Where: `Sema/include/Sema/Symbols/TargetSymbols.h:28,31-32,38`
  - Why: `m_mnemonic`/`m_implicitDefs`/`m_implicitUses` are populated but
    `CppTargetInstructionGenerator.cpp:219-220` emits hard-coded empty lists; `hasFlag()` unused.
  - Fix: emit the fields or stop collecting them.
  - Status: deferred

- [x] **P1 · LEG-05 — Dead/write-only Sema APIs**
  - Where: `SymbolTable.h:23,33`, `Scope.h:36,51,56,61`,
    `InstructionSelectPass.cpp:191` (`m_cost` unused), `IrSymbols.h:42` (`hasFlag`)
  - Fix: delete.
  - Status: fixed

- [x] **P1 · LEG-06 — Dead encoding helpers/AST alternatives**
  - Where: `X86_64EncodingDialect.cpp:16-26` (`findDirective`), `:232` (`m_valid`),
    `Lexer/include/Ast/EncodingDefLangAst.h:37,39` (unused `Value` alternatives),
    header doc `X86_64EncodingDialect.h:55` claiming a return it does not make
  - Fix: delete or implement; fix the doc.
  - Status: fixed

- [x] **P1 · LEG-07 — `RealLiteral` parser unused**
  - Where: `Lexer/include/Parser/CommonParsers.h:146-164`
  - Fix: delete unless a dialect plans to use it.
  - Status: fixed

- [ ] **P1 · LEG-08 — `InfoDumper::dumpSymbols` is stale**
  - Where: `Cli/src/Cli/InfoDumper.cpp:967-1039,1055-1140`
  - Why: only special-cases older symbol types; newer ones fall through to `Other`; text and
    JSON branches disagree.
  - Fix: drive from the symbol type list.
  - Status: deferred

- [ ] **P2 · LEG-09 — Stale CLI help/error text**
  - Where: `CommandLineOptions.cpp:16-19,102`, `Driver.cpp:812-815`,
    `CommandLineOptions.cpp:261-276` (dead special case)
  - Fix: update lists; remove the dead branch.
  - Status: deferred

- [ ] **P2 · LEG-10 — README out of date**
  - Where: `EzDsl/README.md:32-41,312-343`
  - Why: documents removed extensions/options (`.tdf`, `include idf`) and omits several
    generators/options.
  - Fix: refresh.
  - Status: deferred

- [x] **P2 · LEG-11 — Stale test naming from the encoding rename**
  - Where: `tests/EzDslCodeGeneratorsTestSuite/tests/T_CppTargetEncodingGenerator.cpp` and
    `CMakeLists.txt:64,67`
  - Fix: rename to `T_CppEncodingTableGenerator`.
  - Status: fixed

- [x] **P2 · LEG-12 — CodeGenerators PCH still pulls all of lexy**
  - Where: `CodeGenerators/include/EzDslCodeGeneratorsCommon.h:4-9`
  - Why: no CodeGenerators/Sema source uses lexy after the parser/codegen split.
  - Fix: trim the PCH.
  - Status: fixed

## 4. Weird scenarios / old hacks

- [x] **P1 · WEI-01 — `setTargetName` desyncs the resolved backend**
  - Where: `CppEncodingTableGenerator.h:47`, `.cpp:99,209`
  - Why: `m_backend` is resolved in the constructor; `setTargetName` changes only the filename.
  - Fix: re-resolve on set, or remove the setter.
  - Status: fixed

- [x] **P1 · WEI-02 — Target-name sanitization is inconsistent; `x86-64` yields invalid C++**
  - Where: only `CppTargetDescGenerator.cpp:147`, `CppRegisterInfoGenerator.cpp:203`,
    `CppEncodingTableGenerator.cpp:209` sanitize; `CppInstructionSelectorGenerator.cpp:110,136`,
    `CppLegalizerGenerator.cpp:96,105`, `CppLegalizeRuleGenerator.cpp:74,86`,
    `CppTargetInstructionGenerator.cpp:123,135`, `CppCallingConvGenerator.cpp:88,116` do not.
  - Why: `x86-64` is a registered alias, so `--target x86-64` generates uncompilable code.
  - Fix: sanitize once at the Driver/`CodeGenerator` boundary.
  - Status: fixed

- [x] **P1 · WEI-03 — `WriteFileIfChanged` is documented atomic but is not**
  - Where: `CodeGenerator.h:54`, `CodeGenerator.cpp:79,117-135`
  - Why: opens the destination directly with truncation; a crash leaves a corrupt file; reads
    the file once for comparison and again to write.
  - Fix: temp file + rename, or fix the comments.
  - Status: fixed

- [x] **P1 · WEI-04 — Encoding decoded twice; codegen fails silently**
  - Where: `X86_64EncodingDialect.cpp:243-247` (sema) and
    `X86_64EncodingCodegenBackend.cpp:243,247` (codegen returns `EncodingDesc{}` with no diagnostic)
  - Fix: emit a diagnostic (or share the decoded spec).
  - Status: fixed

- [x] **P1 · WEI-05 — `.tyf`/`.reg`/`.tdesc` parsers accept trailing garbage**
  - Where: `TypeDefLang.h:46-47`, `RegisterDefLang.h:219-220`, `TargetDescDefLang.h:282`
    (other root parsers use `dsl::terminator(dsl::eof)`).
  - Fix: add `dsl::terminator(dsl::eof)`.
  - Status: fixed

- [ ] **P1 · WEI-06 — Driver and generator disagree on output-path interpretation**
  - Where: `Driver.cpp:498` vs `CodeGenerator.cpp:60,67-72`
  - Why: unknown extensions and uppercase extensions resolve differently, so `--dump-files`
    can report paths that are not written.
  - Fix: single shared resolver (DUP-08).
  - Status: deferred

- [x] **P2 · WEI-07 — Stray `};;` after generated classes**
  - Where: `CppCallingConvGenerator.cpp:166`, `CppLegalizerGenerator.cpp:117`
  - Fix: remove the extra `;`.
  - Status: fixed

- [x] **P2 · WEI-08 — No-op ternary in generated table**
  - Where: `CppTargetInstructionGenerator.cpp:223` (`(i + 1 == size) ? "" : ""`)
  - Fix: remove or implement the intended last-element handling.
  - Status: fixed

- [x] **P2 · WEI-09 — Sentinel collisions in O(n) lookup helpers**
  - Where: `CppLegalizerGenerator.cpp:272-286` (`0` = not found but a valid index),
    `CppCallingConvGenerator.cpp:202-215` (`findRegId` returns `0`)
  - Fix: use `std::optional`/`unordered_map` (see OPT-01).
  - Status: fixed

- [ ] **P2 · WEI-10 — Default dialect depends on static-init order**
  - Where: `EncodingDialect.cpp:30-34,59`, `TargetInstPass.cpp:121-125`
  - Why: `getDefaultEncodingDialect()` returns "last registered"; adding a second ISA makes
    behavior link-order dependent.
  - Fix: explicit default constant.
  - Status: deferred

## 5. Easy optimization checks

- [x] **P1 · OPT-01 — O(n²) string interning in the legalizer generator**
  - Where: `CppLegalizerGenerator.cpp:189-198,202-211,272-286` called inside loops
  - Fix: `unordered_map<string_view,uint16_t>`; also removes the sentinel ambiguity.
  - Status: fixed

- [ ] **P1 · OPT-02 — Symbol/AST data collected 2–3× per generator**
  - Where: `CppRegisterInfoGenerator.cpp:202,432`, `CppTargetInstructionGenerator.cpp:139,180`,
    `CppInstructionSelectorGenerator.cpp:120,210`, `CppLegalizeRuleGenerator.cpp:90,151`,
    `CppTargetDescGenerator.cpp:146,282,430`
  - Fix: collect once in `run()` and pass down.
  - Status: deferred

- [x] **P1 · OPT-03 — `EncodingCodegenBackend` virtuals return `std::string` for constants**
  - Where: `EncodingCodegenBackend.h:27,30,33` (overrides
    `X86_64EncodingCodegenBackend.h:19-21`); `arrayType()` called 4× per table
    (`CppEncodingTableGenerator.cpp:138,148,169,181`)
  - Fix: return `string_view`.
  - Status: fixed

- [ ] **P2 · OPT-04 — `InfoDumper` enum→string helpers allocate and take `int`**
  - Where: `InfoDumper.cpp:68,87,114` with `static_cast<int>` at `:974,991,1066,1082`
  - Fix: `string_view` + typed enums.
  - Status: deferred

- [x] **P2 · OPT-05 — `WriteFileIfChanged` extra full copy**
  - Where: `CodeGenerator.cpp:89-91`
  - Fix: compare via `std::string`/size first; use `error_code` overloads.
  - Status: fixed

- [ ] **P2 · OPT-06 — Repeated work in `CommandLineOptions::parse`**
  - Where: `CommandLineOptions.cpp:218-221,245-259,279-318`
  - Fix: option→generator table; one lowercase helper.
  - Status: deferred

- [ ] **P2 · OPT-07 — Driver classifies outputs via `path.string()` map keys**
  - Where: `Driver.cpp:823-831,1552-1568`
  - Fix: key by `std::filesystem::path` or cache mtime in `OutputFileInfo`.
  - Status: deferred

## 6. Hot spots

- `Cli/src/Cli/Driver.cpp` — `Driver::run` (830 lines), DUP-07/08, WEI-02/06.
- `CodeGenerators/src/CodeGenerators/CppLegalizerGenerator.cpp` (`emitSource` 539 lines) and
  `CppLegalizeRuleGenerator.cpp` (`emitSource` 492) — DUP-03/04, OPT-01/02.
- `CodeGenerators/src/CodeGenerators/CppRegisterInfoGenerator.cpp` (`emitHeader` 222) — OPT-02.
- `Sema/src/Sema/Encoding/X86_64EncodingDialect.cpp` (`decodeX86_64Encoding` 148) — DUP-06, WEI-04.
- `Cli/src/Cli/InfoDumper.cpp` (`dumpSymbols` 202) — LEG-08, OPT-04.

## 7. Verification gate

Use the standard gate in `ReviewProcess.md`. The generated x86-64 encoding table must stay
byte-identical; run the table diff and the full `ctest`/`TEST_ALL` suites. For WEI-02 add a
generator test using `--target x86-64` and assert the emitted identifiers are valid.

## 8. Findings tracker

| ID | Severity | Category | Status | Owner | Notes |
| --- | --- | --- | --- | --- | --- |
| WEI-01..05, WEI-07..09 | P1/P2 | Weird | fixed | — | backend re-resolve, boundary sanitize, atomic writes, diagnostics, eof |
| DUP-01..02, DUP-04..06, DUP-11..12 | P1/P2 | Duplication | fixed | — | shared string/collect/vocabulary helpers (XPR-02/03/10) |
| LEG-01..02, LEG-05..07, LEG-11..12 | P1/P2 | Legacy | fixed | — | dead SemaContext/wrappers/helpers, test rename, PCH trim |
| OPT-01, OPT-03, OPT-05 | P1/P2 | Optimization | fixed | — | interning, string_view returns, write comparison |
| DUP-03, DUP-07..10, LEG-03..04, LEG-08..10, WEI-06, WEI-10, OPT-02, OPT-04, OPT-06..07 | P2 | mixed | deferred | — | generator/driver/path/enum/pass/escape scaffolds; docs/payload polish |
