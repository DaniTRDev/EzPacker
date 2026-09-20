# EzPacker Review Process

Shared methodology for the per-project reviews. Every project file uses the same
structure so findings can be compared and triaged consistently.

## Documents

| Document | Scope |
| --- | --- |
| `ReviewProcess.md` | This file: method, severity, tracker format, verification gate. |
| `EzCoreReview.md` | `EzCore/` — diagnostics, source manager, FlexNumber, helpers. |
| `EzMirReview.md` | `EzMir/` — MIR AST, builders, parser, printer, passes. |
| `EzTripleReview.md` | `EzTriple/` — legalizer, selector, allocator, ABI, frame, targets. |
| `EzDslReview.md` | `EzDsl/` — lexer/parsers, sema, code generators, CLI driver. |
| `EzCompilerReview.md` | `EzCompiler/` — driver context, pipeline, emission engine. |
| `CrossProjectReview.md` | Duplication/legacy that spans two or more projects. |

## How to use a project file

1. Read **Scope & entry points** first.
2. Walk each category (Duplicated / Legacy / Weird / Optimizations). Every row is a
   **verifiable lead**, not a confirmed defect. Confirm or refute it, then fix or file an issue.
3. Use the **Hot spots** list to prioritize files with the highest lead density.
4. Run the **Verification gate** after any batch of fixes.
5. Update the **Findings tracker**: set `status` and `owner` as items move.

## Severity legend

| Level | Meaning |
| --- | --- |
| **P0** | Correctness, memory-safety, lifetime, or data-race risk. Fix before further feature work. |
| **P1** | Duplication, dead code, or a measurable/perceived performance smell. Cheap cleanup. |
| **P2** | Style, naming, docs, and consistency polish. Batch opportunistically. |

Status values: `new`, `confirmed`, `refuted`, `in-progress`, `fixed`, `deferred`, `wontfix`.

## Category definitions

- **Duplicated code** — two or more near-identical implementations that should share one
  helper, base class, or data source. Includes generated-vs-handwritten overlap.
- **Legacy / un-removed code** — APIs, fields, files, namespaces, comments, or options that
  are no longer reachable/consumed, or that describe a design that has since changed.
  Prefer deletion; if kept, document why.
- **Weird scenarios / old hacks** — null-before-check, raw ownership, mixed error channels,
  sentinel collisions, static-init order, string-based semantic dispatch, non-idempotent
  setup, and other "this works by accident" paths.
- **Easy optimizations** — low-risk changes: avoid copies, hoist repeated lookups, add
  `reserve`, fix `const`/`noexcept`/`[[nodiscard]]`, remove allocations from hot loops,
  guard formatting behind an enabled-diagnostic check.

## Lead format

Each lead is a checkbox line so reviewers can tick it off:

```
- [ ] **P1 · DUP-01 — short title**
  - Where: `Path/File.cpp:12-40` vs `Path/Other.cpp:88-120`
  - Why: one-line rationale.
  - Fix: proposed consolidation.
  - Status: new   (owner: —)
```

## Findings tracker format

Every project file ends with a table:

```
| ID | Severity | Category | Status | Owner | Notes |
| --- | --- | --- | --- | --- | --- |
```

## Verification gate

Run from the repository root (`/home/osikasuke/Repos/EzPacker`):

```bash
# 1. Configure + full build (all libs, tools, tests)
cmake -S . -B build
cmake --build build -j"$(nproc)"

# 2. Per-suite tests
cd build && ctest --output-on-failure -j"$(nproc)"

# 3. Aggregate test binary
./bin/TEST_ALL
```

Byte-stability checks for the compiler/emitter path:

```bash
# Generated x86-64 encoding table must be byte-identical to the committed runtime expectations.
./build/bin/EzDslCli -i EzTriple/targets/x86_64/x86_64_instructions.idf \
    -o /tmp/ezgen --emit-target-encodings --target x86_64
diff <(tail -n +12 /tmp/ezgen/x86_64EncodingTable.h) \
     <(tail -n +12 build/EzTriple/generated/x86_64/x86_64EncodingTable.h)

# End-to-end object output hash (compile a sample, compare across runs/revisions).
./build/bin/ezc ... && sha256sum <output.o>
```

A change is acceptable only if the gate is green (or the delta is explained) and, for the
emitter path, the generated output remains byte-identical unless the change is intentional.

## Implementation progress

Branch `InstructionSelUpgrade`. Each project file marks implemented leads as `fixed` (or
`in-progress` for partial work) and records the remaining ones in its findings tracker.

| Tier | Findings | Status |
| --- | --- | --- |
| P0 | EzCore WEI-01/02/03; EzMir WEI-01/02; EzTriple WEI-01/02; EzCompiler DUP-01, WEI-01; XPR-08 | fixed |
| P1 | EzCore LEG-01..06, WEI-04..08, DUP-06 | fixed |
| P1 | EzCore OPT-01, OPT-04 | in-progress |
| P1 | EzMir WEI-03 | fixed |

Notes:

- New regression tests: `T_DenseBitSet` (WEI-02), `T_SourceManager` (WEI-05/07), a two-function
  liveness case (EzMir WEI-01), and an object-writer alignment case (EzCompiler WEI-01).
- The branch-patching refactor (DUP-01/XPR-08) was verified to keep the sample object output
  byte-identical before and after.
- Verified with `cmake --build` and `ctest` on 63 tests. Note: the EzDsl code-generation suites
  share output paths and can race under `ctest -j`; run them serially (`-j1`) for a clean result.
- Remaining P1/P2 findings across EzMir, EzTriple, EzDsl, EzCompiler and CrossProject are tracked
  in their respective project files.
