# EzPacker: System Architecture & Compiler Blueprint

> **Version:** 0.5 (Pre-Alpha)  
> **Type:** High-Level Assembly Compiler & Obfuscator  
> **Core Architecture:** Multi-Pass, Decoupled State, Variant-Backed MIR

---

## 1. Executive Summary & Core Philosophy

EzPacker is a specialized compiler designed from the ground up to support advanced code obfuscation and mutation. To
achieve this without collapsing under its own complexity, EzPacker strictly enforces a separation of **Syntax** (the
structure of the code) from **Semantics** (the meaning of the code).

* **Stateless AST:** The Abstract Syntax Tree is purely structural. Nodes (`AstNode`) are immutable regarding semantic
  data.
* **Decoupled Semantics via Annotations:** All contextual data—such as Scope limits, Symbol IDs, Data Types, and
  Casts—is attached dynamically via an `IAnnotation` interface. This allows an infinite number of compiler passes to
  decorate the tree without polluting the core node logic.
* **Security by Design:** Operations that hinder obfuscation or memory safety (such as raw direct memory addressing) are
  explicitly caught and forbidden during the translation phases.

---

## 2. Phase I: The Frontend (Lexing and Semantic Analysis)

The frontend is responsible for ensuring the raw code is structurally valid, mathematically sound, and type-safe before
it ever reaches the intermediate representation.

### A. Core Infrastructure

* **`ErrorCollector`:** The class that will be notified _EVERY_ log that ever happened in the compilation process. A log
  can be of these types:
    * Info: Shows information that might be of use for the programmer.
    * Warning: Shows a warning, it's not something bad, but it should be actually checked.
    * Soft: A parser failed because it didn't recognize the structure of a node in the given token stream.
    * Fatal: A parser has successfully identified that the given stream of tokens matches the structure they're waiting
      for, but the content of the node is malformed. Compilation process must finish.
* **`BasicTokenizer`:** This class translates the raw input (a .ez assembly file) into a stream of tokens. Reads
  character by character categorizing it and linking it to a source reference (SourceReference), managed by
  SourceManager.
* **`BasicSemanticContext`:** Manages the global Scope Tree, owns the Symbol Table, and handles robust error reporting
  by linking semantic violations directly back to their original `SourceReference`.

### B. The Three Semantic Passes

These passes traverse the entire AST, giving it different Annotations that will be used by other modules.

1. **Symbol Definition (`SymbolDefinitionVisitor`):** Scans the AST for declarations (e.g., `%var`), builds the internal
   Scope Tree, and tags container nodes with a `ScopedSymbolAnnotation`.
2. **Resolution (`SymbolAndTypeResolverVisitor`):** Links usages to their definitions. It resolves identifiers to
   `SymbolId`s, determines the types of memory operands, and handles variable shadowing via recursive parent-scope
   lookups.
3. **Type Checking (`TypeCheckerVisitor`):** Enforces language safety. It detects data truncation, verifies bit-width
   matches across operands, and injects `TypeCastAnnotation`s where implicit casting is required.

---

## 3. Phase II: The Middle-End (High-Level MIR)

The High-Level Machine Instruction Representation (HLMIR) is the bridge between the hierarchical AST and the linear
machine code. This IR aims to be a CIS (Complex Instruction Set) that, at some point will be lowered into a RIS (Reduced
Instruction Set).

This has double benefit:

* Makes the obfuscator and other modules have a strong set of tools on which they can depend to provide
  complex functionality. Will also have the possibility of defining higher-level instructions (structure-related for
  OOP, Object-Oriented Programming) without affecting the backend at all.
* These instructions will be lowered into simpler instructions that can be also lowered into more target architectures
  without needing to change an immense amount of code.

### A. The Instruction Set (X-Macros)

EzPacker uses C-style X-Macros (`HighLevelInstructionSet.h`) to define the instruction set at compile time. This
generates Enums, string maps, and metadata tables simultaneously, preventing desynchronization.

* **Metadata:** Instructions are tagged with bit-flags (`Op1_Write`, `IsTerminator`, `IsMemory`, `IsBranch`). This
  allows future obfuscation algorithms to manipulate instructions mathematically without needing to know exactly what
  the instruction is.

### B. The Memory Model & Data Structures

* **`HighLevelMirInstruction`:** This class represents an architecture-agnostic machine instruction. Contains
  the opcode and the operands.
* **Data & Code Separation:**
    * `HighLevelMirDataEntry` Represents a global variable that will be set into the
      target's data section (if possible, it might also be put in the stack).
    * `HighLevelMirBlock`: Defines a container of instructions. This block might point to another block and might also
      have previous. This class is used to contain the flat list of instructions lowered from the AST.

---

## 4. Phase III: AST to HLMIR

The `LowererVisitor` flattens the recursive, annotated AST into a linear stream of `HighLevelMirBlock`s.

* **The "Stream" Concept:** Blocks act as buckets. When a Label is encountered, the current block implicitly falls
  through, a new block is spawned, and the stream continues forward. The state is never rewound.
* **Helper Abstractions:** Functions like `lowerScopeBody` handle the complex recursive unwrapping of nested labels and
  instructions, maintaining DRY (Don't Repeat Yourself) principles.

---

## 5. TO-DO List

### Cache-friendly structures

Ensure that classes that will be instantiated lots of times and are close to each other will be available in closer
places in the cache. This would need a new class called 'TypedPool' that allows to reserve a CHUNK of contiguous memory
that will be used to create the instances of the objects (using the operator `` new (memoryAddress) Type(Arguments)`` to
be able to create objects with their constructor called, at specific memory regions).

This class will also be used in the constant pool, to avoid having a constant value defined in multiple places. It will
also be of use to allocate BigInteger.

### Concurrency and parallelization

There are quite a few components in the Frontend that are executed sequentially, but that can be used in parallel to
achieve blazing-fast performance. Here's a break-down of the parallelization points recognized:

#### Parallel files

As the name says, this POV is oriented into parallelize the entire compilation process of a single file. Encapsulating
everything in a CompilationUnit and merging the results of each CompilationUnit at the very end of the process.

* Pros:
    * The compiler will perform quite well when there are many files in the pipeline.
    * Low difficulty of implementation. It would just need a new class "CompilationUnit" that takes the classes that
      BasicTokenizer, BasicParsingContext and BasicSemanticContext use altogether, and when the resulting code is
      produced, it's just a matter of implementing a good merge algorithm.
* Cons: Projects that have few files will be compiled slower.

#### Parallel Processes

Parallelize every compilation phase, if possible. The identified places in which this can be done without a breaking the
entire codebase:

* Parsing: After BasicTokenizer finished, and returned the stream of tokens from any input, the stream will be scanned  
  to identify parsing break-points (like '{}', '()', ';', ...) that will slice the stream into parts. Each part will be
  given to a worker thread and will start the parsing. When a parser produces a result, it will be merged into the AST
  using blocking operations.
* Semantic Analysis: When the first visitor has run (SymbolDefinitionVisitor), symbols are already defined, and it is
  the only place in which TypeCastVisitor and SymbolAndTypeResolver can be run simultaneously without breaking the
  logic.

* Pros:
    * The compiler will perform quite well when there are big files with a lot of code. And it will also improve
      multi-file compilation a little.
* Cons:
    * Challenging implementation: Needs to create a whole new set of concurrent-data-structures helpers, especially in
      the semantic phase. Can introduce bugs that MIGHT or MIGHT not appear.

### Tooling & Verification

Before advancing to graph mutation, EzPacker needs a **`MirPrinterPass`**.

This pass dumps the in-memory HLMIR into a human-readable text file (`.mir`). By viewing the virtual registers (e.g.,
`%vreg_5:i32`) and blocks, the compiler engineer can visually verify that AST lowering, constant folding, and semantic
annotations are functioning perfectly prior to backend emission.

### The Backend

The immediate focus of EzPacker is to achieve a robust, end-to-end compilation pipeline before introducing chaos (
obfuscation).

#### A. The Control Flow Graph (CFG) Builder

A two-pass system that translates the linear list of MIR blocks into a directed graph.

* **Pass 1:** Maps Label `SymbolId`s to memory pointers (`HighLevelMirBlock*`).
* **Pass 2:** Analyzes the terminator instruction of every block. It links explicit branches (Jumps/Calls) to their
  target blocks and links implicit fall-throughs to contiguous blocks, populating the `Predecessors` and `Successors`
  edges.

#### B. Lowering HLMIR to LWMIR

After obfuscating the code and ensuring everything will work as intended, lower HLMIR into a RIS (Reduced Instruction
Set) that can be compiled into nearly every known architecture.

#### C. Register Allocation & Emission

* **Allocation:** Translating the infinite pool of Virtual Registers into a finite set of physical hardware registers (
  e.g., RAX, RBX) using Linear Scan or Graph Coloring algorithms.
* **Emission:** Translating the finalized target-specific MIR into standard assembly text (`.asm`) or raw executable
  binary formats (ELF/COFF).

### The Obfuscator

Once the compiler is mathematically proven to generate correct, executable code, the Obfuscation engine will intercept
the pipeline immediately after the CFG is built.

Because the CFG is fully decoupled from the AST and memory-safe, we can apply aggressive graph mutations:

1. **Dead Code Injection:** Using instruction metadata to insert harmless but confusing mathematical operations.
2. **Opaque Predicates:** Injecting mathematically complex jumps where the outcome is statically known to the compiler
   but ambiguous to a decompiler.
3. **Control Flow Flattening (CFF):** Destroying the visual hierarchy of the code by moving all blocks to the same
   execution level and routing them through a central switch-statement dispatcher.