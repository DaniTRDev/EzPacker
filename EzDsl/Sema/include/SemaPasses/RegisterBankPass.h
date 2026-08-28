#ifndef EZDSL_REGISTER_BANK_PASS_H
#define EZDSL_REGISTER_BANK_PASS_H

#include "EzDslCommon.h"
#include "Ast/TargetDefLangAst.h"

/**
 * Semantic analysis pass processing target register bank and register class definitions (.tdf).
 * Validates register aliases, parent-subregister hierarchies, bit-sizes, bit-offsets,
 * and detects cyclic register parent dependencies using cycle-finding algorithms.
 */
class RegisterBankPass
{
  public:
    /**
     * Executes the register bank semantic pass over a TargetDef AST node.
     * Returns true if all banks, classes, registers, and alias hierarchies were successfully resolved.
     */
    static bool
    run(class DiagnosticCollector *collector, class SymbolTable *table, DSL::Ast::TargetDef::TargetDef *file);

  private:
    /**
     * Declares all register bank and register class symbols in the symbol table.
     */
    static bool
    declareBanks(class DiagnosticCollector *collector, class SymbolTable *table, DSL::Ast::TargetDef::TargetDef *file);

    /**
     * Processes classes within a single register bank.
     */
    static bool runOnBank(class DiagnosticCollector *collector,
                          class SymbolTable *table,
                          const DSL::Ast::TargetDef::TargetRegisterBank *bank,
                          SymbolId bankSymId);

    /**
     * Processes and declares hardware registers inside a register class.
     */
    static bool runOnClass(class DiagnosticCollector *collector,
                           class SymbolTable *table,
                           const DSL::Ast::TargetDef::TargetRegisterClass *_class,
                           SymbolId classSymId,
                           SymbolId bankSymId);

    /**
     * Resolves parent register aliases, validating that subregisters fit within the bit range of their parent.
     */
    static bool resolveHierarchies(class DiagnosticCollector *collector,
                                   class SymbolTable *table,
                                   DSL::Ast::TargetDef::TargetDef *file);

    /**
     * Runs depth-first cycle detection across register parent-child references to prevent infinite alias loops.
     */
    static bool detectRegisterCycles(DiagnosticCollector *collector, SymbolTable *table);
};

#endif // EZDSL_REGISTER_BANK_PASS_H