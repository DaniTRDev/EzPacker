#ifndef EZPACKER_RULES_H
#define EZPACKER_RULES_H

#include "EzLexerCommon.h"
#include "AstNode/AstNodes.h"
#include "Rule.h"

/**
 * This file contains a set of predefined rules that are used by other parsing rules to generate the AST.
 * IMPORTANT!!: If a rule returns false, in most of the cases an error message will be pushed into the error collector.
 * This note is not in the documentation of the functions because it would be repetitive.
 */
namespace Rules
{
/**
 * Tries to match any of the given rules returning on first success. If there isn't a single match, false is returned.
 * @param rules
 * @tparam Args
 * @return std::shared_ptr<Rule>
 */
template <typename... Args> std::shared_ptr<Rule> AnyOf(Args &&...args)
{
    std::vector<std::shared_ptr<Rule>> vector = { std::forward<Args>(args)... }; // Create a vector from parameter pack.
    return Rule::create(
            [rules = std::move(vector)](IParsingContext &ctx, std::shared_ptr<AstNode> out) -> bool
            {
                ctx.getErrorCollector()->enterScope();
                for (auto &rule : rules)
                {
                    if (rule->match(ctx, out))
                    {
                        ctx.getErrorCollector()->exitScope(ErrorHandleType::Discard);
                        return true;
                    }
                }
                ctx.getErrorCollector()->exitScope(ErrorHandleType::Propagate);
                return false;
            });
}

/**
 * Acts like ManyOf and MatchIf. If condition is matched it will try to match the following rule, returning false if
 * it fails. This process is done until condition fails.
 * @param condition
 * @param rule
 * @return std::shared_ptr<Rule>
 */
extern std::shared_ptr<Rule> ConditionalManyOf(const std::shared_ptr<Rule> &condition,
                                               const std::shared_ptr<Rule> &rule);

/**
 * Tries to match a rule multiple times until it fails. It always returns true, no matter if there weren't any matches.
 * @param rule
 * @return std::shared_ptr<Rule>
 */
extern std::shared_ptr<Rule> ManyOf(const std::shared_ptr<Rule> &rule);

/**
 * Tries to match condition. If condition is not matched, it won't throw an error. If condition is matched,
 * it will try to match 'then'. If then fails, false is returned.
 * @param condition
 * @param then
 * @return std::shared_ptr<Rule>
 */
extern std::shared_ptr<Rule> MatchIf(const std::shared_ptr<Rule> &condition, const std::shared_ptr<Rule> &then);

/**
 * Tries to match the rule. Will return true ALWAYS.
 * @param rule
 * @return std::shared_ptr<Rule>
 */
extern std::shared_ptr<Rule> Optional(const std::shared_ptr<Rule> &rule);

/**
 * Tries to match a set of rules. If any fails, false will be returned.
 * @param rules
 * @tparam Args
 * @return std::shared_ptr<Rule>
 */
template <typename... Args> std::shared_ptr<Rule> Sequence(Args &&...args)
{
    std::vector<std::shared_ptr<Rule>> vector = { std::forward<Args>(args)... }; // Create list from parameter pack.
    return Rule::create(
            [rules = std::move(vector)](IParsingContext &ctx, const std::shared_ptr<AstNode> &out) -> bool
            {
                ctx.getErrorCollector()->enterScope();
                for (auto &rule : rules)
                {
                    if (!rule->match(ctx, out))
                    {
                        ctx.getErrorCollector()->exitScope(ErrorHandleType::Propagate);
                        return false;
                    }
                }
                ctx.getErrorCollector()->exitScope(ErrorHandleType::Discard);
                return true;
            });
}

/**
 * Acts like a ConditionalManyOf with some differences. If start is matched, the position (address) of the parser will
 * be restored to the address that was set before parsing start. Then it will try to match rule, returning false if it
 * fails. In any other case it returns true. 'start' should be the very first parsing rule that 'rule' expects to match.
 * @param start
 * @param rule
 * @return std::shared_ptr<Rule>
 */
extern std::shared_ptr<Rule> StrictManyOf(const std::shared_ptr<Rule> &start, const std::shared_ptr<Rule> &rule);
}; // namespace Rules

#endif // EZPACKER_RULES_H
