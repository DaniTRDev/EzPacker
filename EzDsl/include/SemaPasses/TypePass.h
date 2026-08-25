#ifndef EZDSL_TYPE_PASS_H
#define EZDSL_TYPE_PASS_H

#include "EzDslCommon.h"
#include "Ast/TypeDefLangAst.h"

/**
 * Semantic analysis pass processing parsed TypeDefFile (.tyf) AST trees.
 * Registers declared primitive and special types as SymbolType::Type in the active symbol table scope.
 * Validates bit-widths, prevents duplicate declarations, and emits diagnostic messages on violations.
 */
class TypePass
{
  public:
    /**
     * Executes the type declaration semantic pass over the provided AST root.
     * Returns true if all types were successfully validated and declared without errors.
     */
    bool run(class DiagnosticCollector *collector, class SymbolTable *table, DSL::Ast::TypeDef::TypeDefFile *file);
};

#endif // EZDSL_TYPE_PASS_H