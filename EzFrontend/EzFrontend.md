# EzFrontend — Comprehensive Overview

EzFrontend is the compiler front-end of the **EzPacker** project. It takes raw source text written in a custom
assembly-like language and transforms it, through a series of well-defined stages, into a validated **Mid-level
Intermediate Representation (MIR)** suitable for optimization and final code generation. The front-end is split into
four libraries, each responsible for a distinct phase of the pipeline:

1. **EzLexer** — Tokenization and parsing (source text → AST)
2. **EzMir** — Mid-level Intermediate Representation data structures and builders
3. **EzSemantics** — Semantic analysis and AST-to-MIR lowering
4. **EzFrontendCompilationUnit** — Pipeline orchestrator for a single source file

All four libraries build on top of **EzCore** (typed memory pools, error collection, source management, logging) and *
*EzLib** (common utilities).

---

## 1. EzLexer — Tokenization & Parsing

EzLexer is responsible for converting raw source text into a structured Abstract Syntax Tree (AST). It operates in two
phases.

### 1.1 Tokenization

The **BasicTokenizer** scans a character buffer and produces a flat stream of `TokenInformation` objects. Each token
carries a type tag (`_TokenType`), the original text, and a source-location reference for diagnostics.

Recognised token kinds include:

| Category         | Examples                                                                        |
|------------------|---------------------------------------------------------------------------------|
| Keywords         | `if`, `else`, `while`, `break`, `continue`                                      |
| Identifiers      | Any `[a-zA-Z_][a-zA-Z0-9_]*` sequence that is not a keyword                     |
| Numeric literals | Decimal integers, hexadecimal integers (`0x…`), floating-point numbers          |
| String literals  | Double-quoted strings with escape sequences (`\n`, `\t`, `\\`, etc.)            |
| Punctuation      | `:` `,` `.` `(` `)` `{` `}` `+` `-` `%` `;`                                     |
| Comments         | Lines starting with `#` (preserved as `Comment` tokens, skipped during parsing) |
| Newlines         | Tracked for line/column bookkeeping                                             |

### 1.2 Parsing

The token stream is consumed by a set of small, composable parsers that each know how to recognise one language
construct. Every parser implements the **`IAstNodeParser`** interface, whose single method `parse(BasicParsingContext*)`
either returns a new AST node or `nullptr` on failure.

Parsers are grouped into a **ParserBatch**. The batch tries each registered parser in order; for each attempt it saves
the stream position and error scope, rolls them back on failure, and returns the first successful match. This makes the
grammar easily extensible — adding a new construct only requires writing a new parser and registering it.

Available parsers:

| Parser                | Construct it recognises                                                                |
|-----------------------|----------------------------------------------------------------------------------------|
| `ModuleParser`        | Function/module declarations (return type, name, parameter list, body)                 |
| `LabelParser`         | Named labels followed by a brace-delimited code scope                                  |
| `InstructionParser`   | Assembly-style instructions (`mov`, `add`, `cmp`, `jmp`, …) with operands              |
| `VariableParser`      | Variable references (`%name`) with optional type prefix                                |
| `ImmediateParser`     | Numeric and string literal operands with optional type prefix                          |
| `MemoryOperandParser` | Memory references: `type (%base + offset)` or `type (%base + %index * scale + offset)` |
| `ConditionParser`     | Comparison conditions used by `if` and `while` (e.g. `%a < %b`)                        |
| `IfParser`            | `if` / `else` blocks                                                                   |
| `WhileParser`         | `while` loops                                                                          |
| `BreakParser`         | `break` statement                                                                      |
| `ContinueParser`      | `continue` statement                                                                   |
| `CodeScopeParser`     | Brace-delimited `{ … }` blocks containing a list of expressions                        |

All parsers share a **BasicParsingContext** that provides:

- A position cursor over the token stream (`peek`, `consume`, `consumeIf`).
- An **AstNodeTypedPool** — a cache-friendly arena allocator for all AST node objects.
- A **StringPool** — arena-allocated, deduplicated string storage for identifiers and literals.
- Pre-built `ParsingCondition` lambdas for matching tokens by type or content.

### 1.3 The Abstract Syntax Tree

Every parsed construct becomes an `AstNode` subclass. Nodes support the **Visitor pattern** via
`accept(AstNodeVisitor*)` for traversal by semantic passes and the lowerer.

Concrete node types:

| Node class             | Description                                                                                  |
|------------------------|----------------------------------------------------------------------------------------------|
| `Module`               | Top-level unit: owns a `ModuleHeader` and a body `CodeScope`.                                |
| `ModuleHeader`         | Return type name, module/function name, and a parameter list (slice of `AstNode`).           |
| `CodeScope`            | A brace-delimited block containing an ordered list of child expressions.                     |
| `Instruction`          | A mnemonic name plus an ordered operand list.                                                |
| `CallInstruction`      | Specialised instruction for `call` — adds a callee name and expected return type.            |
| `Label`                | A named label followed by a `CodeScope` for the label's body.                                |
| `Variable`             | A `%name` reference with an optional data-type prefix; can be a parameter, local, or global. |
| `ImmediateOperand`     | A numeric or string constant with an optional type prefix (e.g. `i32 42`, `0xFF`).           |
| `MemoryOperandAstNode` | A memory dereference: base register ± offset, with optional index, scale, and type.          |
| `ConditionAstNode`     | A binary comparison (`<`, `>`, `==`, etc.) between two operand nodes.                        |
| `IfAstNode`            | An `if` with a condition, a true-branch scope, and an optional else-branch scope.            |
| `WhileAstNode`         | A `while` with a condition and a loop-body scope.                                            |
| `BreakAstNode`         | A `break` statement (valid only inside a `while` loop).                                      |
| `ContinueAstNode`      | A `continue` statement (valid only inside a `while` loop).                                   |

Nodes can be decorated with **annotations** (`IAstNodeAnnotation` subclasses) that carry semantic metadata attached by
later passes, without modifying the node classes themselves.

---

## 2. EzMir — Mid-level Intermediate Representation

EzMir defines the compiler's intermediate representation that sits between the high-level AST and any backend
code-generation or optimization stages. It is a low-level, register-based, SSA-friendly IR organized around basic
blocks.

### 2.1 Core Data Structures

| Type               | Role                                                                                                                                                                                                                                                                    |
|--------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **MirBlock**       | A basic block — a straight-line sequence of instructions with a single entry point, terminated by a control-flow instruction (JMP, conditional jump, RET, HALT).                                                                                                        |
| **MirInstruction** | A single operation: an opcode (`MirInstructionOpCode`) plus a linked list of operands. Carries metadata (expected operand count, flags).                                                                                                                                |
| **MirOperand**     | A type-safe variant (`std::variant`) that can hold a virtual register, a 64-bit integer, an arbitrary-precision big integer reference, a double, a memory address (base + index × scale + offset), or a reference to another MIR entity (block, function, global data). |
| **MirFunction**    | A callable unit that owns an ordered list of basic blocks, a designated entry-point block, a return-type ID, a unique function ID, and a parameter operand list.                                                                                                        |
| **MirType**        | A lightweight, self-contained type descriptor for the MIR layer. Supports Integer, FloatingPoint, Pointer, Array, and Void kinds, each with a unique ID and a human-readable name. Intentionally decoupled from the semantic-layer `Type` class.                        |

### 2.2 Instruction Set

The full instruction catalogue is defined in a single **X-macro file** (`MirInstructionSet.h`). Each entry specifies a
name, an expected operand count, and a flag bitmask. Adding a new instruction requires only one line — the enum values,
metadata table, and per-opcode emitter helpers are all generated automatically.

Instruction categories:

| Category                 | Instructions                                             |
|--------------------------|----------------------------------------------------------|
| **Data Movement**        | `MOV`, `LEA`, `CLOAD`, `CSTORE`                          |
| **Memory Access**        | `LOAD`, `STORE`, `CREATE`                                |
| **Arithmetic**           | `ADD`, `SUB`, `MUL`, `IMUL`, `DIV`, `IDIV`, `REM`, `NEG` |
| **Bitwise Logic**        | `AND`, `OR`, `XOR`, `NOT`, `SHL`, `SHR`, `SAR`           |
| **Comparison**           | `CMP`, `TEST`                                            |
| **Control Flow**         | `JMP`, `JE`, `JNE`, `JG`, `JGE`, `JL`, `JLE`, `JA`, `JB` |
| **Function Call/Return** | `CALL`, `RET`                                            |
| **Type Casting**         | `TRUNC`, `ZEXT`, `SEXT`, `BITCAST`                       |
| **System/Special**       | `SYSCALL`, `NOP`, `HALT`                                 |

Every instruction is tagged with fine-grained **flags** (`MirInstructionFlags`) describing data-flow (read/write per
operand), operand constraints (must be register, memory, or immediate), type/size safety rules, memory semantics,
control-flow properties (terminator, branch, call, return), optimization barriers (side effects), and CPU-flag usage (
reads/writes flags). These flags drive register allocation, scheduling, and safety checks in downstream passes.

### 2.3 Emitters (Builder APIs)

Three builder classes make constructing well-formed MIR convenient:

- **MirEmitterContext** — The central bookkeeper. It owns the arena pools for all MIR objects (blocks, instructions,
  operands, functions, types, global data) and provides factory methods (`createBlock`, `createInstruction`,
  `createFunction`, `createType`, `createId`). It also maintains a "currently bound block" cursor so new instructions
  are automatically appended to the right place.

- **MirEmitter** — A high-level façade over `MirEmitterContext`. It provides one type-safe helper per opcode (`emitMOV`,
  `emitADD`, `emitJMP`, …) that validates operand counts against the instruction-set metadata at call time. It also
  offers `createRegister()` for allocating new virtual registers.

- **MirGlobalDataEmitter** — Creates `MirGlobalDataEntry` objects representing initialized or uninitialized blobs of
  bytes that live at fixed addresses in the final binary (the `.data` / `.rdata` / `.bss` equivalent). Convenience
  wrappers exist for 64-bit integers, doubles, and null-terminated strings. Data is copied into an internal arena, so
  callers can free source buffers immediately.

---

## 3. EzSemantics — Semantic Analysis & AST-to-MIR Lowering

EzSemantics takes the raw AST produced by EzLexer and transforms it into validated, type-checked MIR. The pipeline has
two major phases: semantic analysis (three visitor passes) and AST-to-MIR lowering (one visitor pass).

### 3.1 Supporting Infrastructure

#### Scope, Symbol, and TypeTable

- **Scope** — A lexical scope that maps names to `Symbol` objects. Scopes form a tree; name resolution walks upward
  through the parent chain, implementing familiar shadowing rules.
- **Symbol** — A named entity in the symbol table. Records the defining AST node, the symbol kind (`GlobalVariable`,
  `LocalVariable`, `Label`, `Module`), its data type, a unique numeric ID, and its name.
- **TypeTable** — A static lookup of the language's built-in primitive types: `i8`, `i16`, `i32`, `i64` (integers),
  `f32`, `f64` (floating point), `void`, and `string`. Provides a default type (`i64`) used when no explicit type
  annotation is given.
- **Type** — Describes a single primitive type with an underlying kind (Integer, FloatingPoint, String, Void), a
  bit-width (8, 16, 32, 64, 128, 256, 512 bits, or variable for strings), and a human-readable name.

#### Semantic Annotations

Annotations are the mechanism through which semantic passes decorate the AST with extra information without changing the
node classes:

| Annotation                 | Purpose                                                                                                                                                                                                                                                                      |
|----------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **SymbolAnnotation**       | Links an AST node to its defining `Symbol`.                                                                                                                                                                                                                                  |
| **ScopedSymbolAnnotation** | For nodes that both define a symbol and own a scope (Module, Label). Inherits from `SymbolAnnotation` and adds a `Scope*`.                                                                                                                                                   |
| **ScopeAnnotation**        | For nodes that own a lexical scope (e.g. `CodeScope`). Holds a `Scope *`.                                                                                                                                                                                                    |
| **DataTypeAnnotation**     | Attaches a resolved `Type` to a node (e.g. an `ImmediateOperand` whose `i16` prefix has been resolved).                                                                                                                                                                      |
| **TypeCastAnnotation**     | Replaces a `SymbolAnnotation` when a variable is used with a type different from its declaration. Records the original symbol and the target cast type. Provides helpers (`isExpansion`, `isTruncation`, `isIntegerToDouble`, etc.) to decide which MIR cast opcode to emit. |

#### BasicSemanticContext

The central façade used by all semantic visitors and the lowerer. It owns:

- The global scope and a scope stack (`beginScope` / `endScope` / `enterScope` / `exitScope`).
- Arena pools for `Symbol` and `IAstNodeAnnotation` objects.
- A loop-nesting counter so the type checker can reject `break`/`continue` outside of loops.
- Helper methods for creating symbols, resolving names, and emitting diagnostic errors tied to source locations.
- RAII guards: `ScopeGuard` (enter/exit scope) and `LoopGuard` (enter/exit loop).

#### SemanticVisitor

A thin base class that equips an `AstNodeVisitor` with a shared `BasicSemanticContext`. All semantic passes and the
lowerer inherit from it.

### 3.2 Semantic Analysis — Three Visitor Passes

The semantic analysis runs three sequential AST traversals. Each pass builds on the annotations left by the previous
one.

#### Pass 1: SymbolDefinitionVisitor

The first pass walks the AST and:

- Creates a child scope for each Module, Label, If/Else branch, and While body.
- Defines symbols for module names, parameters, labels, and local variables (via the `create` instruction).
- Annotates nodes with `SymbolAnnotation` or `ScopedSymbolAnnotation`.

No name resolution or type checking happens here.

#### Pass 2: SymbolAndTypeResolverVisitor

The second pass walks the annotated AST and:

- Resolves each Variable reference to the `Symbol` that defined it, emitting an "unknown symbol" error if no definition
  is found.
- Resolves type-name strings (on immediates, memory operands, etc.) to their `Type` objects via the `TypeTable`.
- Annotates nodes with `DataTypeAnnotation` and `SymbolAnnotation`.

#### Pass 3: TypeCheckVisitor

The third pass walks the fully-resolved AST and:

- Verifies that instruction operand types are compatible (e.g. both sides of an ADD must have the same bit-width, or an
  implicit cast must be possible).
- Replaces a `SymbolAnnotation` with a `TypeCastAnnotation` when a variable is used with a type different from its
  declared type.
- Rejects `break` and `continue` statements that appear outside of a `while` loop.
- Validates memory operand types and condition operand types.

After this pass succeeds, the AST is fully validated and ready for lowering.

### 3.3 AST-to-MIR Lowering

The **AstLowererVisitor** walks the fully-annotated AST and emits the equivalent MIR. It dispatches each node to a
specialised **GenericLowerer** subclass:

| Lowerer                | What it does                                                                                                                            |
|------------------------|-----------------------------------------------------------------------------------------------------------------------------------------|
| **ModuleLowerer**      | Creates the MIR function, its entry block, and lowers header parameters. Delegates to the body.                                         |
| **LabelLowerer**       | Creates a new basic block and links the label symbol to it.                                                                             |
| **InstructionLowerer** | Maps source mnemonics to MIR opcodes and lowers each operand.                                                                           |
| **VariableLowerer**    | Emits virtual-register operands, handling type-cast annotations by inserting TRUNC/ZEXT/SEXT/BITCAST instructions when needed.          |
| **ImmediateLowerer**   | Emits integer, floating-point, or big-integer constants. For values that exceed 64 bits, creates a global data entry and references it. |
| **MemoryLowerer**      | Emits `MirMemory` operands (base + index × scale + offset).                                                                             |
| **ConditionLowerer**   | Emits a CMP instruction plus a conditional jump to the target block.                                                                    |
| **IfLowerer**          | Creates true-branch, false-branch (if else exists), and merge blocks, and wires the condition into them.                                |
| **WhileLowerer**       | Creates condition-check, loop-body, and exit blocks. Pushes a `LoopContext` for break/continue handling.                                |
| **BreakLowerer**       | Emits a JMP to the current loop's exit block and opens a dead-code block for any unreachable code that follows.                         |
| **ContinueLowerer**    | Emits a JMP back to the current loop's condition-check block, also opening a dead-code block.                                           |
| **CodeScopeLowerer**   | Iterates through a brace-delimited scope and lowers every child expression in order.                                                    |

The lowerers share a **LoweringContext** that provides:

- Block, instruction, and operand stacks for passing MIR artefacts up and down the AST tree.
- A `LoopContext` stack (`enterLoop` / `exitLoop` / `getCurrentLoopContext`) so break/continue know their jump targets.
- A symbol-to-MIR-ID linkage map (`linkSymbolToMirId` / `getMirIdOfSymbol`) connecting semantic symbols to their lowered
  virtual registers or blocks.
- A semantic-type-to-MIR-type linkage map.
- Convenience accessors for the shared `MirEmitter`, `MirEmitterContext`, and `MirGlobalDataEmitter`.

---

## 4. EzFrontendCompiler — Pipeline Orchestrator

The **EzFrontendCompiler** library ties every preceding library together into a turn-key compilation driver. It owns two
main classes and four concrete compilation-phase classes.

### 4.1 FrontendCompilationUnit

`FrontendCompilationUnit` is the state container for a single source file as it moves through the pipeline. It inherits
from `ErrorEmitter` so that every phase can emit diagnostics through a shared `ErrorCollector`. Internally it holds:

- A **source ID** (`m_targetSourceId`) assigned by the `SourceManager` when the raw source text is registered.
- A **tokenizer** (`BasicTokenizer`) populated during the tokenization phase.
- A **parsing context** (`BasicParsingContext`) and a pointer to the **global-scope AST node slice** produced during
  parsing.
- A **semantic context** (`BasicSemanticContext`) created during semantic analysis.
- A **MIR emitter**, **MIR emitter context**, **MIR global data emitter**, and **lowering context** — all created
  during the AST-to-MIR lowering phase.

The unit is created in two steps: the constructor takes the shared `ErrorCollector` and `SourceManager`, and then
`create(sourceContent, sourceName)` registers the source text and stores the resulting source ID. A `cleanup()` method
resets every field to a blank state so the unit can be safely destroyed or reused.

All internal components are exposed through const-reference getters (for read-only access) and mutable-reference getters
(for modification by compilation phases), plus matching setters so phases can inject the objects they build.

### 4.2 FrontendCompilationUnitPhase

`FrontendCompilationUnitPhase` is a pure virtual interface (Strategy pattern) with two methods:

- `execute(FrontendCompilationUnit *unit)` — performs the phase's work on the given unit, returning `true` on success.
- `getName()` — returns a human-readable phase name for diagnostics.

Four concrete phases implement this interface:

| Phase                    | What it does                                                                                                                                                           |
|--------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **TokenizationPhase**    | Creates a `BasicTokenizer`, feeds it the unit's source buffer, and stores the result via `setTokenizer`.                                                               |
| **ParsingPhase**         | Builds a `BasicParsingContext` from the token stream, runs a `ParserBatch` (with `VariableParser` and `ModuleParser`) in a loop, and stores the global-scope AST slice.|
| **SemanticAnalysisPhase**| Runs three sequential visitor passes — `SymbolDefinitionVisitor`, `SymbolAndTypeResolverVisitor`, and `TypeCheckVisitor` — over the global-scope AST nodes.            |
| **AstLoweringPhase**     | Creates the full MIR infrastructure (`MirEmitterContext`, `MirEmitter`, `MirGlobalDataEmitter`, `LoweringContext`) and runs the `AstLowererVisitor` over the AST.      |

### 4.3 FrontendCompilerDriver

`FrontendCompilerDriver` is the top-level entry point. It also inherits from `ErrorEmitter` and maintains a list of
`FrontendCompilationUnit` objects. Its public API is minimal:

- `addSource(sourceContent, sourceName)` — creates a new `FrontendCompilationUnit`, calls `create()` on it, and
  appends it to the internal list. Returns `false` if the source name was already registered.
- `compile()` — runs all four phases in order for every registered compilation unit:
  1. **Tokenization** for all units.
  2. **Parsing** for all units.
  3. **Semantic analysis** for all units.
  4. **AST lowering** for all units.

  Each phase is executed by calling the private helper `executeCompilationUnitPhase(phase)`, which iterates over
  every unit and calls `phase->execute(&unit)`. If any unit fails, the driver emits a fatal error containing the
  phase name and the failing source name, and compilation stops immediately.

This design keeps the pipeline trivially extensible: adding a new phase (e.g. an optimization pass) only requires
writing a new `FrontendCompilationUnitPhase` subclass and inserting one line into `compile()`.
---

## 5. The Language at a Glance

The source language is a typed, assembly-like language with structured control flow. A quick example:

```
i64 CalculateChecksum(i64 %bufferPtr, i32 %length, i64 %key)
{
    create i64 %runningSum;
    create i32 %counter;
    create i64 %currentAddr;
    create i8  %byteVal;
    create i64 %tempCalc;
    create i64 %temp;

    mov %temp, 0;
    mov %runningSum, 0;
    mov %counter, 0;
    mov %currentAddr, %bufferPtr;

    if (%length EQ 0) 
    {
        ret i64 0;
    }
    
    # Silly loop that does nothing, just to demonstrate the while syntax and break statement.
    while(%temp NE %length)
    {
        break;
    }

    label_loop_start:
    {
        cmp %counter, %length;
        bge %label_loop_end;

        mov %byteVal, i8 (%currentAddr+0);
        xor %byteVal, 0xFF;
        mov %tempCalc, i64 (%byteVal+0);
        add %tempCalc, %key;
        add %runningSum, %tempCalc;
        add %currentAddr, 1;
        add %counter, 1;
        jmp %label_loop_start;
    }
    label_loop_end:
    {
        add %runningSum, i64 (%currentAddr+0x10);
        nop;
        ret %runningSum;
    }
}
```

Key language features:

- **Modules (functions)** with a return type, name, and typed parameter list.
- **Typed variables** (`%name`) with explicit declarations (`create type %name;`).
- **Assembly-style instructions** (mov, add, sub, cmp, jmp, call, ret, etc.).
- **Memory operands** with base + index × scale + offset addressing.
- **Type-prefixed operands** allowing explicit casts (e.g. `i64 (%byteVal+0)`).
- **Labels** that introduce named jump targets with their own scoped body.
- **Structured control flow**: `if`/`else`, `while` loops, `break`, `continue`.
- **Built-in type system**: `i8`, `i16`, `i32`, `i64`, `f32`, `f64`, `void`, `string`.

---

## 6. Test Coverage

Each library has a dedicated test suite using Google Test:

- **EzLexer tests** — Tokenizer correctness, and individual parser tests for every construct (conditions, if,
  immediates, instructions, labels, memory operands, modules, variables, while loops).
- **EzMir tests** — MIR emitter context, MIR emitter, and global data emitter.
- **EzSemantics tests** — Three levels:
    - **SymbolVisitor tests**: SymbolDefinitionVisitor, SymbolAndTypeResolverVisitor, and TypeCheckVisitor.
    - **AstLowererVisitor tests**: End-to-end lowering correctness.
    - **LoweringPipeline tests**: Full pipeline tests for arithmetic, bitwise, data movement, if/else, immediates,
      labels, modules, variables, while loops, and complex multi-construct programs.

---

## 7. Architecture Summary

```
Source Text
    │
    ▼
┌──────────────────┐
│   BasicTokenizer │  (EzLexer)
│   Token stream   │
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│   ParserBatch    │  (EzLexer)
│   AST            │
└────────┬─────────┘
         │
         ▼
┌──────────────────────────────┐
│  SymbolDefinitionVisitor     │  (EzSemantics — Pass 1)
│  Scopes & symbols created    │
└────────┬─────────────────────┘
         │
         ▼
┌──────────────────────────────┐
│  SymbolAndTypeResolverVisitor│  (EzSemantics — Pass 2)
│  Names & types resolved      │
└────────┬─────────────────────┘
         │
         ▼
┌──────────────────────────────┐
│  TypeCheckVisitor            │  (EzSemantics — Pass 3)
│  Types validated, casts noted│
└────────┬─────────────────────┘
         │
         ▼
┌──────────────────────────────┐
│  AstLowererVisitor           │  (EzSemantics)
│  Annotated AST → MIR         │
│  (using MirEmitter + context)│
└────────┬─────────────────────┘
         │
         ▼
┌──────────────────┐
│  MIR             │  (EzMir)
│  Functions,      │
│  Blocks,         │
│  Instructions,   │
│  Operands        │
└──────────────────┘
```

The entire pipeline is orchestrated by `FrontendCompilationUnit`, which owns instances of every component and drives
them in the correct order through its `compile()` method.

