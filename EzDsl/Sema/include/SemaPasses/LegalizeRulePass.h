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

/**
 * Semantic analysis pass validating IR rewrite rules (.lrd).
 * Checks SSA variable scoping between match and expand templates, resolves types and immediate constants,
 * and validates semantic guard predicate arguments.
 */
class LegalizeRulePass
{
  public:
    /**
     * Executes the rewrite rule semantic analysis and symbol resolution pass.
     * Returns true if all rules, match patterns, predicates, and expansion sequences were successfully resolved.
     */
    static bool run(class DiagnosticCollector *collector,
                    class SymbolTable *table,
                    DSL::Ast::LegalizeRuleDef::TargetLegalizeRuleDef *file);

  private:
    /**
     * Processes a single rewrite rule declaration.
     */
    static bool processRule(class DiagnosticCollector *collector,
                            class SymbolTable *table,
                            const DSL::Ast::LegalizeRuleDef::LegalizeRewriteRule &rule);

    static bool processPredicate(DiagnosticCollector *collector,
                                 SymbolTable *table,
                                 const DSL::Ast::LegalizeRuleDef::RulePredicate &predicate,
                                 std::string_view ruleName);

    /**
     * Validates semantic guard predicates and verifies that variable arguments were defined in the match block.
     */
    static bool processInstruction(DiagnosticCollector *collector,
                                   SymbolTable *table,
                                   const DSL::Ast::LegalizeRuleDef::RuleInstruction &inst,
                                   std::string_view ruleName,
                                   bool isMatchPattern,
                                   Sema::Symbols::RuleInstructionSymbol &outInst);

    /**
     * Resolves rule operands (SSA variables, constants, type prefixes, custom transforms).
     */
    static bool resolveOperand(class DiagnosticCollector *collector,
                               class SymbolTable *table,
                               const DSL::Ast::LegalizeRuleDef::RuleOperand &operand,
                               std::string_view ruleName,
                               bool isMatchPattern,
                               Sema::Symbols::RuleOperandSymbol &outOperand);
};

#endif // EZDSL_LEGALIZE_RULE_PASS_H