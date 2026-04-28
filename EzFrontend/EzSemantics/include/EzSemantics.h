/**
 * @file EzSemantics.h
 * @brief Umbrella header for the EzSemantics library.
 *
 * EzSemantics is the semantic-analysis and lowering layer that sits between
 * the parsed Ez AST and the MIR back-end. A typical consumer uses it in this
 * order:
 *
 *   1. Create a shared `BasicSemanticContext`.
 *   2. Run the semantic passes on the AST:
 *        - `SymbolDefinitionVisitor`
 *        - `SymbolAndTypeResolverVisitor`
 *        - `TypeCheckVisitor`
 *
 * Semantic analysis enriches the AST with `SemanticAnnotations` so later
 * passes can consume resolved symbols, owned scopes, data types and required
 * casts without re-deriving them.
 *
 * Scope-sensitive constructs supported by the semantic passes include modules,
 * labels, `if` branches, `while`, `for` and `switch` / `case`. Note that the
 * umbrella header exposes the public lowering infrastructure only; not every
 * AST construct necessarily has a dedicated public lowerer class.
 *
 * Including this header gives third-party code access to the public
 * EzSemantics API surface: context objects, semantic passes, scope/type
 * infrastructure, and AST annotations
 */
#ifndef EZPACKER_EZSEMANTICS_H
#define EZPACKER_EZSEMANTICS_H

#include "EzSemanticsCommon.h"

// ── Core semantic context & visitor base ────────────────────────────────────
#include "BasicSemanticContext.h"
#include "SemanticVisitor.h"

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
