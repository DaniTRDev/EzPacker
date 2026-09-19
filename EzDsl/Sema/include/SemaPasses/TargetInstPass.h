#ifndef EZDSL_TARGET_INST_PASS_H
#define EZDSL_TARGET_INST_PASS_H

#include "EzDslSemaCommon.h"
#include "Ast/TargetInstDefLangAst.h"

class DiagnosticCollector;
class SymbolTable;

/**
 * Semantic analysis pass validating target machine instruction declarations (.idf).
 * Registers TargetInstructionSymbol entries in the symbol table and enforces integrity rules:
 * operand uniqueness, duplicate opcode prevention, and implicit register dependencies.
 */
class TargetInstPass
{
  public:
    /**
     * Executes the target instruction semantic validation pass across all declared opcodes in the AST.
     * Returns true if all instructions were successfully validated and registered.
     */
    static bool run(DiagnosticCollector *collector, SymbolTable *table, DSL::Ast::TargetInstDef::TargetInstFile *file);

  private:
    /**
     * Validates a single target instruction declaration.
     */
    static bool validateInstruction(DiagnosticCollector *collector,
                                    SymbolTable *table,
                                    const DSL::Ast::TargetInstDef::TargetInstDecl &inst);
};

#endif // EZDSL_TARGET_INST_PASS_H
