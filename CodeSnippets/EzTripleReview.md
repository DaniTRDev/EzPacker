# EzTriple Review

## 1. Scope & entry points

- `EzTriple/include|src/Legalizer/` (+ `Actions/`) — legality, actions, legalizer pass.
- `EzTriple/include|src/InstructionSelector/` and `Targets/X86_64/X86_64InstructionSelector.cpp`.
- `EzTriple/include|src/RegisterAllocator/`, `AbiLowerer/`, `FrameLowerer/`.
- `EzTriple/include|src/Descriptors/` (`TargetDesc`, `TargetBinaryDesc`, `TargetRelocationResolver`).
- `EzTriple/include|src/Targets/X86_64/` — target descriptor, binary descriptors, relocation.
- `EzTriple/targets/x86_64/` — `.ezcc`, `.idf`, `.lad`, `.isf`, `.reg`, `.lrd`, `.tdesc`, `.tyf`.
- `EzTriple/data/` — standalone `.ezcc` copies. Build: `EzTriple/CMakeLists.txt`,
  generated tables via `EzTriple/CMake/EzDslGen*.cmake`.

All leads are unverified until checked. See `ReviewProcess.md`.

## 2. Duplicated code

- [x] **P1 · DUP-01 — `widenScalarTo`/`narrowScalarTo` and source variants are twins**
  - Where: `EzTriple/include/Legalizer/LegalizerInfo.h:289-337` vs `:343-391`, `:482-512` vs `:517-548`
  - Why: four bodies differ only in `LegalizeActionKind::WidenScalar` vs `NarrowScalar`;
    the generated legalizer never calls them (tests only).
  - Fix: one parameterized `scalarActionFor(ActionKind, ...)`.
  - Status: fixed

- [x] **P1 · DUP-02 — AbiLowerer has four near-identical `ArgLocationType` switches**
  - Where: `EzTriple/src/AbiLowerer/MirAbiLowerer.cpp:146-278` vs `:409-528` (and
    `processReturnBlock:24-140`, `processCallReturnBlock:284-403`)
  - Why: same Register/Split/Indirect/Stack bodies with only names/diagnostics differing;
    ~250 lines of near-duplication.
  - Fix: one "assign value to ABI location" routine parameterized by direction.
  - Status: fixed

- [x] **P1 · DUP-03 — "insert first, then switch insertion point" lambda repeated 4–5×**
  - Where: `LegalizeCallAction.cpp:37-45`, `LegalizeReturnAction.cpp:45-53`,
    `LegalizeNarrowScalarAction.cpp:143-152`, `LegalizeBitcastAction.cpp:39-48`,
    `MirFunctionSignatureLegalizerPass.cpp:92-103`
  - Fix: an `EmitOrdered` helper in `LegalizeActionCommon.h`.
  - Status: fixed

- [x] **P1 · DUP-04 — Extension-opcode selection repeated three times**
  - Where: `EzTriple/src/Legalizer/Actions/LegalizeWidenScalarAction.cpp:35-41,62-68,103-109`
  - Why: identical `FPEXT / (isSigned ? SEXT : ZEXT)` block.
  - Fix: local helper.
  - Status: fixed

- [x] **P1 · DUP-05 — ELF/COFF descriptors are structurally identical**
  - Where: `X86_64ElfBinaryDesc.cpp:18-35` vs `X86_64CoffBinaryDesc.cpp:15-32`; headers
    `X86_64ElfBinaryDesc.h:27-54` vs `X86_64CoffBinaryDesc.h:25-52`
  - Why: identical `getSection`/`initialize`/endianness/alignment; differ in name/PIC/format.
  - Fix: shared `BasicBinaryDesc` base.
  - Status: fixed

- [x] **P1 · DUP-06 — `findClass` duplicated hand-written vs generated**
  - Where: `EzTriple/src/Targets/X86_64/X86_64InstructionSelector.cpp:31-49` vs produced by
    `EzDsl/CodeGenerators/src/CodeGenerators/CppInstructionSelectorGenerator.cpp:279`
  - Why: the generated selector emits a local `findClass` per method and even duplicated
    `setClass` lines.
  - Fix: expose one target-level class lookup (see OPT-02).
  - Status: fixed

- [x] **P1 · DUP-07 — Hand-written register setup duplicates `.reg` source**
  - Where: `EzTriple/src/Targets/X86_64/X86_64TargetDesc.cpp:48-123` vs
    `EzTriple/targets/x86_64/x86_64_registers.reg:7-51`
  - Why: the `.reg` file is orphaned but encodes the same 16 GPR + 16 XMM names/sub-regs.
  - Fix: reconnect the generator or delete the `.reg` (see LEG-02/XPR-03).
  - Status: fixed

- [x] **P1 · DUP-08 — `data/*.ezcc` duplicate the in-target calling conventions**
  - Where: `EzTriple/data/SysV_AMD64.ezcc`, `data/Win64.ezcc` (and orphan `data/AAPCS64.ezcc`)
  - Why: byte-identical to halves of `targets/x86_64/x86_64_calling_conv.ezcc`; none referenced
    by any build file.
  - Fix: delete the duplicates or make them the single source.
  - Status: fixed

- [x] **P2 · DUP-09 — `Predicates` re-implements helpers now generated/handwritten elsewhere**
  - Where: `EzTriple/src/Predicates/Predicates.cpp:125-139,420` vs `X86_64Lowering.cpp:14-28`
    and generated selector helpers
  - Fix: delete with the module (LEG-01).
  - Status: fixed

- [x] **P2 · DUP-10 — `insertPrologue`/`insertEpilogue` setup boilerplate**
  - Where: `EzTriple/src/Targets/X86_64/X86_64FrameLowerer.cpp:24-97` vs `:103-185`
  - Fix: shared descriptor/register setup helper.
  - Status: fixed

## 3. Legacy / un-removed code

- [x] **P1 · LEG-01 — The entire `Predicates` module is dead**
  - Where: `EzTriple/include/Predicates/Predicates.h`, `EzTriple/src/Predicates/Predicates.cpp`
  - Why: ~575 lines, no callers repo-wide; still compiled via `CMakeLists.txt:23,49`.
  - Fix: delete files + CMake entries.
  - Status: fixed

- [x] **P1 · LEG-02 — Register-file generation is orphaned and targets the removed `TableGen` namespace**
  - Where: `targets/x86_64/x86_64_registers.reg:3`,
    `EzDsl/CodeGenerators/src/CodeGenerators/CppRegisterInfoGenerator.cpp:204`,
    tests `T_CppRegisterInfoGenerator.cpp:79,117`
  - Why: the runtime namespace is `EzCodeEmitter::X86_64`; the `.reg` file is not invoked by
    any CMake target.
  - Fix: reconnect generation with the correct namespace, or delete the file/generator.
    Tracked in `CrossProjectReview.md` (XPR-03).
  - Status: fixed

- [x] **P1 · LEG-03 — Target-descriptor generation is orphaned with `TableGen` leftovers**
  - Where: `EzDsl/.../CppTargetDescGenerator.cpp:295`, `tests/.../T_CppTargetDescGenerator.cpp:86,120`
  - Why: emits `EzCodeEmitter::TableGen::` / `EzTriple::TableGen::`; no CMake invokes it, and
    the real descriptor is hand-written in `namespace EzTriple`.
  - Fix: finish or remove the generator path.
  - Status: fixed

- [x] **P1 · LEG-04 — Other orphaned target source-of-truth files**
  - Where: `targets/x86_64/x86_64_target.tdesc`, `targets/x86_64/x86_64_types.tyf`
  - Why: zero references repo-wide; the `.tdesc` still wires a `.reg` that is not run.
  - Fix: delete or wire into the build.
  - Status: fixed

- [x] **P1 · LEG-05 — Write-only fields**
  - Where: `include/Legalizer/MirFunctionSignatureLegalizerPass.h:30` (assigned, never read),
    `src/Legalizer/MirFunctionSignatureLegalizerPass.cpp:40` (`succeeded` never false),
    `include/FrameLowerer/MirFrameLowerer.h:20,26` (`m_allocator`)
  - Fix: delete or use.
  - Status: fixed

- [x] **P1 · LEG-06 — Unreferenced methods**
  - Where: `src/RegisterAllocator/MirRegisterAllocator.cpp:478` (`addEdge`),
    `src/Legalizer/MirLegalizer.cpp:131` (`legalizeInstruction`),
    `include/Legalizer/LegalizerInfo.h:105` (`queryFast`),
    `include/InstructionSelector/MirInstructionSelector.h:30-42` (get/set current function/block)
  - Fix: delete.
  - Status: fixed

- [x] **P1 · LEG-07 — Addressing-mode folding is test-only, not wired into production selection**
  - Where: `src/InstructionSelector/MirInstructionSelector.cpp:153,174` called only from
    `tests/EzTripleTestSuite/src/EzTripleTestSuite.cpp:209,222,245,258`, yet
    `X86_64TargetDesc.cpp:142` still creates `X86AddressingModeMatcher`.
  - Why: production `X86_64TargetInstructionSelector` does not fold.
  - Fix: wire folding into production or delete the matcher creation.
  - Status: fixed

- [x] **P1 · LEG-08 — `getLibcallStr` is a stale libcall id-space**
  - Where: `src/Targets/X86_64/X86_64TargetDesc.cpp:191-210` vs generated
    `x86_64LegalizerActionTable.cpp` (`__divti3`/`__udivti3`)
  - Why: the fallback in `MirLegalizer.cpp:287-308` would return the wrong symbol.
  - Fix: align the id-space with the generated table.
  - Status: fixed

- [x] **P1 · LEG-09 — `MirRegisterAllocatorPass::getDependencies` TODO returns `{}`**
  - Where: `src/RegisterAllocator/MirRegisterAllocatorPass.cpp:136-139`,
    header doc `include/RegisterAllocator/MirRegisterAllocatorPass.h:63-65`
  - Fix: declare the real dependencies or fix the doc.
  - Status: fixed

- [x] **P2 · LEG-10 — `X86_64Lowering.cpp` is a thin shim with ad-hoc predicates**
  - Where: `src/Targets/X86_64/X86_64Lowering.cpp:8,11,14-28`
  - Why: one-line wrappers plus `isPowTwo`/`log2` declared only by generated code/tests;
    `log2` returns trailing-zero count for non-powers and shadows `std::log2`.
  - Fix: declare the generator contract in a header, rename `log2`.
  - Status: fixed

- [x] **P2 · LEG-11 — `LoweredBlock` alias and `m_loweredBlocks` allocator**
  - Where: `include/AbiLowerer/MirAbiLowererPass.h:44,84`
  - Fix: remove the confusing alias; supply the allocator.
  - Status: fixed

## 4. Weird scenarios / old hacks

- [x] **P0 · WEI-01 — Null dereference before the null check in `MirAbiLowererPass`**
  - Where: `src/AbiLowerer/MirAbiLowererPass.cpp:31-32` (and unchecked operand access
    `:49,62,75,88,101,115,128`)
  - Why: every other pass checks `func` first; this one constructs the signature first.
  - Fix: guard + validate operand tags/bounds.
  - Status: fixed

- [x] **P0 · WEI-02 — `MirRegisterAllocatorPass` dereferences the target descriptor unchecked**
  - Where: `src/RegisterAllocator/MirRegisterAllocatorPass.cpp:14-15`
  - Why: null target/allocator crashes in the constructor before `run()` can validate.
  - Fix: validate before storing.
  - Status: fixed

- [x] **P1 · WEI-03 — Inconsistent calling-convention null checks**
  - Where: `src/RegisterAllocator/MirRegisterAllocator.cpp:63` (guarded) vs `:113,120,349`
    (unguarded)
  - Fix: consistent guard.
  - Status: fixed

- [x] **P1 · WEI-04 — Call detection and bank identity by string**
  - Where: `src/RegisterAllocator/MirRegisterAllocator.cpp:56-59` (`getName()=="CALL"`),
    `:299-308` (`rfind("FPR",0)`)
  - Why: target instruction spelling and class naming become semantic.
  - Fix: use descriptor flags / class identity.
  - Status: fixed

- [x] **P1 · WEI-05 — `X86_64TargetDesc::initialize()` is not idempotent**
  - Where: `src/Targets/X86_64/X86_64TargetDesc.cpp:39-44,52-70,126-127`
  - Why: a second call leaks banks/classes/conventions and leaves stale registrations.
  - Fix: guard with an initialized flag or make construction the only init point.
  - Status: fixed

- [x] **P2 · WEI-06 — Mixed ownership across the target seam**
  - Where: `src/Targets/X86_64/X86_64TargetDesc.cpp:245-262` (`new`, caller-owned) vs
    `unique_ptr` members and lazy `getRelocationResolver` (`:265-272`)
  - Fix: return `std::unique_ptr<GenericCodeEmitter>`.
  - Status: fixed

- [x] **P2 · WEI-07 — `MirAddressingModeMatcher::getDef` can scan the whole module**
  - Where: `src/InstructionSelector/MirAddressingModeMatcher.cpp:92-102,116`
  - Why: O(functions × instructions) fallback per folded operand.
  - Fix: restrict to the current function.
  - Status: fixed

- [x] **P2 · WEI-08 — `selectPHI` predecessor mapping is heuristic and name-based**
  - Where: `src/Targets/X86_64/X86_64InstructionSelector.cpp:583-641` (`"undef"` name check at `:614`)
  - Why: relies on positional/`MirId` ordering and register naming.
  - Fix: use explicit predecessor/phi metadata.
  - Status: fixed

- [x] **P2 · WEI-09 — `std::format(...).c_str()` temporaries fed to `buildPhysReg`**
  - Where: `src/AbiLowerer/MirAbiLowerer.cpp:181,210,237,248,321,353,384,444,473,496`
  - Why: safe only while `buildPhysReg` copies; forces an allocation per operand.
  - Fix: pass owned `pmr::string`/`std::string` (see OPT-07).
  - Status: fixed

- [x] **P2 · WEI-10 — Hand-rolled LE store / opcode sniffing in the relocation resolver**
  - Where: `src/Targets/X86_64/X86_64RelocationResolver.cpp:10-16,36-66`
  - Why: duplicates frame-size/offset knowledge owned by the encoder/branch relaxer.
  - Fix: share the encoder's constants/helpers; unify with `EmissionEngine` (see
    `EzCompilerReview.md` DUP-01).
  - Status: fixed

## 5. Easy optimization checks

- [x] **P1 · OPT-01 — `getAvailable*` returns `pmr::vector` by value in hot paths**
  - Where: `include/Descriptors/TargetDesc.h:91,96,101`,
    `src/Targets/X86_64/X86_64TargetDesc.cpp:213-219`
  - Why: each call allocates on the default heap; `findClass` copies the bank vector on every
    lookup.
  - Fix: return `const&`/`std::span`.
  - Status: fixed

- [x] **P1 · OPT-02 — `findClass` is a linear scan per constraint**
  - Where: `src/Targets/X86_64/X86_64InstructionSelector.cpp:31-49`
  - Why: called per operand/instruction from many selectors.
  - Fix: build a `string_view → MirRegisterClass*` map once at init.
  - Status: fixed

- [x] **P1 · OPT-03 — `calculateSpillCost` dead loop weighting and O(n²) rescans**
  - Where: `src/RegisterAllocator/MirRegisterAllocator.cpp:448-473` (`loopDepth` declared inside
    the loop at `:454`), called from `:210,216`
  - Why: weight always `pow(10,0)==1` and recomputed per block; called per candidate per
    simplification step.
  - Fix: cache per-register use/def counts; hoist/fix the weight.
  - Status: fixed

- [x] **P1 · OPT-04 — `selectPHI` scans the whole function twice per PHI**
  - Where: `src/Targets/X86_64/X86_64InstructionSelector.cpp:544-603`
  - Fix: use `MirFunctionRegisterInfo::getUses`.
  - Status: fixed

- [x] **P1 · OPT-05 — Unconditional `MirPrinter::printToString` on trace/debug paths**
  - Where: `src/FrameLowerer/MirFrameLowerer.cpp:61,125`,
    `src/FrameLowerer/MirFrameLowererPass.cpp:89-90`,
    `src/RegisterAllocator/MirRegisterAllocatorPass.cpp:130`
  - Fix: guard behind the enabled-diagnostic check.
  - Status: fixed

- [x] **P1 · OPT-06 — 27 `const_cast<MirTargetInstructionDesc*>`**
  - Where: `src/Targets/X86_64/X86_64InstructionSelector.cpp`, `X86_64FrameLowerer.cpp`
  - Why: `getTargetDesc` returns const but `MirInstructionBuilder` wants mutable (it only reads).
  - Fix: make the builder accept `const MirTargetInstructionDesc*`.
  - Status: fixed

- [x] **P2 · OPT-07 — `std::format("...").c_str()` for literals / operand names**
  - Where: `src/FrameLowerer/MirFrameLowererPass.cpp:85`,
    `src/RegisterAllocator/MirRegisterAllocatorPass.cpp:126`
  - Fix: use literals / owned strings.
  - Status: fixed

- [x] **P2 · OPT-08 — Missing `reserve` on operand vectors**
  - Where: `LegalizeCallAction.cpp:37-45`, `LegalizeLibcallAction.cpp:44-64`,
    `X86_64RegisterAllocator.cpp:75-81`
  - Fix: reserve.
  - Status: fixed

- [x] **P2 · OPT-09 — Generated legalizer copies a full static matrix**
  - Where: generated `x86_64LegalizerActionTable.cpp` constructor (build dir)
  - Why: ~2304 element copies and unused `m_ruleMatchers` population.
  - Fix: keep the static table and drop the member, or consult the map.
  - Status: fixed

- [x] **P2 · OPT-10 — Interference graph repeated hashing / mixed allocators**
  - Where: `src/RegisterAllocator/MirRegisterAllocator.cpp:73-74,91-92,283-295,490-493`
  - Fix: `try_emplace`/local refs; per-class bitmask for colors.
  - Status: fixed

- [x] **P2 · OPT-11 — Dead locals causing warnings**
  - Where: `src/RegisterAllocator/MirRegisterAllocator.cpp:419,436`,
    `src/FrameLowerer/MirFrameLowerer.cpp:101,136`
  - Fix: delete.
  - Status: fixed

- [x] **P2 · OPT-12 — Per-worklist-item `LegalityQuery` zero-fill**
  - Where: `src/Legalizer/MirLegalizer.cpp:168-229`
  - Fix: lightweight pre-check / reuse a stack query.
  - Status: fixed

## 6. Hot spots

- `src/AbiLowerer/MirAbiLowerer.cpp` — DUP-02, WEI-09.
- `src/Targets/X86_64/X86_64InstructionSelector.cpp` — DUP-06, WEI-08, OPT-02/04/06.
- `src/RegisterAllocator/MirRegisterAllocator.cpp` — WEI-03/04, OPT-03/10.
- `src/Targets/X86_64/X86_64TargetDesc.cpp` — DUP-07, LEG-08, WEI-05/06, OPT-01.
- `src/Legalizer/Actions/*` — DUP-03/04, OPT-08.
- `src/Predicates/*` — LEG-01 (delete candidate).

## 7. Verification gate

Use the standard gate in `ReviewProcess.md`. For OPT-01/02/06, capture a before/after timing
on a large MIR module and re-run `T_X86_64TargetDesc`, `T_MirInstructionSelector`,
`T_MirRegisterAllocator`, `T_MirLegalizer`.

## 8. Findings tracker

| ID | Severity | Category | Status | Owner | Notes |
| --- | --- | --- | --- | --- | --- |
| DUP-01..10 | P1/P2 | Duplication | fixed | — | legalizer/ABI/frame/descriptor/selector/runtime consolidation |
| LEG-01..11 | P1/P2 | Legacy | fixed | — | dead Predicates module, orphaned target files (XPR-13), dead fields, lowering contract |
| WEI-01..10 | P0/P2 | Weird | fixed | — | ABI/RegAlloc guards, string dispatch, idempotent init, PHI predecessor metadata, shared reloc constants |
| OPT-01..12 | P1/P2 | Optimization | fixed | — | lookups, spill cost, const descriptors, trace guards, color masks, reused query, dead locals |
