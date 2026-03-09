/**
 * @file EzLexer.h
 * @brief Umbrella header for the EzLexer library.
 *
 * EzLexer is the lexical analysis and parsing front-end of the EzPacker compiler.
 * It is responsible for two main tasks:
 *
 *   1. **Tokenization** – Splitting raw source text into a flat stream of tokens
 *      (keywords, identifiers, operators, literals, etc.) via BasicTokenizer.
 *
 *   2. **Parsing** – Consuming that token stream and building a typed Abstract
 *      Syntax Tree (AST).  Each language construct (modules/functions, labels,
 *      instructions, variables, if/else, while loops, break, continue, memory
 *      operands, immediates, and conditions) has a dedicated parser that
 *      produces the corresponding AST node.  Parsers are grouped into a
 *      ParserBatch and executed through a shared BasicParsingContext.
 *
 * The resulting AST is the input for downstream semantic analysis (EzSemantics)
 * and code generation (EzMir).
 *
 * Including this single header gives you access to every public type in EzLexer:
 * the tokenizer, all parsers, every AST node, the visitor infrastructure, and
 * the supporting pool/error utilities re-exported from EzCore.
 */
#ifndef EZPACKER_EZLEXER_H
#define EZPACKER_EZLEXER_H

#include "EzLexerCommon.h"

// ── AST node base types & visitor infrastructure ────────────────────────────
#include "AstNode/AstNode.h"
#include "AstNode/AstNodeContainer.h"
#include "AstNode/AstNodeTypedPool.h"
#include "AstNode/AstNodeVisitor.h"

// ── Parsing framework (context, interface, batch runner) ────────────────────
#include "AstNodeParsers/BasicParsingContext.h"
#include "AstNodeParsers/IAstNodeParser.h"
#include "AstNodeParsers/ParserBatch.h"

// ── Individual parsers (one per language construct) ─────────────────────────
#include "AstNodeParsers/Parsers/BreakParser.h"
#include "AstNodeParsers/Parsers/CodeScopeParser.h"
#include "AstNodeParsers/Parsers/ConditionParser.h"
#include "AstNodeParsers/Parsers/ContinueParser.h"
#include "AstNodeParsers/Parsers/ForParser.h"
#include "AstNodeParsers/Parsers/IfParser.h"
#include "AstNodeParsers/Parsers/ImmediateParser.h"
#include "AstNodeParsers/Parsers/IncludeParser.h"
#include "AstNodeParsers/Parsers/InstructionParser.h"
#include "AstNodeParsers/Parsers/LabelParser.h"
#include "AstNodeParsers/Parsers/MemoryOperandParser.h"
#include "AstNodeParsers/Parsers/ModuleParser.h"
#include "AstNodeParsers/Parsers/VariableParser.h"
#include "AstNodeParsers/Parsers/SwitchParser.h"
#include "AstNodeParsers/Parsers/WhileParser.h"

// ── Concrete AST node types ─────────────────────────────────────────────────
#include "AstNodes/BreakAstNode.h"
#include "AstNodes/CodeScope.h"
#include "AstNodes/ConditionAstNode.h"
#include "AstNodes/ContinueAstNode.h"
#include "AstNodes/ForAstNode.h"
#include "AstNodes/IfAstNode.h"
#include "AstNodes/ImmediateOperand.h"
#include "AstNodes/IncludeAstNode.h"
#include "AstNodes/Instruction.h"
#include "AstNodes/Label.h"
#include "AstNodes/MemoryOperand.h"
#include "AstNodes/Module.h"
#include "AstNodes/Variable.h"
#include "AstNodes/SwitchAstNode.h"
#include "AstNodes/SwitchCaseAstNode.h"
#include "AstNodes/WhileAstNode.h"

// ── Error reporting ─────────────────────────────────────────────────────────
#include "ErrorCollector/ErrorCollector.h"

// ── Memory pool utilities (re-exported from EzCore) ─────────────────────────
#include "TypedPool/StringPool.h"
#include "TypedPool/TypedArrayPool.h"
#include "TypedPool/TypedPool.h"

// ── Tokenizer ───────────────────────────────────────────────────────────────
#include "Tokenizer/BasicTokenizer.h"

#endif // EZPACKER_EZLEXER_H
