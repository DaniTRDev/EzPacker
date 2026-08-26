#ifndef EZDSL_INST_SEL_PASS_H
#define EZDSL_INST_SEL_PASS_H

#include "Ast/InstructionSelDefLangAst.h"


// Forward declarations.
namespace Sema::Symbols
{
    class AddrModeVariantSymbol;
    class RuleInstructionSymbol;
};

/**
 * Executes the semantic analysis and symbol resolution pass for instruction selection definitions (.isf). It registers
 * selection rules for specific sequences of IR isntructions.
 */
class InstSelPass
{
  public:
    static bool run(DiagnosticCollector *collector, SymbolTable *table, DSL::Ast::InstSelDef::ISelDefFile *file);

  private:
    static bool
    processAddrMode(DiagnosticCollector *collector, SymbolTable *table, const DSL::Ast::InstSelDef::AddrModeDef &def);

    static bool processAddrModeVariant(DiagnosticCollector *collector,
                                       SymbolTable *table,
                                       const DSL::Ast::InstSelDef::AddrModeVariant &variant,
                                       std::string_view addrModeName,
                                       Sema::Symbols::AddrModeVariantSymbol &outVariant);

    static bool processPattern(DiagnosticCollector *collector,
                               SymbolTable *table,
                               const DSL::Ast::InstSelDef::ISelPattern &pattern);

    static bool processMatchInstruction(DiagnosticCollector *collector,
                                        SymbolTable *table,
                                        const DSL::Ast::LegalizeRuleDef::RuleInstruction &inst,
                                        std::string_view contextName,
                                        Sema::Symbols::RuleInstructionSymbol &outInst);

    static bool processEmitInstruction(DiagnosticCollector *collector,
                                       SymbolTable *table,
                                       const DSL::Ast::LegalizeRuleDef::RuleInstruction &inst,
                                       std::string_view patternName,
                                       Sema::Symbols::RuleInstructionSymbol &outInst);

    static bool processPredicate(DiagnosticCollector *collector,
                                 SymbolTable *table,
                                 const DSL::Ast::LegalizeRuleDef::RulePredicate &predicate,
                                 std::string_view contextName);

    static bool resolveRuleOperand(DiagnosticCollector *collector,
                                   SymbolTable *table,
                                   const DSL::Ast::LegalizeRuleDef::RuleOperand &operand,
                                   std::string_view contextName,
                                   bool isMatchPattern,
                                   Sema::Symbols::RuleOperandSymbol &outOperand);
};

#endif // EZDSL_INST_SEL_PASS_H