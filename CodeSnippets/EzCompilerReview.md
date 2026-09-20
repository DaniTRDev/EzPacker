# EzCompiler Review

## 1. Scope & entry points

- `EzCompiler/include|src/Main.cpp`, `CommandLineOptions`, `DriverContext`.
- `EzCompiler/include|src/CompilationPipeline.cpp` — pass scheduling and MIR/assembly dumps.
- `EzCompiler/include|src/EmissionEngine.cpp` — sections, symbols, relocations, writers.
- `EzCompiler/include|src/TargetResolver`, `TargetTriple`, `FrontendAdapter`.
- `EzCompiler/src/Targets/X86_64TargetRegistration.cpp`.
- Build: `EzCompiler/CMakeLists.txt`.

All leads are unverified until checked. See `ReviewProcess.md`.

## 2. Duplicated code

- [ ] **P0 · DUP-01 — x86-64 branch patching duplicated; the target resolver is bypassed**
  - Where: `EzCompiler/src/EmissionEngine.cpp:251-294` (inline opcode recognition + `target - nextRip`)
    vs `EzTriple/src/Targets/X86_64/X86_64RelocationResolver.cpp:25-84`
  - Why: `TargetDesc::getRelocationResolver()` is never called in production (only in
    `tests/EzTripleTestSuite/tests/T_X86_64TargetDesc.cpp:195`); the two copies of frame-size and
    offset knowledge can diverge. This is the clearest leftover of the emitter-seam refactor.
  - Fix: route patching through the target's relocation resolver.
  - Status: new

- [ ] **P1 · DUP-02 — ELF/COFF writer setup duplicated**
  - Where: `EzCompiler/src/EmissionEngine.cpp:357-388`
  - Why: two identical `addSymbol`/`addRelocation` loops differing only in writer type.
  - Fix: an `IObjectWriter` interface or a templated helper.
  - Status: new

- [ ] **P1 · DUP-03 — Undefined-symbol creation duplicated**
  - Where: `EzCompiler/src/EmissionEngine.cpp:186-193` vs `:317-322`
  - Fix: one helper.
  - Status: new

- [ ] **P1 · DUP-04 — Pass-pipeline scaffolding duplicated per stage**
  - Where: `EzCompiler/src/CompilationPipeline.cpp:73-201`
  - Why: each stage rebuilds a `MirPassManager`, calls `setTestMode()`, repeats the `printPasses`
    block and seven near-identical error blocks; done inside the per-function loop.
  - Fix: `runPass<PassT>(name, args...)` helper; build the manager once.
  - Status: new

- [ ] **P1 · DUP-05 — `dumpCurrentMir` vs `dumpAssembly` duplicate the walk**
  - Where: `EzCompiler/src/CompilationPipeline.cpp:203-230` vs `:232-267`
  - Fix: one walker parameterized by the formatter.
  - Status: new

- [ ] **P2 · DUP-06 — `CommandLineParser` mirrors the EzDsl/Cli parser and is set up twice**
  - Where: `EzCompiler/src/CommandLineOptions.cpp:7,116` (constructor + `parse` both call
    `setupArguments()`); overall shape mirrors `EzDsl/Cli/src/Cli/CommandLineOptions.cpp`.
  - Fix: share the scaffold; set up once.
  - Status: new

- [ ] **P2 · DUP-07 — Target ownership triplet duplicated and re-validated**
  - Where: `include/TargetResolver.h:18-23` vs `include/DriverContext.h:82-84` /
    `src/DriverContext.cpp:27-35`, re-checked at `EmissionEngine.cpp:29-49`
  - Fix: keep one owner; trust `initialize()` in the engine.
  - Status: new

- [ ] **P2 · DUP-08 — Arena construction written three ways**
  - Where: `src/DriverContext.cpp:43,55,63`
  - Fix: one factory.
  - Status: new

## 3. Legacy / un-removed code

- [ ] **P1 · LEG-01 — `EmissionEngine::emitFunction` is declared but never defined/called**
  - Where: `EzCompiler/include/EmissionEngine.h:37`
  - Why: introduced in `66d759e` with no body; aggregation lives inline in `emitModule`.
  - Fix: delete.
  - Status: new

- [ ] **P1 · LEG-02 — `getFunctionAllocator`/`resetFunctionAllocator` unreachable**
  - Where: `EzCompiler/src/DriverContext.cpp:50-64`, `include/DriverContext.h:50,55`
  - Why: zero call sites; `m_functionArena` is still eagerly allocated (`:43`).
  - Fix: wire into the per-function loop or delete arena + methods.
  - Status: new

- [ ] **P1 · LEG-03 — Parsed options never consumed (user-visible dead surface)**
  - Where: `src/CommandLineOptions.cpp:54-57` (`--emit-obj`), `:59` (`-O0`), `:206` (`timePasses`),
    `:207` (`isPositionIndependent`), `:208` (`compileOnly`), `:211-227` (`diagThreshold`),
    `include/CommandLineOptions.h:51` (`color`)
  - Why: none are read; `-O0` cannot work because the else-branch hardcodes O0, and `-fPIC` cannot
    take effect because the ELF descriptor is always constructed with `isPic=false`.
  - Fix: implement or remove each; update help.
  - Status: new

- [ ] **P1 · LEG-04 — Dead public API / accessors**
  - Where: `src/CommandLineOptions.cpp:232` + `include/CommandLineOptions.h:80` (`printHelp`),
    `include/DriverContext.h:58` (mutable `getOptions`)
  - Fix: delete.
  - Status: new

- [ ] **P1 · LEG-05 — `TargetTriple` predicates unused in production**
  - Where: `src/TargetTriple.cpp:100-115` (`isX86_64/isLinux/isElf/isCoff`)
  - Why: only tests use them; `isElf`'s fallback is never exercised.
  - Fix: delete or document.
  - Status: new

- [ ] **P1 · LEG-06 — Stale includes in the target registration TU**
  - Where: `src/Targets/X86_64TargetRegistration.cpp:4-11`
  - Why: seven includes (`TargetRelocationResolver`, `MirFrameLowerer`, `MirAddressingModeMatcher`,
    `MirInstructionSelector`, `LegalizerInfo`, `MirLegalizer`, `MirRegisterAllocator`) are unused.
  - Fix: trim.
  - Status: new

- [ ] **P1 · LEG-07 — Dead alignment/code-model APIs, so functions are never aligned**
  - Where: `EzTriple/include/Descriptors/TargetBinaryDesc.h:65,79,84`
  - Why: no production call sites; `EmissionEngine` never aligns `.text`.
  - Fix: honor function alignment during emission or delete the API.
  - Status: new

- [ ] **P1 · LEG-08 — Placeholder frontend silently ignores non-MIR input**
  - Where: `src/FrontendAdapter.cpp:39-42` (and unused `outMirCtx` on the empty-path branch `:20-25`)
  - Why: any non-`.mir` file compiles to `main(){return 42;}`.
  - Fix: reject unsupported inputs with a diagnostic until the real frontend lands.
  - Status: new

- [ ] **P2 · LEG-09 — `SectionType::Custom` emitted but unsupported by both writers**
  - Where: `EmissionEngine.cpp:188,318`; `CoffWriter.cpp:167-170`, `Elf64Writer.cpp:228-231`
  - Why: works by accident for undefined symbols; any real `Custom` section will silently emit
    index 0. The section is allocated (`Helpers.cpp:66-73,144-151`) but never written.
  - Fix: implement or forbid `Custom`.
  - Status: new

## 4. Weird scenarios / old hacks

- [ ] **P0 · WEI-01 — Global-variable symbol offsets computed before alignment padding**
  - Where: `EmissionEngine.cpp:120-124` with `CodeSection.cpp:262-283` vs `:134-176`
  - Why: `getCurrentOffset()` ignores pending `Align` nodes, so a symbol after a padded region is
    recorded at the wrong offset (e.g. 1-byte global followed by an 8-byte global).
  - Fix: account for pending alignment when computing offsets.
  - Status: new

- [ ] **P1 · WEI-02 — Shared sections are finalized multiple times**
  - Where: `EmissionEngine.cpp:236-242` iterating `binDesc->getSections()`, where
    `Helpers::ObjectFormat::CreateElfSections` aliases one section under several keys
    (`EzCodeEmitter/src/Helpers.cpp:153-166`)
  - Why: `.rodata`/`.data` finalized repeatedly in unspecified order.
  - Fix: finalize over a deduplicated set of `CodeSection*`.
  - Status: new

- [ ] **P1 · WEI-03 — Emitter failures are silently swallowed**
  - Where: `GenericCodeEmitter.h:14-15` (contract: throw), but
    `EzCodeEmitter/src/X86_64/X86_64CodeEmitter.cpp:339-348` returns `void` and discards the
    `tryEmitTableDriven` result
  - Why: missing encodings/operands drop instructions with no diagnostic and a malformed object.
  - Fix: make failures observable (diagnostic or exception).
  - Status: new

- [ ] **P1 · WEI-04 — Relocation type/section ignored; semantics inferred from opcode bytes**
  - Where: `EmissionEngine.cpp:251-294,327-331,347-351`
  - Why: `ref->isFunction()` is always treated as `BranchRel32` and `ref->isGlobalVar()` as
    `PCRel32`; only `E9`/`0F 8x`/`E8` are handled, with `instOffset + 1` hardcoded.
  - Fix: use `CodeRelocation::m_relocType` and the target resolver (DUP-01).
  - Status: new

- [ ] **P1 · WEI-05 — Unchecked output writes / no atomic output**
  - Where: `EmissionEngine.cpp:391,397-398`
  - Why: only `is_open()` is checked; `write`/`close` errors leave a truncated object and return
    `true`.
  - Fix: check stream state; write to a temp then rename.
  - Status: new

- [ ] **P1 · WEI-06 — `const_cast` span to satisfy the emitter API**
  - Where: `EmissionEngine.cpp:217-218`
  - Why: `getOperands()` exposes `const MirOperand**` but `emitInst` wants `std::span<MirOperand*>`.
  - Fix: fix the interface.
  - Status: new

- [ ] **P1 · WEI-07 — Emitter ownership via raw `new` across the seam**
  - Where: `EzTriple/src/Targets/X86_64/X86_64TargetDesc.cpp:247` + `EmissionEngine.cpp:53`
  - Why: contract "caller owns"; a target returning `nullptr`/throwing is a trap.
  - Fix: return `std::unique_ptr<GenericCodeEmitter>`.
  - Status: new

- [ ] **P2 · WEI-08 — Uncaught filesystem exceptions**
  - Where: `FrontendAdapter.cpp:27-28,47-48`, `DriverContext.cpp:13`; `Main.cpp` has no
    top-level try/catch.
  - Fix: catch at the entry point or use `error_code`.
  - Status: new

- [ ] **P2 · WEI-09 — Silent precedence for conflicting flags**
  - Where: `CommandLineOptions.cpp:165-184,210-227`
  - Why: multiple emit modes pick one silently; unknown/`info` diag levels ignored.
  - Fix: diagnose conflicts.
  - Status: new

## 5. Easy optimization checks

- [ ] **P1 · OPT-01 — O(n²) symbol-name lookup during relocation resolution**
  - Where: `EmissionEngine.cpp:306-314`
  - Fix: maintain an `unordered_set<string>` (or reuse `funcById`).
  - Status: new

- [ ] **P1 · OPT-02 — `CodeSection::getCurrentOffset` is O(nodes) and called per symbol/function**
  - Where: `EzCodeEmitter/src/CodeSection.cpp:262-283`, callers `EmissionEngine.cpp`
  - Fix: maintain a running byte count.
  - Status: new

- [ ] **P1 · OPT-03 — `MirPassManager` rebuilt per function per stage**
  - Where: `CompilationPipeline.cpp:73-201` inside the loop at `:34`
  - Fix: build managers/analyses once.
  - Status: new

- [ ] **P2 · OPT-04 — Name copies in the emission path**
  - Where: `EmissionEngine.cpp:164,187,226,301,317,340`
  - Why: `ObjectSymbol::m_name`/`ObjectRelocEntry::m_symbolName` are `std::string`.
  - Fix: `string_view`/`pmr::string`.
  - Status: new

- [ ] **P2 · OPT-05 — Unreserved containers**
  - Where: `EmissionEngine.cpp:60-62` (`funcById`, `gvarById`), `symbols`; writer string tables
    `Elf64Writer.cpp:135-149`, `CoffWriter.cpp:177-183`
  - Fix: reserve.
  - Status: new

- [ ] **P2 · OPT-06 — Whole-file read inefficiency**
  - Where: `FrontendAdapter.cpp:62` (`istreambuf_iterator`)
  - Fix: seek/size + `resize` + `read`.
  - Status: new

- [ ] **P2 · OPT-07 — `ostringstream` dumps without reserve**
  - Where: `CompilationPipeline.cpp:205,234`
  - Fix: reserve or use `std::string`/`format`.
  - Status: new

- [ ] **P2 · OPT-08 — Minor const/pass-by-value polish**
  - Where: `EzCodeEmitter/include/CodeEmitterContext.h:91` (`const std::string_view&`),
    `TargetTriple.h:39-42` (missing `noexcept`), `TargetTriple.cpp:20-40` (string copies)
  - Fix: tighten.
  - Status: new

## 6. Hot spots

- `src/EmissionEngine.cpp` — `emitModule` (379 lines): DUP-01/02/03, WEI-01..07, OPT-01/04/05.
- `src/CompilationPipeline.cpp` — DUP-04/05, OPT-03/07.
- `src/CommandLineOptions.cpp` — LEG-03/04, WEI-09, DUP-06.
- `src/FrontendAdapter.cpp` — LEG-08, WEI-08, OPT-06.
- `src/Targets/X86_64TargetRegistration.cpp` — LEG-06.

## 7. Verification gate

Use the standard gate in `ReviewProcess.md`. For DUP-01/WEI-04, end-to-end object output must be
byte-identical after routing through `X86_64RelocationResolver`; capture a `sha256sum` of a sample
`.o` before/after. For WEI-01, add a module with mixed 1-/8-byte globals and assert symbol offsets.

## 8. Findings tracker

| ID | Severity | Category | Status | Owner | Notes |
| --- | --- | --- | --- | --- | --- |
| DUP-01 | P0 | Duplication | new | — | branch patching vs resolver |
| WEI-01 | P0 | Weird | new | — | global symbol offsets vs alignment |
| DUP-02..05 | P1 | Duplication | new | — | writers/symbols/pipeline/dumps |
| LEG-01..08 | P1 | Legacy | new | — | dead API/options/frontend |
| WEI-02..07 | P1 | Weird | new | — | finalize/swallowed errors/ownership |
| OPT-01..03 | P1 | Optimization | new | — | lookups/offset/manager rebuild |
| DUP-06..08, LEG-09, WEI-08..09, OPT-04..08 | P2 | mixed | new | — | polish |
