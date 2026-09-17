#ifndef EZDSL_INSTRUCTION_SELECT_PASS_H
#define EZDSL_INSTRUCTION_SELECT_PASS_H

#include "EzDslSemaCommon.h"
#include "Ast/InstructionSelectDefLangAst.h"

class DiagnosticCollector;
class SymbolTable;

/**
 * Semantic analysis pass validating instruction selection patterns (.isf).
 * Registers AddressingMode and SelectionPattern symbols in the symbol table,
 * checks variable binding consistency between match, when, and select clauses,
 * and ensures referenced target instructions and addressing modes exist.
 */
class InstructionSelectPass
{
  public:
    static bool run(DiagnosticCollector *collector,
                    SymbolTable *table,
                    DSL::Ast::InstructionSelectDef::InstructionSelectFile *file);

  private:
    static bool validateAddrMode(DiagnosticCollector *collector,
                                 SymbolTable *table,
                                 const DSL::Ast::InstructionSelectDef::AddrModeDecl &mode);

    static bool validatePattern(DiagnosticCollector *collector,
                                SymbolTable *table,
                                const DSL::Ast::InstructionSelectDef::SelectionPattern &pattern);
};

#endif // EZDSL_INSTRUCTION_SELECT_PASS_H
