# EzCore Review

## 1. Scope & entry points

- `EzCore/include/Diagnostics/` — `DiagnosticCollector`, `DiagnosticBuilder`, `DiagnosticLogger`,
  `DiagnosticMessage`, `DiagnosticScope`.
- `EzCore/include/SourceManager/` — `SourceManager`, `GenericSourceManager`.
- `EzCore/include/FlexNumber/` — `FlexInt`, `FlexFloat` (libtommath + libbf).
- `EzCore/include/HelperClasses/` — `DenseBitSet`, `IntrusiveLinkedList`.
- `EzCore/include/StringUtils.h`, `EzCore/include/EzCoreCommon.h`.
- Build: `EzCore/CMakeLists.txt`. Tests: `tests/EzCore/`.

All leads are unverified until checked. See `ReviewProcess.md` for legend and format.

## 2. Duplicated code

- [x] **P1 · DUP-01 — `FlexInt` integer constructors are four copies**
  - Where: `EzCore/src/FlexNumber/FlexInt.cpp:36-91`
  - Why: `(uint32_t)/(uint64_t)/(int32_t)/(int64_t)` differ only in `m_isSigned` and the `mp_set_*` call.
  - Fix: one private templated/delegating helper.
  - Status: fixed

- [x] **P1 · DUP-02 — `FlexInt` comparison guard repeated four times**
  - Where: `EzCore/src/FlexNumber/FlexInt.cpp:257-294`
  - Why: identical bit-width/signedness-mismatch throw block in each comparison operator.
  - Fix: `checkCompatible(const FlexInt&) const` helper.
  - Status: fixed

- [x] **P1 · DUP-03 — `FlexInt` arithmetic operators are the same shape five times**
  - Where: `EzCore/src/FlexNumber/FlexInt.cpp:424-539`
  - Why: `+ - * / %` all do "copy this, in-place op, return".
  - Fix: shared binary-op template / free function.
  - Status: fixed

- [x] **P1 · DUP-04 — `getHighHalf`/`getLowHalf` duplicate normalization**
  - Where: `EzCore/src/FlexNumber/FlexInt.cpp:335-419`, `EzCore/src/FlexNumber/FlexFloat.cpp:429-469`
  - Why: same odd-width guard and negative normalization re-derived in four functions.
  - Fix: shared `normalizeNegative()` / width helper.
  - Status: fixed

- [x] **P1 · DUP-05 — `FlexFloat` NaN guard repeated six times**
  - Where: `EzCore/src/FlexNumber/FlexFloat.cpp:236-291`
  - Why: each comparison re-checks NaN explicitly.
  - Fix: shared comparison helper.
  - Status: fixed

- [x] **P1 · DUP-06 — `DiagnosticCollector` error/trace overloads are four copies**
  - Where: `EzCore/include/Diagnostics/DiagnosticCollector.h:36-49,55,60-71,77` and
    `EzCore/src/Diagnostics/DiagnosticCollector.cpp:32-44,49-59`
  - Why: templated + non-templated `error`/`trace` all do "builder, if enabled append, return".
  - Fix: one private `makeBuilderAndAppend(type, sender, message)`.
  - Status: fixed

- [x] **P2 · DUP-07 — `SourceManager` entry creation duplicated**
  - Where: `EzCore/src/SourceManager/SourceManager.cpp:73-87` vs `:264-288`
  - Why: `addSourceContent` and `loadFile` repeat allocate/placement-new/populate/map/push.
  - Fix: shared private `createEntry(...)`.
  - Status: fixed

- [x] **P2 · DUP-08 — `DiagnosticLogger` padding built char-by-char**
  - Where: `EzCore/src/Diagnostics/DiagnosticLogger.cpp:128-133`
  - Why: manual loop duplicates what `std::string(count, ' ')` does, with a tab-fixup pass.
  - Fix: construct once, then adjust for tabs.
  - Status: fixed

## 3. Legacy / un-removed code

- [x] **P1 · LEG-01 — `DiagnosticScope::m_hasFatalErrors` is write-only**
  - Where: `EzCore/include/Diagnostics/DiagnosticScope.h:46,54`,
    `EzCore/src/Diagnostics/DiagnosticCollector.cpp:183-186`
  - Why: set but never read anywhere in the repo; consumers use `ErrorCollector` instead.
  - Fix: delete field and assignment.
  - Status: fixed

- [x] **P1 · LEG-02 — Unused public `SourceManager` API**
  - Where: `EzCore/include/SourceManager/SourceManager.h:80,85` (+ impls `:294-301,306`)
  - Why: `getSourceBuffer` and `getIncludePaths` have zero call sites repo-wide.
  - Fix: delete or document as external API.
  - Status: fixed

- [x] **P1 · LEG-03 — Unused `DiagnosticCollector::setScopeAction`**
  - Where: `EzCore/include/Diagnostics/DiagnosticCollector.h:102`,
    `EzCore/src/Diagnostics/DiagnosticCollector.cpp:141-148`
  - Why: no call sites.
  - Fix: delete.
  - Status: fixed

- [x] **P1 · LEG-04 — Unused 6-arg `DiagnosticMessage` constructor**
  - Where: `EzCore/include/Diagnostics/DiagnosticMessage.h:95-100`,
    `EzCore/src/Diagnostics/DiagnosticMessage.cpp:16-27`
  - Why: the builder only uses the 1-arg (allocator) constructor.
  - Fix: delete.
  - Status: fixed

- [x] **P1 · LEG-05 — Dead `FlexInt`/`FlexFloat` members**
  - Where: `FlexInt.cpp:189,222,227,237,544-552,568-576,644`,
    `FlexFloat.cpp:184,216,492`
  - Why: `fitsIn`, `hasError`, `isEven`, `isOdd`, `getI8..getU32`, `dump` have zero callers;
    `m_lastErr` is only ever assigned, never observed.
  - Fix: delete the dead surface (or wire the error path intentionally).
  - Status: fixed

- [x] **P2 · LEG-06 — `EzCore` CMake omits a real header**
  - Where: `EzCore/CMakeLists.txt`
  - Why: `include/SourceManager/GenericSourceManager.h` is not listed while every other header is.
  - Fix: add it (or drop the explicit header list).
  - Status: fixed

- [x] **P2 · LEG-07 — Stale `<EzCore.h>` reference outside the build**
  - Where: `EzFrontend/EzLexer/include/EzLexerCommon.h:21`
  - Why: no `EzCore.h` exists in the repo and `EzFrontend` is not in the top-level
    `add_subdirectory` list; also calls a 4-arg `createReference` no longer exposed.
  - Fix: decide to delete `EzFrontend` or bring it back into the build with a fixed API.
    Tracked in `CrossProjectReview.md` (XPR-01).
  - Status: fixed

## 4. Weird scenarios / old hacks

- [x] **P0 · WEI-01 — `SourceManager` is implicitly copyable while owning raw arena memory**
  - Where: `EzCore/include/SourceManager/SourceManager.h:14-113`,
    `EzCore/src/SourceManager/SourceManager.cpp:41-52`
  - Why: user-declared destructor suppresses move but not copy; a copy double-frees
    `SourceFileEntry` and leaves map keys pointing into the original arena.
  - Fix: `SourceManager(const SourceManager&) = delete;` + deleted assignment.
  - Status: fixed

- [x] **P0 · WEI-02 — `DenseBitSet::computeLiveIn` can index out of bounds**
  - Where: `EzCore/src/HelperClasses/DenseBitSet.cpp:52-64`
  - Why: loop bounded by `m_words.size()` then indexes `use`/`liveOut`/`def` unconditionally;
    sibling `unionWith` (`:33-46`) correctly uses `std::min`.
  - Fix: bound by the min of all word counts and/or assert equal widths.
  - Status: fixed

- [x] **P0 · WEI-03 — Throwing `~DiagnosticBuilder` and null-deref after flush/move**
  - Where: `EzCore/src/Diagnostics/DiagnosticBuilder.cpp:37,98-101`,
    `EzCore/include/Diagnostics/DiagnosticBuilder.h:44,59`
  - Why: destructor calls `flush()` → collector, which can throw during unwinding;
    `isDiagEnabledForType()` dereferences `m_collector` after it has been nulled by flush/move.
  - Fix: make flush non-throwing / guard null; null-check before appending notes.
  - Status: fixed

- [x] **P1 · WEI-04 — `FlexFloat` widths below 32 bits are not clamped/rounded**
  - Where: `EzCore/src/FlexNumber/FlexFloat.cpp:653-682,407-423`
  - Why: `clampToFloatBounds()` handles only 32/64; `getPrecBits()` returns 24 for all `<= 32`,
    so a 16-bit half keeps 24-bit precision and `dump()` emits wrong binary16 payloads.
  - Fix: implement per-width precision/exponent handling or reject unsupported widths.
  - Status: fixed

- [x] **P1 · WEI-05 — `populateLineRanges` always appends a phantom final line**
  - Where: `EzCore/src/SourceManager/SourceManager.cpp:20-24`
  - Why: condition `lineStart <= content.size()` is always true despite the comment about
    trailing newlines, shifting EOF line lookups.
  - Fix: only push the final range when the last char was not `\n`.
  - Status: fixed

- [x] **P1 · WEI-06 — `m_enabledDiags` touched without the mutex**
  - Where: `EzCore/include/Diagnostics/DiagnosticCollector.h:116,122`,
    `EzCore/src/Diagnostics/DiagnosticCollector.cpp:18,85`
  - Why: class advertises thread-safety; `isDiagEnabledForType`/`enableDiag` are unlocked and
    are called on the builder-creation path before any lock.
  - Fix: lock, or document/remove the thread-safety contract.
  - Status: fixed

- [x] **P1 · WEI-07 — `loadFile` trusts `tellg()` (SIZE_MAX allocation on failure)**
  - Where: `EzCore/src/SourceManager/SourceManager.cpp:260-262`
  - Why: failed seek returns `-1` → cast to a huge size → enormous allocation.
  - Fix: check `std::streampos`/`tellg()` result before casting.
  - Status: fixed

- [x] **P1 · WEI-08 — Exception types do not match their meaning/docs**
  - Where: `EzCore/src/FlexNumber/FlexInt.cpp:598-601`,
    `EzCore/src/FlexNumber/FlexFloat.cpp:475-478,367-372`,
    `EzCore/include/FlexNumber/FlexFloat.h:186-188`
  - Why: narrowing throws `std::bad_alloc`; float narrowing throws `std::runtime_error` while
    the header documents `std::bad_alloc`; divide-by-zero throws `std::runtime_error`.
  - Fix: define a single error policy and align docs.
  - Status: fixed

- [x] **P2 · WEI-09 — Inconsistent null-listener checks**
  - Where: `EzCore/src/Diagnostics/DiagnosticCollector.cpp:113` vs `:170-173`
  - Why: nested-scope path guards `if (listener)`, root path dereferences directly;
    `addListener` accepts null.
  - Fix: reject null at registration and guard uniformly.
  - Status: fixed

- [x] **P2 · WEI-10 — `DiagnosticCollector` messages grow without bound**
  - Where: `EzCore/include/Diagnostics/DiagnosticCollector.h:119`,
    `EzCore/src/Diagnostics/DiagnosticCollector.cpp:112,176`
  - Why: committed messages are appended forever; no `removeListener` either.
  - Fix: clear after scope, or document a bounded lifetime.
  - Status: fixed

- [x] **P2 · WEI-11 — `getReferenceLine` returns a mutable interior pointer from `const`**
  - Where: `EzCore/src/SourceManager/SourceManager.cpp:137-167`,
    `EzCore/include/SourceManager/SourceManager.h:54`
  - Why: `const` method returns `SourceLineRange*`; growing `m_lines` can invalidate it.
  - Fix: return by value/index or document lifetime.
  - Status: fixed

- [x] **P2 · WEI-12 — LibBF header wrapped in a namespace under a fragile guard**
  - Where: `EzCore/include/EzCoreCommon.h:18-28`, `EzCore/src/FlexNumber/FlexFloat.cpp:8`
  - Why: correctness depends on which include path sets libbf's guard first.
  - Fix: dedicated wrapper header / build-level namespace option.
  - Status: fixed

## 5. Easy optimization checks

- [x] **P1 · OPT-01 — `string_view` passed by `const&`**
  - Where: `EzCore/include/Diagnostics/DiagnosticBuilder.h:25,71,76,81,86`,
    `DiagnosticMessage.h:54,64`, `DiagnosticCollector.h:30,37,55,61,77`, `StringUtils.h:10,23`
  - Why: `string_view` is trivially copyable.
  - Fix: pass by value.
  - Status: fixed-progress

- [x] **P1 · OPT-02 — Excessive message copies in the diagnostics path**
  - Where: `EzCore/src/Diagnostics/DiagnosticCollector.cpp:112,127,176,180`
  - Why: `push_back` copies messages/lists that are about to be destroyed; missing
    `DiagnosticScope` rvalue overload.
  - Fix: add `DiagnosticMessage&&` overloads and use move iterators.
  - Status: fixed

- [x] **P1 · OPT-03 — `StringUtils` `tolower`/`toupper` UB on signed `char`**
  - Where: `EzCore/include/StringUtils.h:15,28`
  - Why: passing a negative `char` is UB; duplicated EzDsl copies do the cast correctly.
  - Fix: `static_cast<unsigned char>` inside the core helper, then adopt it repo-wide
    (see `CrossProjectReview.md` XPR-02).
  - Status: fixed

- [x] **P2 · OPT-04 — Redundant / missing includes**
  - Where: `EzCore/include/EzCoreCommon.h:4,12` (`<memory>` twice, unused `<stack>`/`<functional>`);
    `StringUtils.h` (`<algorithm>`, `<cctype>`); `FlexFloat.cpp` uses `std::realloc/free` but
    includes `<cstring>` instead of `<cstdlib>`.
  - Fix: trim/add.
  - Status: fixed-progress

- [x] **P2 · OPT-05 — Unnecessary deep copies in FlexNumber**
  - Where: `EzCore/src/FlexNumber/FlexFloat.cpp:206` (copies whole `bf_t`),
    `FlexInt.cpp:315-320` (cross-width `==` heap-copies both operands).
  - Fix: use local temporaries / compare without materializing.
  - Status: fixed

- [x] **P2 · OPT-06 — `IntrusiveLinkedList::splice` range overload recounts**
  - Where: `EzCore/include/HelperClasses/IntrusiveLinkedList.h:514-524`
  - Why: walks the range although the count-aware overload exists (`:441`).
  - Fix: use the cached `m_size`/count overload.
  - Status: fixed

- [x] **P2 · OPT-07 — Missing `const`/`noexcept` wins**
  - Where: `FlexInt` binary operators (`FlexInt.cpp:424-539`) are non-`const`;
    `fitsIn` (`:189`) is non-`const`; comparisons can be `noexcept`.
  - Fix: add qualifiers.
  - Status: fixed

## 6. Hot spots

- `EzCore/src/FlexNumber/FlexInt.cpp` (881 lines) and `FlexFloat.cpp` (682 lines) — largest files,
  most duplication.
- `EzCore/include/HelperClasses/IntrusiveLinkedList.h` — const-correctness and splice issues.
- `EzCore/src/SourceManager/SourceManager.cpp` — ownership, lifetime, allocation.
- `EzCore/src/Diagnostics/DiagnosticCollector.cpp` + `DiagnosticBuilder.cpp` — error-channel,
  copies, thread-safety.
- Tests cover only `T_DiagnosticLogger` and `T_IntrusiveLinkedList`; FlexNumber / DenseBitSet /
  SourceManager have **no unit tests** — several P0 findings would have been caught.

## 7. Verification gate

Use the standard gate in `ReviewProcess.md`. Add smoke coverage for FlexNumber conversions and
`DenseBitSet` when fixing WEI-02/WEI-04.

## 8. Findings tracker

| ID | Severity | Category | Status | Owner | Notes |
| --- | --- | --- | --- | --- | --- |
| WEI-01..03 | P0 | Weird | fixed | — | `SourceManager` ownership, `DenseBitSet` bounds, builder lifetime |
| WEI-04..08 | P1 | Weird | fixed | — | FlexFloat widths, phantom line, atomic diags, `tellg()`, exception policy |
| WEI-09..12 | P2 | Weird | fixed | — | null listeners, bounded message lifetime, const line pointer, libbf wrapper |
| DUP-01..06 | P1 | Duplication | fixed | — | FlexInt/FlexFloat consolidation and diagnostic funnel |
| DUP-07..08 | P2 | Duplication | fixed | — | `SourceManager` entry creation, logger padding |
| LEG-01..07 | P1/P2 | Legacy | fixed | — | dead fields/APIs; stale `EzFrontend` removed via XPR-12 |
| OPT-01..03 | P1 | Optimization | fixed | — | `string_view` by value, move overloads, safe case conversion |
| OPT-04..07 | P2 | Optimization | fixed | — | includes, deep copies, splice recount, const/noexcept |

Notes: OPT-05/OPT-07 are partially refuted — the stale whole-`bf_t` copy no longer exists, and `FlexInt`
comparisons intentionally keep throwing on width/signedness mismatch (so they are not `noexcept`).
New `T_FlexNumber` regression suite added for the changed numerics.
