# Plan Part 5: Subsystems Overview, Doxygen Groups, and API Reference Audit

## 1. Objective
Author the **Subsystems Architectural Guide** (`docs/pages/subsystem_guides.md`), establish a formal Doxygen `@defgroup` module taxonomy across all EzPacker subprojects, and ensure that key public API headers feature rich, compliant Doxygen docstrings for class-level and function-level documentation.

---

## 2. Deliverables & File Locations

| File | Purpose |
|:---|:---|
| [`docs/pages/subsystem_guides.md`](file:///E:/Repos/EzPacker/docs/pages/subsystem_guides.md) | Dedicated architectural deep dive integrated via `@page subsystem_guides Subsystems Architectural Overview`. |
| Header Docstring Annotations | Enriching primary C++ headers with `@defgroup`, `@ingroup`, `@brief`, `@param`, and `@return` tags. |

---

## 3. Detailed Technical Content Outline

### A. Subsystems Architectural Guide (`docs/pages/subsystem_guides.md`)
1. **EzCore**:
   - Diagnostic subsystem: `DiagnosticCollector`, `DiagnosticBuilder`, `DiagnosticScope` (Commit / Discard / Propagate), `DiagnosticMessage`.
   - Arbitrary-precision math: `FlexInt` (LibTomMath `mp_int` backend) and `FlexFloat` (LibBF `bf_t` backend, IEEE-754 precision).
   - Source tracking: `SourceManager`, `SourceReference`, O(log N) binary-searched line table.
   - Core utility containers: `DenseBitSet`, `IntrusiveLinkedList`, `NameRegistry`, `StringUtils`.
2. **EzMir**:
   - Intermediate Representation hierarchy: `MirModule`, `MirFunction`, `MirBlock`, `MirInstruction`, `MirOperand`.
   - Multi-tier classification: `HighLevel`, `PassInternal`, `TargetLow`.
   - Dominator tree & SSA construction: Cooper-Harvey-Kennedy dominators, Cytron SSA phi insertion, variable renaming.
   - Dataflow & liveness analysis: `CodeFlowAnalysisPass`, `LivenessAnalysisPass`.
   - Textual serialization: `MirParser` and `MirPrinter`.
3. **EzDsl**:
   - The 9 DSL sub-languages: `.tdesc`, `.idf`, `.ezcc`, `.irdf`, `.lad`, `.lrd`, `.isf`, `.tyf`.
   - Lexy parser combinator architecture and zero-copy lexing.
   - Sema semantic validation and type verification.
   - 10 C++ code generators.
4. **EzCodeEmitter**:
   - Doubly-linked stream model: `CodeSection`, `DataNode`, `LabelNode`, `AlignNode`.
   - Multi-pass section finalization and binary patching.
   - Relocation descriptors: `TargetCodeRelocationType` and fixups.
   - Object format serialization: `Elf64Writer` and `CoffWriter`.
5. **EzTriple**:
   - `TargetTriple`: Normalized architecture, vendor, OS, environment, and object format parser.
   - Built-in predicate methods: `isWindows()`, `isLinux()`, `isCoff()`, `isElf()`.
6. **EzCompiler**:
   - Standalone CLI driver `ezc` and library `EzCompilerLib`.
   - Session lifecycle: `DriverContext` and `MirModuleLoader`.
   - Architecture factory registry: `TargetResolver`.
   - Multi-phase pipeline: `CompilationPipeline` (Middle-End -> Legalization -> Instruction Selection -> Lowering -> Register Allocation -> Code Emission).
7. **EzTargets**:
   - Concrete x86-64 reference architecture implementation: `X86_64TargetDesc`, `X86_64FrameLowerer`, `X86_64TargetInstructionSelector`, `X86_64RelocationResolver`, `X86_64CodeEmitter`.

---

### B. Doxygen Module Groups (`@defgroup` Taxonomy)
Define a unified module hierarchy so Doxygen displays a clean, organized "Modules / Topics" navigation tab:
```cpp
/**
 * @defgroup EzCore EzCore - Foundational Utilities & Math
 * @brief Core diagnostic reporting, arbitrary-precision arithmetic, source tracking, and intrusive containers.
 */

/**
 * @defgroup EzMir EzMir - Machine Intermediate Representation & SSA
 * @brief Multi-tier intermediate representation, SSA transformations, dataflow analysis, and textual syntax.
 */

/**
 * @defgroup EzDsl EzDsl - Declarative Architecture DSL & Generators
 * @brief Domain-specific languages, lexing, semantic analysis, and C++ code generator backends.
 */

/**
 * @defgroup EzCodeEmitter EzCodeEmitter - Binary Section & Object Emitter
 * @brief Doubly-linked node section streams, machine code serialization, relocations, and ELF/COFF writers.
 */

/**
 * @defgroup EzTriple EzTriple - Target Architecture & Environment Triples
 * @brief Normalized target triple parsing, platform identification, and ABI selection.
 */

/**
 * @defgroup EzCompiler EzCompiler - Pipeline Driver & Engine
 * @brief Compiler driver, pass scheduling, target resolver registry, and end-to-end compilation pipeline.
 */

/**
 * @defgroup EzTargets EzTargets - Architecture Implementations
 * @brief Concrete target architecture descriptors, frame lowerers, instruction selectors, and binary emitters.
 */
```

---

### C. Header Docstring Audit
Enrich public API header files with Doxygen-compliant documentation:
- Class briefs (`@brief`) and detailed descriptions.
- Method parameters (`@param[in]`, `@param[out]`) and return values (`@return`).
- Template parameters (`@tparam`) where applicable.
- Ownership and memory semantics (e.g. PMR allocator requirements).

---

## 4. Verification & Acceptance Criteria
1. The Subsystems Architectural Guide links to all primary classes and interfaces.
2. Doxygen generates a well-structured "Modules / Topics" page reflecting the 7 defined groups.
3. Class and member documentation displays formatted descriptions, parameter docs, and return types.
