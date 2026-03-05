/**
 * @file EzSemantics.h
 * @brief Umbrella header for the EzSemantics library.
 *
 * EzSemantics sits between the parser (EzLexer) and the final code output.
 * It takes the raw AST produced by the lexer and transforms it into validated,
 * type-checked MIR (Mid-level Intermediate Representation) ready for
 * optimization or emission.  The pipeline has two major phases:
 *
 *   1. **Semantic Analysis** (SymbolVisitors) — A sequence of AST visitor
 *      passes that progressively enrich the tree with meaning:
 *        • SymbolDefinitionVisitor  – Creates scopes and registers every
 *          declared symbol (variables, labels, modules/functions, parameters).
 *        • SymbolAndTypeResolverVisitor – Resolves name references to their
 *          defining symbols and attaches concrete type information.
 *        • TypeCheckVisitor – Validates type compatibility across operands
 *          and enforces structural rules (e.g. break/continue must appear
 *          inside a loop).
 *      Each pass annotates AST nodes with SemanticAnnotations (symbol links,
 *      data types, scope ownership, type-cast requirements) so that later
 *      stages never need to re-derive that information.
 *
 *   2. **AST-to-MIR Lowering** (AstLowererVisitor) — An AST visitor that
 *      walks the fully-annotated tree and emits the equivalent MIR.  Every
 *      language construct has a dedicated lowerer:
 *        • ModuleLowerer   – Creates the function entry block and lowers
 *          header parameters, then delegates to the body.
 *        • LabelLowerer    – Creates a new basic block and links the label
 *          symbol to it.
 *        • InstructionLowerer – Maps mnemonics to MIR opcodes and lowers
 *          each operand.
 *        • VariableLowerer / ImmediateLowerer / MemoryLowerer – Emit
 *          virtual registers, integer/float constants, and memory operands.
 *        • ConditionLowerer – Emits CMP + conditional jump sequences.
 *        • IfLowerer        – Creates true/false/merge blocks and wires
 *          the condition into them.
 *        • WhileLowerer     – Creates condition-check, loop-body, and exit
 *          blocks, and pushes a LoopContext for break/continue handling.
 *        • BreakLowerer     – Emits a JMP to the current loop's exit block
 *          and opens a dead-code block for any unreachable code that follows.
 *        • ContinueLowerer  – Emits a JMP back to the current loop's
 *          condition-check block, also opening a dead-code block.
 *        • CodeScopeLowerer – Iterates through a brace-delimited scope and
 *          lowers every child expression in order.
 *      A shared LoweringContext tracks the block/operand stacks and the
 *      nested LoopContext stack that break/continue rely on.
 *
 * Supporting infrastructure:
 *   - Scope / Symbol / TypeTable – The symbol table and type registry that
 *     back every name-resolution and type-checking decision.
 *   - BasicSemanticContext – A façade that owns the global scope, the
 *     error collector, the symbol-to-MIR-ID linkage map, and loop-nesting
 *     tracking used across all visitor passes.
 *   - SemanticVisitor – A thin base class that all semantic visitors derive
 *     from, providing the shared context plumbing.
 *
 * Including this single header gives you access to every public type in
 * EzSemantics: the semantic context, all visitor passes, every annotation,
 * the scope/symbol infrastructure, and the complete set of AST-to-MIR
 * lowerers.
 */
#ifndef EZPACKER_EZSEMANTICS_H
#define EZPACKER_EZSEMANTICS_H

#include "EzSemanticsCommon.h"

// ── Core semantic context & visitor base ────────────────────────────────────
#include "BasicSemanticContext.h"
#include "SemanticVisitor.h"

// ── AST-to-MIR lowerers (one per language construct) ────────────────────────
#include "AstLowererVisitor/AstLowererVisitor.h"
#include "AstLowererVisitor/BreakLowerer.h"
#include "AstLowererVisitor/CodeScopeLowerer.h"
#include "AstLowererVisitor/ConditionLowerer.h"
#include "AstLowererVisitor/ContinueLowerer.h"
#include "AstLowererVisitor/GenericLowerer.h"
#include "AstLowererVisitor/IfLowerer.h"
#include "AstLowererVisitor/ImmediateLowerer.h"
#include "AstLowererVisitor/InstructionLowerer.h"
#include "AstLowererVisitor/LabelLowerer.h"
#include "AstLowererVisitor/LoweringContext.h"
#include "AstLowererVisitor/MemoryLowerer.h"
#include "AstLowererVisitor/ModuleLowerer.h"
#include "AstLowererVisitor/VariableLowerer.h"
#include "AstLowererVisitor/WhileLowerer.h"

// ── Symbol table & type registry ────────────────────────────────────────────
#include "Scope/Scope.h"
#include "Scope/Symbol.h"
#include "Scope/TypeTable.h"

// ── AST annotations attached during semantic passes ─────────────────────────
#include "SemanticAnnotations/DataTypeAnnotation.h"
#include "SemanticAnnotations/ScopeAnnotation.h"
#include "SemanticAnnotations/ScopedSymbolAnnotation.h"
#include "SemanticAnnotations/SymbolAnnotation.h"
#include "SemanticAnnotations/TypeCastAnnotation.h"

// ── Semantic visitor passes (definition → resolution → type checking) ───────
#include "SymbolVisitors/SymbolAndTypeResolverVisitor.h"
#include "SymbolVisitors/SymbolDefinitionVisitor.h"
#include "SymbolVisitors/TypeCheckVisitor.h"

#endif // EZPACKER_EZSEMANTICS_H
