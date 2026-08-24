#ifndef EZDSL_TYPE_PASS_H
#define EZDSL_TYPE_PASS_H

#include "EzDslCommon.h"
#include "Ast/TypeDefLangAst.h"

/**
 * This pass runs on the resulting AST of parsing a .tyf file. It will register each type as a symbol in the symbol
 * table.
 *
 * This pass is scope-sensible, meaning symbols will be declared in the current scope of the symbol table. Caller must
 * ensure it is the global scope or at least, it is higher that everything else that references a type.
 */
class TypePass
{
  public:
    /**
     * Runs the pass and returns true if succeeded.
     */
    bool run(class DiagnosticCollector *collector, class SymbolTable *table, DSL::Ast::TypeDef::TypeDefFile *file);
};

#endif // EZDSL_TYPE_PASS_H