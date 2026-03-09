# EzLexer

`EzLexer` is the lexical and syntactic front-end for the Ez language used by `EzPacker`.
It turns source text into a typed AST that later passes in `EzSemantics` resolve and validate.

## What EzLexer owns

EzLexer is responsible for:

1. Tokenizing source text into `TokenInformation` entries.
2. Parsing tokens into `AstNode` trees.
3. Preserving source locations for diagnostics.
4. Storing parsed nodes and interned strings in cache-friendly pools.

EzLexer is **not** responsible for:

- name resolution,
- type resolution,
- scope validation,
- instruction legality,
- MIR generation.

Those steps happen later in `EzSemantics` and `EzMir`.

---

## High-level flow

Typical use looks like this:

1. Register or load source text in `SourceManager`.
2. Build a `BasicTokenizer` with the shared `ErrorCollector` and `SourceManager`.
3. Call `tokenizeBuffer(0, sourceId)`.
4. Move the resulting token vector into `BasicParsingContext`.
5. Invoke a parser directly, or use `ParserBatch` to try multiple grammar alternatives.
6. Consume the returned `AstNode*` tree while the parsing context is still alive.

Important lifetime rule:

- AST nodes live inside the `AstNodeTypedPool` owned by `BasicParsingContext`.
- Interned strings live inside the `StringPool` owned by `BasicParsingContext`.
- Keep the parsing context alive for as long as you want to access parsed nodes or pooled strings.

---

## Tokenizer contract

Public entry point: `Tokenizer/BasicTokenizer.h`

Observable tokenizer behavior:

- Identifiers follow `[a-zA-Z_][a-zA-Z0-9_]*`.
- Reserved words such as `if`, `else`, `while`, `switch`, `case`, `default`, `break`, and `continue` are emitted as dedicated token kinds.
- Decimal integers, hexadecimal integers (`0x...`), floats, and strings are supported.
- Strings are double-quoted and support common escape sequences.
- `#` starts a line comment.
- Spaces, tabs, line breaks, and comments are consumed by the tokenizer and are not meant to be relied on as parser-visible tokens.
- Signed numeric literals are represented syntactically as `Minus` + numeric token, not as one token.

When tokenization fails, the tokenizer emits a fatal error to the shared `ErrorCollector` and returns `false`.

---

## Parsing contract

Core types:

- `AstNodeParsers/IAstNodeParser.h`
- `AstNodeParsers/BasicParsingContext.h`
- `AstNodeParsers/ParserBatch.h`

### `IAstNodeParser`

Every parser implements:

- `AstNode *parse(const std::shared_ptr<BasicParsingContext>& ctx)`

Return conventions:

- Returns a non-null node on success.
- Returns `nullptr` when the parser does not match.
- May emit soft or fatal errors depending on how far it got.

### `BasicParsingContext`

The parsing context owns:

- the token array,
- the current cursor,
- the AST node pool,
- the string pool,
- the error emission services.

Behavior that matters to parser authors:

- `peek()` inspects the current token without consuming it.
- `consume()` advances one logical token and skips trailing tabs/newlines.
- `consumeIf(...)` is the preferred match helper.
- `getLastSourceReference()` reports the last successfully consumed token.

### `ParserBatch`

Use `ParserBatch` when multiple parsers could be attempted at the same cursor position.

It:

- saves the stream position before each parser attempt,
- rolls back on soft failure,
- stops immediately on fatal failure,
- returns the first successful node.

Registration order matters.
Place specialized parsers before generic ones.

---

## AST overview

Every parsed construct derives from `AstNode`.

Common APIs:

- `getType()` — fast enum-based node kind check.
- `getAstNodeName()` — stable debug name.
- `getSourceRef()` — source location for diagnostics.
- `accept(AstNodeVisitor*)` — visitor dispatch.
- annotation APIs — used later by semantic passes.

### Common container pattern

Several nodes also inherit from `AstNodeContainer`.
That means they own an ordered slice of child expressions.

Examples:

- `Instruction` stores operands.
- `ModuleHeader` stores parameter variables.
- `CodeScope` stores statements.
- `Variable` may store initializer expressions.

---

## Public grammar supported by the main parsers

### Variables

Parser: `VariableParser`

Accepted forms:

- `%name`
- `type %name`
- `type %name: immediate`
- `type %name: { immediate, immediate, ... }`

Notes:

- The `%` is mandatory.
- Initializers are immediate-only at lexer/parser level.
- Whether a variable is a parameter, local, global, or symbol use is decided later.

### Immediates

Parser: `ImmediateParser`

Accepted forms:

- integer: `0`, `123`, `0xFF`
- float: `3.14`
- string: `"hello"`

Concrete AST classes:

- `IntegerImmediate`
- `FloatImmediate`
- `StringImmediate`

### Memory operands

Parser: `MemoryOperandParser`

Accepted forms:

- `type (%base + displacement)`
- `type (%base - displacement)`
- `type (%base, %index, scale, displacement)`
- `type (, %index, scale)`
- `type (address)`

Concrete AST classes:

- `BaseDisplacementMemory`
- `BaseIndexScaleDisplacementMemory`
- `IndexScaleMemory`
- `DirectMemory`

### Instructions

Parser: `InstructionParser`

Accepted forms:

- `mnemonic;`
- `mnemonic operand;`
- `mnemonic operand1, operand2;`
- `call %callee();`
- `call %callee(arg1, arg2);`

Notes:

- The parser normalizes instruction mnemonics to lower case.
- Operands are stored in source order.
- `CallInstruction` stores the callee as expression 0, followed by arguments.

### Conditions

Parser: `ConditionParser`

Observed comparison operators from tests:

- `EQ`, `NE`, `GT`, `GE`, `LT`, `LE`

Typical forms:

- `%a EQ %b`
- `%a LT 123`

### Modules

Parser: `ModuleParser`

Accepted form:

- `returnType Name(param1, param2) { ... }`

AST structure:

- `ModuleHeader` stores return type, module name, and parameter list.
- `Module` stores the header and the body `CodeScope`.

### Control flow

Available statement-level parsers include:

- `IfParser`
- `WhileParser`
- `ForParser`
- `BreakParser`
- `ContinueParser`
- `LabelParser`
- `SwitchParser`

#### Switch

Observed grammar from `SwitchParser`:

- `switch (%var) { case immediate: { ... } default: { ... } }`

AST shape:

- `SwitchAstNode` stores the selector variable, ordered `SwitchCaseAstNode` entries, and an optional default body.
- `SwitchCaseAstNode` stores one `ImmediateOperand` case value and one `CodeScope` body.

---

## Error model

EzLexer uses the shared `ErrorCollector` and distinguishes between two practical parser outcomes:

- **soft failure** — this parser did not match; another parser may still succeed.
- **fatal failure** — the parser has already committed to a grammar shape and found malformed input.

This distinction matters most when a parser is used inside `ParserBatch`.

---

## Recommended integration pattern

If you are adding a new frontend consumer, prefer this shape:

1. Tokenize once with `BasicTokenizer`.
2. Create one `BasicParsingContext` per parse session.
3. Use `ParserBatch` for top-level alternatives.
4. Keep the context alive while traversing the AST.
5. Do semantic decisions in `EzSemantics`, not inside EzLexer.

---

## Where to look next

For API details, start with these headers:

- `include/EzLexer.h`
- `include/Tokenizer/BasicTokenizer.h`
- `include/AstNodeParsers/BasicParsingContext.h`
- `include/AstNodeParsers/ParserBatch.h`
- `include/AstNodeParsers/Parsers/*.h`
- `include/AstNodes/*.h`

For behavior examples, the parser and tokenizer tests under `tests/` are the best executable reference.

