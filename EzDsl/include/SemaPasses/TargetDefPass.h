#ifndef EZDSL_TARGET_DEF_PASS_H
#define EZDSL_TARGET_DEF_PASS_H

#include "EzDslCommon.h"
#include "Ast/TargetDefLangAst.h"

/**
 * Semantic analysis pass processing target architecture declarations (.tdf).
 * Validates target architecture names and declares the TargetSymbol in the symbol table.
 */
class TargetDefPass
{
  public:
    /**
     * Executes the target architecture declaration pass over a TargetDef AST node.
     * Returns true if the target symbol was successfully declared in the symbol table.
     */
    static bool
    run(class DiagnosticCollector *collector, class SymbolTable *table, DSL::Ast::TargetDef::TargetDef *file);
};

#endif // EZDSL_TARGET_DEF_PASS_H
