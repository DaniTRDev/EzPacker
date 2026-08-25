#ifndef EZDSL_LEGALIZE_RULE_PASS_H
#define EZDSL_LEGALIZE_RULE_PASS_H

#include "EzDslCommon.h"
#include "Ast/LegalizeRuleDefLangAst.h"

/**
 * Forward declarations.
 */
namespace Sema::Symbols
{
class RuleInstructionSymbol;
class RuleOperandSymbol;
}; // namespace Sema::Symbols

class LegalizeRulePass
{
  public:
    /**
     * Executes the semantic analysis and symbol resolution pass for legalization rewrite rules.
     */
    static bool run(class DiagnosticCollector *collector,
                    class SymbolTable *table,
                    DSL::Ast::LegalizeRuleDef::TargetLegalizeRuleDef *file);

  private:
    static bool processRule(class DiagnosticCollector *collector,
                            class SymbolTable *table,
                            const DSL::Ast::LegalizeRuleDef::LegalizeRewriteRule &rule);

    static bool processMatchPattern(class DiagnosticCollector *collector,
                                    class SymbolTable *table,
                                    const DSL::Ast::LegalizeRuleDef::RuleInstruction &inst,
                                    std::string_view ruleName,
                                    Sema::Symbols::RuleInstructionSymbol &outInst);

    static bool processPredicate(class DiagnosticCollector *collector,
                                 class SymbolTable *table,
                                 const DSL::Ast::LegalizeRuleDef::RulePredicate &predicate,
                                 std::string_view ruleName);

    static bool processExpandInstruction(class DiagnosticCollector *collector,
                                         class SymbolTable *table,
                                         const DSL::Ast::LegalizeRuleDef::RuleInstruction &inst,
                                         std::string_view ruleName,
                                         Sema::Symbols::RuleInstructionSymbol &outInst);

    static bool resolveOperand(class DiagnosticCollector *collector,
                               class SymbolTable *table,
                               const DSL::Ast::LegalizeRuleDef::RuleOperand &operand,
                               std::string_view ruleName,
                               bool isMatchPattern,
                               Sema::Symbols::RuleOperandSymbol &outOperand);
};

#endif // EZDSL_LEGALIZE_RULE_PASS_H