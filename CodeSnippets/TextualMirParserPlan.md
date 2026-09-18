# Master Architecture & Implementation Plan: Textual MIR Parser & Deserializer (`.mir`)
**Industrial-Grade Intermediate Representation Parser, Lexer, Round-Trip Serializer & Driver Integration**

---

## 1. Executive Summary & Vision

### 1.1 Context & Motivation
The **EzPacker** compiler suite utilizes [`EzMir`](file:///E:/Repos/EzPacker/EzMir) as its core typed Static Single Assignment (SSA) intermediate representation. While `EzMir` already possesses in-memory data structures (`MirModule`, `MirFunction`, `MirBlock`, `MirInstruction`, `MirOperand`) and a diagnostic printer ([`MirPrinter`](file:///E:/Repos/EzPacker/EzMir/include/Printer/MirPrinter.h)), it currently lacks a **Textual MIR Parser**.

With the delivery of the unified compiler driver [`ezc`](file:///E:/Repos/EzPacker/EzCompiler), the driver accepts `.mir` files from the command line, but must synthesize MIR in memory because there is no mechanism to deserialize `.mir` source text from disk.

Furthermore, as `EzFrontend` is undergoing complete redesign, a standalone Textual MIR Parser provides critical strategic value:
1. **Decoupled Backend Development**: Allows authoring, optimizing, and verifying backend passes (Legalization, ABI Lowering, ISel, RegAlloc, Frame Lowering, Code Emission) without waiting for frontend syntax stabilization.
2. **FileCheck / LLVM-Style IR Testing**: Enables concise, human-readable IR regression test cases that run through the compiler pipeline.
3. **Reproducible Bug Reports**: Compiler crashes or codegen bugs can be reduced to self-contained `.mir` test cases.
4. **Complete Driver Toolchain**: Fulfills `ezc input.mir -o output.o` as a first-class compilation workflow.

```
┌────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                Textual MIR Parsing & Compilation Pipeline                              │
└────────────────────────────────────────────────────────────────────────────────────────────────────────┘

  [ Textual .mir Source File ]
               │
               ▼
  ┌───────────────────────────┐
  │   SourceManager / Lexer   │  Memory mapping, UTF-8 buffer ingestion, tokenization (lexy)
  └────────────┬──────────────┘
               │
               ▼
  ┌───────────────────────────┐
  │         MirParser         │  Module parsing, type resolution, global variables,
  │    (Two-Pass Resolver)    │  function signatures, basic blocks, forward label patching
  └────────────┬──────────────┘
               │
               ▼
  ┌───────────────────────────┐
  │     MirBuilderContext     │  Fully materialized in-memory MirModule, MirFunction list,
  │     (In-Memory MIR)       │  PMR arena allocations, SSA graph, def-use chains
  └────────────┬──────────────┘
               │
               ├──────────────────────────────────────────────┐
               ▼                                              ▼
  ┌───────────────────────────┐                 ┌───────────────────────────┐
  │   CompilationPipeline     │                 │   MirPrinter (Canonical)  │
  │   - Middle-End (CFG, SSA) │                 │   - Round-Trip Idempotence│
  │   - Legalizer & ABI       │                 │   - AST Structure Match   │
  │   - ISel & RegAlloc       │                 └───────────────────────────┘
  │   - Frame & Emission      │
  └────────────┬──────────────┘
               │
               ▼
  [ Native Object File (.o / .obj) ]
```

---

## 2. Core Architectural Invariants

The Textual MIR parser and deserializer must strictly enforce the following architectural invariants:

* **[INV-TMP-01] PMR Arena & Memory Resource Compliance**:
  * Parsing an entire `.mir` file must allocate all AST nodes, symbols, instructions, blocks, and operands exclusively through `MirBuilderContext` and `DriverContext` memory resources (`globalArena`, `functionArena`, `sessionArena`).
  * No ad-hoc heap allocations (`malloc` / unmanaged `new`) inside the parser token stream or semantic actions.
  * Memory overhead must scale strictly linearly ($O(N)$) with the source file size.

* **[INV-TMP-02] Two-Pass Forward Reference Resolution**:
  * Basic block labels (`label %exit`), forward function calls (`CALL @helper`), and global references (`@data`) can be referenced before their textual definition.
  * The parser must maintain an unresolved forward reference table during block/function parsing and patch all operand references prior to validating the function CFG.
  * Any dangling forward references at the end of the module must produce byte-exact source diagnostics and reject the module.

* **[INV-TMP-03] Strict Type Soundness & Opcode Verification**:
  * Operands must strictly conform to opcode metadata defined in [`instructions.irdf`](file:///E:/Repos/EzPacker/EzMir/instructions.irdf).
  * Immediate types must match destination register types (e.g., `MOV i64 %v0, 42` creates an `i64` immediate; mismatched widths like assigning an `i32` immediate directly to an `i64` register without `ZEXT`/`SEXT` trigger diagnostics).
  * Malformed register classes or mismatched operand directions (`OUT` vs `IN`) must be caught during parsing.

* **[INV-TMP-04] Idempotent Round-Trip Equivalence**:
  * Formatting an in-memory module via `MirPrinter::printParseable()` and then parsing it via `MirParser::parseModule()` must produce an identical in-memory MIR structural representation:
    $$\text{Parse}(\text{Print}(\text{Module})) \equiv \text{Module}$$

* **[INV-TMP-05] Zero C++ Exception Leakage**:
  * Syntax errors, unresolved symbols, and type violations must be routed cleanly to `DiagnosticCollector` with line/column coordinates.
  * The parser returns `bool` or `std::optional` without throwing unhandled exceptions across module boundaries.

---

## 3. Textual MIR Language Specification (EBNF & Syntax)

### 3.1 Lexical Elements

```ebnf
Whitespace      ::= [ \t\r\n]+
Comment         ::= ';' [^\n]* '\n' | '//' [^\n]* '\n'
Identifier      ::= [a-zA-Z_][a-zA-Z0-9_.]*
GlobalName      ::= '@' Identifier
LocalName       ::= '%' Identifier | '%' [0-9]+
IntegerLiteral  ::= [+-]? [0-9]+ | '0x' [0-9a-fA-F]+
FloatLiteral    ::= [+-]? [0-9]+ '.' [0-9]* ([eE] [+-]? [0-9]+)?
StringLiteral   ::= '"' ([^"\\] | '\\' .)* '"'
```

### 3.2 Types

```ebnf
Type            ::= PrimitiveType | PointerType | ArrayType | TokenType | VoidType
PrimitiveType   ::= 'i1' | 'i8' | 'i16' | 'i32' | 'i64' | 'i128' | 'i256'
                  | 'f32' | 'f64' | 'f128'
PointerType     ::= 'ptr' ('<' Type '>')?
ArrayType       ::= '[' IntegerLiteral 'x' Type ']'
TokenType       ::= 'token' | '__bindToken'
VoidType        ::= 'void'
```

### 3.3 Module Structure & Globals

```ebnf
MirModule       ::= TopLevelDecl*
TopLevelDecl    ::= GlobalVarDecl | FunctionDecl | FunctionDef

Linkage         ::= 'external' | 'internal' | 'weak'
GlobalVarDecl   ::= GlobalName '=' Linkage ('const' | 'var') Type ('=' ConstantInit)? ';'
ConstantInit    ::= IntegerLiteral | FloatLiteral | StringLiteral | '[' ConstantInit (',' ConstantInit)* ']'
```

### 3.4 Functions & Basic Blocks

```ebnf
FunctionDecl    ::= 'declare' GlobalName '(' ParamTypeList? ')' '->' Type ';'
FunctionDef     ::= 'fn' GlobalName '(' ParamList? ')' '->' Type AttrList? '{' BasicBlock+ '}'

Param           ::= Type LocalName
ParamList       ::= Param (',' Param)*
ParamTypeList   ::= Type (',' Type)*
AttrList        ::= '[' Attr (',' Attr)* ']'
Attr            ::= Identifier ('=' (StringLiteral | IntegerLiteral))?

BasicBlock      ::= BlockLabel ':' Instruction*
BlockLabel      ::= LocalName | Identifier
```

### 3.5 Operands

```ebnf
Operand         ::= TypedRegister | TypedImmediate | LabelRef | MemoryRef | GlobalRef | StackRef

TypedRegister   ::= (Type)? LocalName ('(' ClassBinding ')')?
ClassBinding    ::= Identifier (':' Identifier)?   (* e.g. %p0(rax:GPR64) or %v0(unassigned) *)

TypedImmediate  ::= (Type)? (IntegerLiteral | FloatLiteral)
LabelRef        ::= 'label' LocalName
GlobalRef       ::= (Type)? GlobalName ('+' IntegerLiteral)?
StackRef        ::= (Type)? '%stack' '[' IntegerLiteral ']'

MemoryRef       ::= (Type)? '[' MemExpr ']'
MemExpr         ::= BaseReg ('+' IndexReg ('*' Scale)?)? (('+' | '-') Disp)?
BaseReg         ::= ('ptr')? LocalName
IndexReg        ::= LocalName
Scale           ::= '1' | '2' | '4' | '8'
Disp            ::= IntegerLiteral
```

### 3.6 Instructions

Instructions follow a uniform structure:
- High-level MIR: Opcode followed by comma-separated operands (destination first for definitions, or standard destination assignment `%dst = OPCODE %src...`).
- Both formats are supported for maximum ergonomic readability:
  1. Prefix format (MirInstruction internal representation):
     ```text
     ADD i64 %v2, %v0, %v1
     MOV i64 %v0, 42
     STORE [ptr %v0 + 8], i64 %v1
     ```
  2. Assignment format (SSA conventional notation):
     ```text
     %v2 = ADD i64 %v0, %v1
     %v0 = MOV i64 42
     %v1 = LOAD i64 [ptr %v0 + 8]
     ```

```ebnf
Instruction     ::= (LocalName '=')? Opcode OperandList? ';'
OperandList     ::= Operand (',' Operand)*
Opcode          ::= Identifier (* Validated against MirInstructionOpCode table *)
```

### 3.7 Example Canonical `.mir` File

```mir
// Target module metadata
target = "x86_64-unknown-linux-gnu";

// Global data definitions
@msg = internal const [14 x i8] "Hello, World!\0A\00";
@counter = external var i64 = 0;

// External function declaration
declare @printf(ptr, ...) -> i32;

// Main function definition
fn @main() -> i64 [calling_conv="sysv"] {
entry:
    %v0 = MOV i64 10;
    %v1 = MOV i64 32;
    %v2 = ADD i64 %v0, %v1;
    
    // Conditional branch
    %cond = ICMP_SGT i1 %v2, 40;
    BR_COND %cond, label %then_block, label %else_block;

then_block:
    %v3 = ADD i64 %v2, 2;
    BR label %exit;

else_block:
    %v4 = SUB i64 %v2, 2;
    BR label %exit;

exit:
    %result = PHI i64 [%v3, label %then_block], [%v4, label %else_block];
    RET i64 %result;
}
```

---

## 4. Architectural Class Design & Subsystem Structure

### 4.1 Target Layout & Component Breakdown

All new parser components will reside under [`EzMir`](file:///E:/Repos/EzPacker/EzMir):

```
EzMir/
├── include/
│   └── Parser/
│       ├── MirParser.h                 <-- Public high-level parser API
│       ├── MirParserContext.h          <-- Symbol tables, forward fixups, arena routing
│       ├── MirLexer.h                  <-- lexy grammar definitions for MIR tokens
│       └── MirAstNodes.h               <-- Transient AST representation before MIR materialization
├── src/
│   └── Parser/
│       ├── MirParser.cpp               <-- Ingestion, module orchestration, pass integration
│       ├── MirParserContext.cpp        <-- Forward reference patching and scope resolution
│       └── MirLexer.cpp                <-- lexy token grammar implementation
```

### 4.2 `MirParserContext` (Symbol Table & Forward Resolver)

```cpp
namespace EzMir
{

enum class SymbolKind
{
    Register,
    BasicBlock,
    Function,
    GlobalVar
};

struct UnresolvedReference
{
    std::string_view m_symbolName;
    SourceReference *m_ref;
    MirInstruction  *m_targetInstruction;
    size_t           m_operandIndex;
    SymbolKind       m_kind;
};

class MirParserContext
{
public:
    MirParserContext(MirBuilderContext *bCtx,
                     DiagnosticCollector *diagCollector,
                     std::pmr::memory_resource *arena);

    // Scoping
    void enterFunction(MirFunction *func);
    void exitFunction();

    // Register mapping
    MirRegister *declareRegister(std::string_view name, MirType *type, SourceReference *ref);
    MirRegister *resolveRegister(std::string_view name, SourceReference *ref);

    // Basic block mapping & forward references
    MirBlock *declareBlock(std::string_view name, SourceReference *ref);
    MirBlock *getOrCreateBlock(std::string_view name, SourceReference *ref);

    // Globals & Functions
    MirGlobalVar *resolveGlobal(std::string_view name, SourceReference *ref);
    MirFunction  *resolveFunction(std::string_view name, SourceReference *ref);

    // Forward reference recording & resolution
    void recordForwardReference(std::string_view name,
                                MirInstruction *inst,
                                size_t operandIdx,
                                SymbolKind kind,
                                SourceReference *ref);
    bool resolveAllPendingFixups();

private:
    MirBuilderContext         *m_bCtx;
    DiagnosticCollector       *m_diag;
    std::pmr::memory_resource *m_arena;
    MirFunction               *m_currentFunction{ nullptr };

    // Function-scoped symbol tables
    std::pmr::unordered_map<std::pmr::string, MirRegister *> m_registers;
    std::pmr::unordered_map<std::pmr::string, MirBlock *>    m_blocks;

    // Module-scoped symbol tables
    std::pmr::unordered_map<std::pmr::string, MirGlobalVar *> m_globals;
    std::pmr::unordered_map<std::pmr::string, MirFunction *>  m_functions;

    // Worklist of forward references to patch
    std::pmr::vector<UnresolvedReference> m_pendingFixups;
};

} // namespace EzMir
```

### 4.3 `MirParser` Public Interface

```cpp
namespace EzMir
{

struct MirParserOptions
{
    bool verifySsa{ true };
    bool allowTargetInstructions{ true };
};

class MirParser
{
public:
    explicit MirParser(MirBuilderContext *ctx,
                       DiagnosticCollector *diagCollector = nullptr,
                       MirParserOptions options = {});

    /**
     * Parses a complete MIR module from a source string buffer.
     * Populates ctx with functions, types, and global variables.
     */
    bool parseModule(std::string_view source, std::string_view bufferName = "input.mir");

    /**
     * Parses a single MIR function from a string buffer and attaches it to ctx.
     */
    MirFunction *parseFunction(std::string_view source);

    /**
     * Parses a single MIR instruction and inserts it at the current builder point.
     */
    MirInstruction *parseInstruction(std::string_view source, MirBlock *targetBlock);

private:
    MirBuilderContext   *m_ctx;
    DiagnosticCollector *m_diag;
    MirParserOptions     m_options;
};

} // namespace EzMir
```

---

## 5. Lexer & Grammar Architecture (lexy-based Engine)

Following the established precedent in [`EzDsl/Lexer`](file:///E:/Repos/EzPacker/EzDsl/Lexer), `MirLexer` leverages `foonathan::lexy` for deterministic parsing with zero dynamic heap allocation:

```cpp
namespace DSL::Parser::Mir
{
namespace dsl = ::lexy::dsl;

// Primitive types: i1, i8, i16, i32, i64, i128, f32, f64, ptr, token, void
struct TypeRule
{
    static constexpr auto whitespace = dsl::ascii::whitespace;
    static constexpr auto rule = [] {
        auto prim = dsl::symbol<MirTypeKind>(
            dsl::symbol_table<MirTypeKind>
                .map(LEXY_LIT("i1"), MirTypeKind::Integer)
                .map(LEXY_LIT("i8"), MirTypeKind::Integer)
                .map(LEXY_LIT("i16"), MirTypeKind::Integer)
                .map(LEXY_LIT("i32"), MirTypeKind::Integer)
                .map(LEXY_LIT("i64"), MirTypeKind::Integer)
                .map(LEXY_LIT("f32"), MirTypeKind::FloatingPoint)
                .map(LEXY_LIT("f64"), MirTypeKind::FloatingPoint)
                .map(LEXY_LIT("ptr"), MirTypeKind::Pointer)
                .map(LEXY_LIT("token"), MirTypeKind::BindingToken)
                .map(LEXY_LIT("void"), MirTypeKind::Void)
        );
        return prim;
    }();
};

// Memory expressions: [ptr %base + %index * scale + disp]
struct MemoryExprRule
{
    static constexpr auto whitespace = dsl::ascii::whitespace;
    static constexpr auto rule = [] {
        auto base = dsl::p<LocalNameRule>;
        auto index = dsl::opt(dsl::lit_c<'+'> >> (dsl::p<LocalNameRule> + dsl::opt(dsl::lit_c<'*'> >> dsl::integer<uint8_t>)));
        auto disp = dsl::opt(dsl::lit_c<'+'> >> dsl::integer<int64_t>);
        return dsl::square_bracketed(base + index + disp);
    }();
};

// Instruction parsing: [dst =] OPCODE [operand, ...]
struct InstructionRule
{
    static constexpr auto whitespace = dsl::ascii::whitespace;
    static constexpr auto rule = [] {
        auto dst = dsl::opt(dsl::p<LocalNameRule> + dsl::lit_c<'='>);
        auto opcode = dsl::identifier(dsl::ascii::alpha_underscore);
        auto operands = dsl::opt(dsl::list(dsl::p<OperandRule>, dsl::sep(dsl::lit_c<','>)));
        return dst + opcode + operands + dsl::lit_c<';'>;
    }();
};

} // namespace DSL::Parser::Mir
```

---

## 6. Round-Trip Serialization & `MirPrinter` Canonicalization

`MirPrinter` currently formats output with diagnostic columns for debug inspection:
```text
  HL  ADD          (unselected)         i64 %v2(unassigned), i64 %v0, i64 %v1
```

To achieve **Invariant [INV-TMP-04]** (Idempotent Round-Trip), `MirPrinter` will be extended with a **Canonical Parseable Format**:
```cpp
enum class MirPrinterMode
{
    Diagnostic, // Human inspection with Tier, Opcode column alignment, and unselected tags
    Parseable   // Strict, round-trip parseable valid .mir text
};

class MirPrinter
{
public:
    // Enhanced print method with parseable mode
    static std::string printModule(class MirBuilderContext *ctx, MirPrinterMode mode = MirPrinterMode::Parseable);
    static std::string printFunction(class MirFunction *func, MirPrinterMode mode = MirPrinterMode::Parseable);
};
```

When emitted in `MirPrinterMode::Parseable`:
```mir
fn @main() -> i64 {
entry:
    MOV i64 %v0, 42;
    RET i64 %v0;
}
```
This guarantees that `MirParser::parseModule(MirPrinter::printModule(ctx))` produces an exact duplicate of the in-memory MIR graph.

---

## 7. Driver Integration (`EzCompiler` / `ezc`)

[`EzCompiler`](file:///E:/Repos/EzPacker/EzCompiler) already implements [`FrontendAdapter.cpp`](file:///E:/Repos/EzPacker/EzCompiler/src/FrontendAdapter.cpp) which currently creates synthetic test functions.

With `MirParser`, the integration in `FrontendAdapter::loadMirFile()` is seamless:

```cpp
bool FrontendAdapter::loadMirFile(DriverContext &ctx, const std::string &mirPath)
{
    SourceManager *sourceMgr = ctx.getSourceManager();
    auto bufferOpt = sourceMgr->loadFile(mirPath);
    if (!bufferOpt.has_value())
    {
        ctx.getDiagCollector()->error("EzCompiler", "Failed to open input MIR file: {}", mirPath);
        return false;
    }

    MirParserOptions parserOptions;
    parserOptions.verifySsa = (ctx.getOptions().optLevel != OptimizationLevel::None);

    MirParser parser(ctx.getBuilderContext(), ctx.getDiagCollector(), parserOptions);
    if (!parser.parseModule(bufferOpt->getBuffer(), mirPath))
    {
        ctx.getDiagCollector()->error("EzCompiler", "Failed to parse MIR file: {}", mirPath);
        return false;
    }

    return true;
}
```

Now `ezc sample.mir -o sample.o` reads arbitrary `.mir` programs directly from disk, runs them through target lowering, and emits native object code.

---

## 8. Verification & Test Suite Matrix

A comprehensive test suite `EzMirTestSuite_T_MirParser` and integration suite `EzCompilerTestSuite_T_CompilerMirInput` will be created:

| Test ID | Test Category | Target Coverage | Success Criteria |
| :--- | :--- | :--- | :--- |
| `TMP-01` | **Type Parsing** | Primitives, vectors, pointers, tokens, arrays | All types resolve to unique pointers in `MirTypeTable`. |
| `TMP-02` | **Global Variables** | `internal`, `external`, `weak`, initialized & zeroinit | `MirGlobalVar` created with correct byte payloads. |
| `TMP-03` | **Basic Blocks & CFG** | Single & multi-block functions, loops, diamonds | CFG edges correctly connect predecessors and successors. |
| `TMP-04` | **Forward Block References** | Jump to label defined later in file | Forward references resolved without error. |
| `TMP-05` | **Memory Addressing** | SIB operands `[ptr %b + %i*4 + 16]` | Base, index, scale, disp parsed accurately. |
| `TMP-06` | **Phi Nodes** | SSA `PHI i64 [%v1, %bb1], [%v2, %bb2]` | `PHI` instruction with paired operands populated. |
| `TMP-07` | **ABI Tokens** | `PUSH_ARG`, `POP_ARG`, `END_ARG`, `PUSH_RET` | Internal lowering tokens preserved and bound to tokens. |
| `TMP-08` | **Round-Trip Idempotence** | Parse $\rightarrow$ Print $\rightarrow$ Re-parse $\rightarrow$ Compare | In-memory representations are structurally isomorphic. |
| `TMP-09` | **Error Diagnostics** | Syntax errors, undeclared registers, duplicate labels | Byte-exact source diagnostics with line/col carets. |
| `TMP-10` | **End-to-End CLI Pipeline** | `ezc test.mir -o test.o` | Binary ELF/COFF generated with valid headers and code. |

---

## 9. Phased Implementation Roadmap

```
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                   Phased Implementation Schedule                                 │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘

  [ Phase 1: Grammar & Lexer ] ────────► [ Phase 2: Parser Context & Symbol Resolver ]
             │                                              │
             ▼                                              ▼
  [ Phase 3: Function & CFG Parser ] ──► [ Phase 4: Full Instruction Set & Memory Addressing ]
             │                                              │
             ▼                                              ▼
  [ Phase 5: Round-Trip Serializer ] ──► [ Phase 6: ezc Integration & End-to-End Testing ]
```

### Phase 1: Grammar Specification & Lexer Rules
- Define Lexy tokens for MIR keywords, types, registers (`%v0`), globals (`@g`), labels, and integers.
- Implement `EzMir/include/Parser/MirLexer.h` and unit test lexical scanning.

### Phase 2: Parser Context & Symbol Resolution
- Implement `MirParserContext` with scoped registers, block labels, globals, and fixup table.
- Implement forward reference patching for labels and function symbols.

### Phase 3: Function Signature & CFG Parser
- Parse `fn @name(...) -> type`, attributes, and basic blocks.
- Construct `MirFunction` and `MirBlock` instances in `MirBuilderContext`.

### Phase 4: Full Instruction Set & Operand Dispatcher
- Implement parser for all high-level opcodes (`ADD`, `SUB`, `MOV`, `LOAD`, `STORE`, `BR`, `CALL`, etc.).
- Parse complex memory expressions `[ptr %base + %index*scale + disp]`.
- Implement `PHI` and ABI token parsers.

### Phase 5: Canonical `MirPrinter` & Round-Trip Tests
- Add `MirPrinterMode::Parseable` to `MirPrinter`.
- Create unit test verifying `Parse(Print(Module)) == Module`.

### Phase 6: Compiler Driver Integration & Verification
- Wire `MirParser` into `EzCompiler::FrontendAdapter`.
- Author sample `.mir` test files and verify compilation to ELF64 and PE-COFF.
- Run `graphify update .` to synchronize knowledge graph.
