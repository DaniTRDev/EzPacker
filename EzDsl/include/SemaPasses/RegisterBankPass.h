#ifndef EZDSL_REGISTER_BANK_PASS_H
#define EZDSL_REGISTER_BANK_PASS_H

#include "EzDslCommon.h"
#include "Ast/TargetDefLangAst.h"

class RegisterBankPass
{
  public:
    static bool
    run(class DiagnosticCollector *collector, class SymbolTable *table, DSL::Ast::TargetDef::TargetDef *file);

  private:
    static bool
    declareBanks(class DiagnosticCollector *collector, class SymbolTable *table, DSL::Ast::TargetDef::TargetDef *file);

    static bool runOnBank(class DiagnosticCollector *collector,
                          class SymbolTable *table,
                          const DSL::Ast::TargetDef::TargetRegisterBank *bank,
                          SymbolId bankSymId);

    static bool runOnClass(class DiagnosticCollector *collector,
                           class SymbolTable *table,
                           const DSL::Ast::TargetDef::TargetRegisterClass *_class,
                           SymbolId classSymId,
                           SymbolId bankSymId);

    static bool resolveHierarchies(class DiagnosticCollector *collector,
                                   class SymbolTable *table,
                                   DSL::Ast::TargetDef::TargetDef *file);

    static bool detectRegisterCycles(DiagnosticCollector *collector, SymbolTable *table);
};

#endif // EZDSL_REGISTER_BANK_PASS_H