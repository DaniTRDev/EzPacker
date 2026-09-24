# EzMir Review

## 1. Scope & entry points

- `EzMir/include|src/Parser/` — `MirLexer`, `MirParser`, `MirParserContext`.
- `EzMir/include|src/Instruction/` — `MirInstruction`, `MirInstructionBuilder`,
  `MirInstructionSet`, `MirTargetInstructionDesc`.
- `EzMir/include|src/Operand/` — operands, register banks/classes, register references.
- `EzMir/include|src/Builder/`, `Block/`, `Function/`, `GlobalVar/`, `Type/`.
- `EzMir/include|src/MirPasses/` — pass manager and the analysis/transform passes.
- `EzMir/include|src/Printer/`. Build: `EzMir/CMakeLists.txt`; generated tables under
  `EzMir/CMake/EzDslGen*.cmake`.

All leads are unverified until checked. See `ReviewProcess.md`.

## 2. Duplicated code

- [x] **P1 · DUP-01 — Two copies of instruction-statement parsing**
  - Where: `EzMir/src/Parser/MirParser.cpp:704-796` (`parseBasicBlock` inline)
    vs `:814-897` (`parseInstructionStatement`)
  - Why: both implement assignment form, prefix form, optional type, operand loop and
    semicolon matching; they have already drifted because one inspects `tok1` first.
  - Fix: consolidate on `parseInstructionStatement` with proper lookahead.
  - Status: fixed

- [x] **P1 · DUP-02 — Memory-operand parsing duplicated**
  - Where: `EzMir/src/Parser/MirParser.cpp:903-957` vs `:1032-1071`
  - Why: the base/index/scale/displacement loop is equivalent; the second exists only because
    the `[...]` branch already consumed `valTok`.
  - Fix: single `parseMemoryOperand` once token pushback exists (WEI-07).
  - Status: fixed

- [x] **P1 · DUP-03 — `declareRegister` vs `getOrCreateRegister` near-identical**
  - Where: `EzMir/src/Parser/MirParserContext.cpp:196-236` vs `:256-290`
  - Why: same physical-register detection + `from_chars`; only the duplicate check differs.
  - Fix: one helper with an `allowReuse` flag.
  - Status: fixed

- [x] **P1 · DUP-04 — `MirOperandBuilder::buildMem` validates four times; int/float twins**
  - Where: `EzMir/src/Operand/MirOperandBuilder.cpp:116-221` and `:24-111`
  - Why: four overloads repeat base/index checks; `buildFloat`/`buildInt` differ only in
    kind/type lookup/extend args.
  - Fix: templated helper + shared validation.
  - Status: fixed

- [x] **P1 · DUP-05 — `MirInstructionBuilder::build`/`buildTarget` triplicated**
  - Where: `EzMir/src/Instruction/MirInstructionBuilder.cpp:85-132,137-192`
  - Why: same body per container type.
  - Fix: `buildImpl(span)` template.
  - Status: fixed

- [x] **P1 · DUP-06 — Instruction operand formatting duplicated in the printer**
  - Where: `EzMir/src/Operand/MirOperands.cpp:47-66,102-170` vs
    `EzMir/src/Printer/MirPrinter.cpp:255-376`
  - Why: register/memory/reference formatting re-implemented with subtly different output
    (`%p{id}` vs `%block_{id}`, `+0x…`) — a round-trip divergence risk.
  - Fix: route the printer through a shared `formatOperand`.
  - Status: fixed

- [x] **P2 · DUP-07 — def/use unregistration copy-pasted**
  - Where: `EzMir/src/Instruction/MirInstruction.cpp:222-340`,
    `EzMir/src/Instruction/MirInstructionBuilder.cpp:30-46,432-469`
  - Why: three independent walks that specialize `MirMemory` and test virtuality; the index
    flag is missing in `getDefinedRegisters`.
  - Fix: a shared `visitOperandRegisters(op, flag, fn)`.
  - Status: fixed

- [x] **P2 · DUP-08 — `ArgumentLocationDesc` stores a redundant discriminant**
  - Where: `EzMir/include/Function/ArgumentLocationDesc.h:174-184`
  - Why: `m_type` mirrors `m_storage.index()` and can desync.
  - Fix: derive type from the variant.
  - Status: fixed

## 3. Legacy / un-removed code

- [x] **P1 · LEG-01 — Unused second `MirFunctionBuilder` constructor (+ TODO)**
  - Where: `EzMir/include/Function/MirFunctionBuilder.h:7-9,26`,
    `EzMir/src/Function/MirFunctionBuilder.cpp:22-26,127-130`
  - Why: the `(ctx, std::pmr::vector<MirFunction*>*)` ctor has zero callers and duplicates
    `MirBuilderContext::getFunctions()`; the TODO says to add a `MirModule`.
  - Fix: delete the ctor (and the stale TODO) or introduce `MirModule`.
  - Status: fixed

- [x] **P1 · LEG-02 — `MirFunction::addCalleeSavedRegUse` is dead**
  - Where: `EzMir/include/Function/MirFunction.h:156`, `EzMir/src/Function/MirFunction.cpp:152`
  - Why: no callers; duplicates `MirFunctionBuilder::addPhysRegUse`.
  - Fix: delete.
  - Status: fixed

- [x] **P1 · LEG-03 — Dead `CallLoweringState` API**
  - Where: `EzMir/include/Function/CallLoweringState.h:29,69`,
    `EzMir/src/Function/CallLoweringState.cpp:61-73,78-82`
  - Why: `getUsableRegCount`, `getUsedRegCount`, `getArgIndex` have no callers.
  - Fix: delete (keep `advanceArg`/`advanceSlot`/`getBankCursor`).
  - Status: fixed

- [x] **P1 · LEG-04 — Dead `MirTargetInstructionDesc` API**
  - Where: `EzMir/include/Instruction/MirTargetInstructionDesc.h:33-38,58,73`
  - Why: the "backward-compatible" ctor and `get/setOperandClasses` have no callers; ctor
    bodies duplicate the `m_operandsFlags` insert.
  - Fix: delete.
  - Status: fixed

- [x] **P1 · LEG-05 — Dead `MirType` API; `m_isTrivial` constant**
  - Where: `EzMir/include/Type/MirType.h:79,109`
  - Why: `getArrayElementCount`/`setNonTrivial` unused, so `isTrivial()` always true.
  - Fix: delete or wire up.
  - Status: fixed

- [x] **P1 · LEG-06 — Dead public parser entry points**
  - Where: `EzMir/include/Parser/MirParser.h:52,57`
  - Why: `parseFunction`/`parseInstruction` have no in-repo callers; `parseFunction` also has a
    questionable `getFunctions().back()` fallback (`MirParser.cpp:1345`).
  - Fix: delete or route callers through them.
  - Status: fixed

- [x] **P1 · LEG-07 — Unused global tables and context map**
  - Where: `EzMir/include/Operand/MirOperand.h:24` (`g_MirOperandType2Str`),
    `EzMir/include/Instruction/MirInstructionMetadata.h:160` (`g_MirInstructionCategory2Str`),
    `EzMir/include/Builder/MirBuilderContext.h:120` (`m_typeIdToClass`)
  - Why: never referenced; the two maps also force static-init allocations.
  - Fix: delete.
  - Status: fixed

- [x] **P1 · LEG-08 — Dead file-scope `getRegInfo`**
  - Where: `EzMir/src/Instruction/MirInstructionBuilder.cpp:16-24`
  - Why: shadowed by the member `getRegInfo` (`:354`); external linkage on a dead function.
  - Fix: delete.
  - Status: fixed

- [x] **P1 · LEG-09 — `MirPass::getResult()` is dead**
  - Where: `EzMir/include/MirPasses/MirPass.h:89`, `EzMir/src/MirPasses/MirPass.cpp:6`
  - Why: never called; the manager keeps its own results.
  - Fix: delete.
  - Status: fixed

- [x] **P2 · LEG-10 — Variadic operand slot branch in `getOperandFlag` is unreachable**
  - Where: `EzMir/src/Instruction/MirInstruction.cpp:149-198`
  - Why: generated metadata never emits `VariadicArgs`, so the branch is dead and
    `UNMERGE_VALUES` extra destinations fall through as `Read` (wrong def/use).
  - Fix: encode variadic slots in metadata or special-case OUT-variadic instructions.
  - Status: fixed

## 4. Weird scenarios / old hacks

- [x] **P0 · WEI-01 — Analysis cache returns the last function's result**
  - Where: `EzMir/src/MirPasses/MirPassManager.cpp:199`,
    `EzMir/src/MirPasses/Passes/LivenessAnalysisPass.cpp:88`,
    `EzTriple/src/RegisterAllocator/MirRegisterAllocatorPass.cpp:60-63`
  - Why: `LivenessAnalysisPass::run` resets per function while the manager resets once, so the
    cached `getResult()` holds only the last function's liveness; every other function gets the
    wrong live sets. `CodeFlowAnalysisPass` does not reset per function — inconsistent contract.
  - Fix: define one `reset()` contract and/or key cached results by function.
  - Status: fixed

- [x] **P0 · WEI-02 — PMR containers escape the arena in `MirBuilderContext`**
  - Where: `EzMir/src/Builder/MirBuilderContext.cpp:19-21`,
    `EzMir/include/Builder/MirBuilderContext.h:117,120,123`
  - Why: only three maps are built on `m_globalResource`; `m_globalVars`, the hot
    `m_registerIdToRegister`, and `m_typeIdToClass` use the default heap, contradicting the
    arena design.
  - Fix: construct with `m_globalResource`.
  - Status: fixed

- [x] **P1 · WEI-03 — SSA rename skips `ReadWrite` operands**
  - Where: `EzMir/src/MirPasses/Passes/NonSsaToSsaPass.cpp:449,494`
  - Why: exact `==` comparisons against `Read`/`Write` miss `ReadWrite`; builder code uses
    bitwise `&`.
  - Fix: use bitwise flag tests.
  - Status: fixed

- [x] **P1 · WEI-04 — Parser `SourceManager` lifetime vs retained `SourceReference`s**
  - Where: `EzMir/src/Parser/MirParser.cpp:1270-1273,1333-1336,1364-1367`
  - Why: a local `SourceManager` owns the buffers that later diagnostic lookups resolve.
  - Fix: keep the manager alive for the MIR lifetime, or copy owned text into references.
  - Status: fixed

- [x] **P1 · WEI-05 — Physical-register detection is a `"p"`-prefix hack with ignored errors**
  - Where: `EzMir/src/Parser/MirParserContext.cpp:222-227,276-281`
  - Why: any name starting with `p` (e.g. `param`) becomes physical reg 0; `from_chars` errors
    are ignored.
  - Fix: require the `%p<digits>` form and handle parse failure.
  - Status: fixed

- [x] **P1 · WEI-06 — Lexer token "restore" hack**
  - Where: `EzMir/src/Parser/MirParser.cpp:708-723`
  - Why: consuming `tok1` to test for a label can only be emulated by treating it as the opcode —
    the root cause of DUP-01/DUP-02.
  - Fix: add two-token lookahead / pushback.
  - Status: fixed

- [x] **P1 · WEI-07 — Ignored `matchToken` results and discarded syntax**
  - Where: `EzMir/src/Parser/MirParser.cpp:947,1009,1067,1111-1113,1133-1141`
  - Why: malformed input can desync tokens; `%p0(rax:GPR64)` class binding is parsed and thrown away.
  - Fix: check results; apply or reject the class binding.
  - Status: fixed

- [x] **P1 · WEI-08 — Builders return half-constructed objects on error**
  - Where: `EzMir/src/Function/MirFunctionBuilder.cpp:32-42`,
    `EzMir/src/Block/MirBlockBuilder.cpp:51-61`
  - Why: null-context builders look valid but crash on the next build call.
  - Fix: `std::optional`/nullptr with checked call sites.
  - Status: fixed

- [x] **P2 · WEI-09 — `MirRegisterRef::operator<` orders by pointer address**
  - Where: `EzMir/src/Operand/MirRegisterReference.cpp:92-93`
  - Why: ordered-key use becomes non-deterministic across runs.
  - Fix: order by `(class id/name, register id)`.
  - Status: fixed

- [x] **P2 · WEI-10 — Nondeterministic iteration in pipeline/SSA**
  - Where: `EzMir/src/MirPasses/MirPassManager.cpp:220`,
    `EzMir/src/MirPasses/Passes/NonSsaToSsaPass.cpp:296`
  - Why: `unordered_map` iteration changes scheduling/phi order and printed IR.
  - Fix: stable ordering for scheduling and phi insertion.
  - Status: fixed

- [x] **P2 · WEI-11 — `idom::intersect` uses `operator[]` (possible hang)**
  - Where: `EzMir/src/MirPasses/Passes/NonSsaToSsaPass.cpp:191-198,326-327`
  - Why: indexing inserts zero entries that can spin (0↔0); null block deref at the phi site.
  - Fix: use `find`/at with guards.
  - Status: fixed

- [x] **P2 · WEI-12 — Mixed error channels (exceptions vs diagnostics vs silent)**
  - Where: `EzMir/src/Instruction/MirInstructionBuilder.cpp:199-202`,
    `EzMir/src/Function/ArgumentLocationDesc.cpp:52-91`,
    `EzMir/src/MirPasses/MirPassManager.cpp:187,251,257`,
    `EzMir/src/Parser/MirParser.cpp:42-55,1303-1318`
  - Why: forces blanket try/catch and dual handling.
  - Fix: pick one contract per layer.
  - Status: fixed

- [x] **P2 · WEI-13 — Null-deref hazards on public entry points**
  - Where: `EzMir/src/GlobalVar/MirGlobalVarBuilder.cpp:30-37`,
    `EzMir/src/Block/MirBlockBuilder.cpp:66-70`,
    `EzMir/src/Printer/MirPrinter.cpp:427,506`,
    `EzMir/src/Operand/MirOperandBuilder.cpp:28,74,120,141,163,174,197,208`,
    `EzMir/src/Operand/MirOperands.cpp:16,33,106`
  - Why: checks are inconsistent; malformed input crashes the printer/builders.
  - Fix: guard or assert.
  - Status: fixed

- [x] **P1 · WEI-14 — `std::hash<MirRegisterRef>` mixes in a class pointer**
  - Where: `EzMir/include/Operand/MirRegisterReference.h:110-129`
  - Why: hashing the physical register's `MirRegisterClass*` (an arena address) makes
    unordered-container iteration — and therefore register allocation and emitted object
    bytes — vary with ASLR across runs.
  - Fix: hash the class *name* instead, matching `operator<` (WEI-09). Verified deterministic
    across 25 ELF and 20 COFF compilations.
  - Status: fixed

## 5. Easy optimization checks

- [x] **P1 · OPT-01 — Eager `MirPrinter::printToString` in trace paths**
  - Where: `EzMir/src/Instruction/MirInstructionBuilder.cpp:204-207,386-389`,
    `EzMir/src/Function/MirFunctionBuilder.cpp:116-118`,
    `EzMir/src/GlobalVar/MirGlobalVarBuilder.cpp:62-65`
  - Why: formats the full instruction for every build even when trace is disabled.
  - Fix: guard with `isDiagEnabledForType(Diag_Trace)` or a lazy callback.
  - Status: fixed

- [x] **P1 · OPT-02 — `MirFunctionRegisterInfo::getUses` returns a copy**
  - Where: `EzMir/src/Function/MirFunctionRegisterInfo.cpp:98-105`,
    `EzMir/include/Function/MirFunctionRegisterInfo.h:86`
  - Why: `optional<vector>` by value, called per def inside loops (e.g.
    `LivenessAnalysisPass.cpp:194`).
  - Fix: return a pointer/`optional<reference_wrapper<const…>>`.
  - Status: fixed

- [x] **P1 · OPT-03 — Per-instruction heap vectors in hot passes**
  - Where: `EzMir/src/Instruction/MirInstruction.cpp:222-297`,
    callers `LivenessAnalysisPass.cpp:142,149`, `NonSsaToSsaPass.cpp:100`,
    `EzTriple/src/RegisterAllocator/MirRegisterAllocator.cpp:48-49,459,464,513`
  - Why: `getUsedRegisters`/`getDefinedRegisters` allocate `std::vector` with the default
    allocator and rescan flags per operand.
  - Fix: out-parameter `pmr::vector` or visitor + per-instruction flag cache.
  - Status: fixed

- [x] **P1 · OPT-04 — `std::pmr::string` keys allocated on every symbol lookup**
  - Where: `EzMir/src/Parser/MirParserContext.cpp:201,243,261,308,344,362,376,386,400,410`
  - Why: a temporary `pmr::string` is built before each `find`; source buffers outlive lookups.
  - Fix: transparent `string_view` keys.
  - Status: fixed

- [x] **P1 · OPT-05 — `getOpCodeFromStr` allocations / repeated lookups**
  - Where: `EzMir/include/Instruction/MirInstructionSet.h:49-83`, callers
    `EzMir/src/Parser/MirParser.cpp:759,861`
  - Why: takes `const std::string&`, allocates, then does up to three map finds plus
    `StrToUpper`/`StrToLower`.
  - Fix: normalize once, take `string_view`, use one lookup table.
  - Status: fixed

- [x] **P1 · OPT-06 — `CallLoweringState` bank cursors key on `std::string`**
  - Where: `EzMir/include/Function/CallLoweringState.h:114`,
    `EzMir/src/Function/CallLoweringState.cpp:97,104`
  - Why: defers PMR and builds a temporary string per lookup.
  - Fix: `pmr::string`/`string_view` keys or a fixed enum.
  - Status: fixed

- [x] **P2 · OPT-07 — Ordered `pmr::map/set` for CFG adjacency**
  - Where: `EzMir/include/MirPasses/Passes/CodeFlowAnalysisPass.h:12-13`
  - Why: blocks are dense-indexable; ordered containers add log factors and allocations.
  - Fix: vector/bitset indexed by `MirId`.
  - Status: fixed

- [x] **P2 · OPT-08 — Missing `reserve`/moves in builders and formatters**
  - Where: `EzMir/src/Type/MirType.cpp:15-16`, `EzMir/src/GlobalVar/MirGlobalVar.cpp:17`,
    `EzMir/src/Function/ArgumentLocationDesc.cpp:7,31-34`,
    `EzMir/src/Instruction/MirInstruction.cpp:345-355`
  - Fix: move sink parameters, avoid `const&`+`std::move`, reserve output strings.
  - Status: fixed

- [x] **P2 · OPT-09 — Redundant lookups / small API tightening**
  - Where: `EzMir/src/Operand/MirRegisterBank.cpp:14-20`,
    `EzMir/src/Operand/MirRegisterClass.cpp:22-38`,
    `EzMir/include/MirPasses/MirPassManager.h:33-35,52-54`,
    `EzMir/src/MirPasses/Passes/CodeFlowAnalysisPass.cpp:214-225`
  - Why: `find`+`operator[]`, `contains`+`operator[]`, double indexing.
  - Fix: `try_emplace`/single lookup.
  - Status: fixed

- [x] **P2 · OPT-10 — Static-init global metadata vectors**
  - Where: `EzMir/include/Instruction/MirInstructionSet.h:21-26`,
    `EzMir/include/Instruction/MirInstructionMetadata.h:219`
  - Why: dynamic initialization at static-init time; `getMeta` could be called early.
  - Fix: `std::span`/`std::array` over static data.
  - Status: fixed

## 6. Hot spots

- `EzMir/src/Parser/MirParser.cpp` (1384 lines) — DUP-01/02, WEI-05/06/07, OPT-05.
- `EzMir/src/Parser/MirParserContext.cpp` — DUP-03, WEI-05, OPT-04.
- `EzMir/src/Instruction/MirInstructionBuilder.cpp` and `MirInstruction.cpp` — DUP-05/07, LEG-08,
  WEI-13, OPT-01/03.
- `EzMir/src/Builder/MirBuilderContext.cpp` — WEI-02.
- `EzMir/src/MirPasses/MirPassManager.cpp` + `Passes/*` — WEI-01/03/10/11.

## 7. Verification gate

Use the standard gate in `ReviewProcess.md`. For WEI-01/03, add a regression test with two
functions whose liveness differs and a `ReadWrite` operand.

## 8. Findings tracker

| ID | Severity | Category | Status | Owner | Notes |
| --- | --- | --- | --- | --- | --- |
| WEI-01..14 | P0/P2 | Weird | fixed | — | ownership/liveness, SSA flags, source lifetime, physical regs, lookahead, deterministic hashing, error contract |
| DUP-01..08 | P1/P2 | Duplication | fixed | — | parser/builder/operand/printer/def-use consolidation |
| LEG-01..10 | P1/P2 | Legacy | fixed | — | dead ctors/APIs/tables removed; variadic operand slot encoded in metadata |
| OPT-01..10 | P1/P2 | Optimization | fixed | — | trace guards, no-copy queries, out-vectors, transparent keys, CFG vectors, static metadata |
