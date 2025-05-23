#ifndef EZPACKER_COMBINATORS_H
#define EZPACKER_COMBINATORS_H

#include "EzFrontendCommon.h"
#include "parser/ParseRule.h"
#include "parser/ast/TokenTypeNode.h"

namespace grammar::combinators
{
/**
 * Creates a rule that will try to match every rule inside rules until one returns != nullptr. At least 1 must
 * match.
 * @param rules
 * @return std::shared_ptr<ParseRule>
 */
extern std::shared_ptr<ParseRule> anyOf(const std::vector<std::shared_ptr<ParseRule>> &rules);

/**
 * Tries to parse many elements. If less than minimum are found, it returns nullptr. If more than maximum is found,
 * it returns nullptr.
 * @param rule
 * @return std::shared_ptr<ParseRule>
 */
extern std::shared_ptr<ParseRule> manyOf(size_t min, size_t max, const std::shared_ptr<ParseRule> &rule);

/**
 * Creates a rule that tries to match rule, if it is matched, nextRule will also be checked. If rule is not matched,
 * nextRule won't be checked neither.
 * @param ifRule
 * @param nextRule
 * @return std::shared_ptr<ParseRule>
 */
extern std::shared_ptr<ParseRule> matchIf(std::shared_ptr<ParseRule> ifRule, std::shared_ptr<ParseRule> nextRule);

/**
 * Creates a rule that tries to match given rule. If it couldn't be matched, it will return nullptr. It is up to
 * the caller to log / check if this nullptr is wanted or not.
 * @param rule
 * @return std::shared_ptr<ParseRule>
 */
extern std::shared_ptr<ParseRule> optional(std::shared_ptr<ParseRule> rule);

/**
 * Tries to match the given sequence, following the given order, and returns != nullptr if succeeded.
 * @param rules
 * @return std::shared_ptr<ParseRule>
 */
extern std::shared_ptr<ParseRule> sequence(const std::vector<std::shared_ptr<ParseRule>> &rules);
} // namespace Grammar::combinators

#endif // EZPACKER_COMBINATORS_H
