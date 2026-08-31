#ifndef EZDSLSEMA_LEGALIZE_RULE_PASS_H
#define EZDSLSEMA_LEGALIZE_RULE_PASS_H

#include "EzDslSemaCommon.h"
#include "Ast/LegalizeRuleDefLangAst.h"
#include "Ast/IrInstructionDefLangAst.h"

/**
 * Forward declarations.
 */
namespace Symbols
{
class LegalizeRuleInstructionSymbol;
class LegalizeRuleOperandSymbol;
}; // namespace Symbols

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
                    DSL::Ast::LegalizeRuleDef::LegalizeRuleFile *file);

  private:
    /**
     * Processes a single rewrite rule declaration.
     */
    static bool processRule(class DiagnosticCollector *collector,
                            class SymbolTable *table,
                            const DSL::Ast::LegalizeRuleDef::LegalizeRule &rule);

    static bool processPredicate(DiagnosticCollector *collector,
                                 SymbolTable *table,
                                 const DSL::Ast::LegalizeRuleDef::RuleWhen &predicate,
                                 std::string_view ruleName);

    /**
     * Validates semantic guard predicates and verifies that variable arguments were defined in the match block.
     */
    static bool processInstruction(DiagnosticCollector *collector,
                                   SymbolTable *table,
                                   const DSL::Ast::LegalizeRuleDef::RuleInstruction &inst,
                                   std::string_view ruleName,
                                   bool isMatchPattern,
                                   Symbols::LegalizeRuleInstructionSymbol &outInst);

    /**
     * Resolves rule operands (SSA variables, constants, type prefixes, custom transforms).
     */
    static bool resolveOperand(class DiagnosticCollector *collector,
                               class SymbolTable *table,
                               const DSL::Ast::LegalizeRuleDef::RuleInstructionOperand &operand,
                               std::string_view ruleName,
                               bool isMatchPattern,
                               Symbols::LegalizeRuleOperandSymbol &outOperand);
};

#endif // EZDSLSEMA_LEGALIZE_RULE_PASS_H