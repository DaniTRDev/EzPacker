#ifndef EZDSLSEMA_REGISTER_PASS_H
#define EZDSLSEMA_REGISTER_PASS_H

#include "Ast/RegisterDefLangAst.h"
#include "EzDslSemaCommon.h"

/**
 * Semantic analysis pass processing parsed .reg register definition files.
 * Declares register banks, classes, physical registers, and pseudo registers as symbols,
 * validating naming/hardware-encoding uniqueness, class references, sub-register edges,
 * and special-register id collisions.
 */
class RegisterPass
{
  public:
    /**
     * Executes the register definition semantic pass over the provided AST root.
     * Returns true if all declarations were successfully validated and declared.
     */
    bool run(class DiagnosticCollector *collector,
             class SymbolTable *table,
             DSL::Ast::RegisterDef::RegisterFile *file);
};

#endif // EZDSLSEMA_REGISTER_PASS_H
