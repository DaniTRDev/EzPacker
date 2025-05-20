#include "grammar/Combinators.h"

namespace grammar::combinators
{
std::shared_ptr<ParseRule> anyOf(const std::vector<std::shared_ptr<ParseRule>> &rules)
{
    return ParseRule::create(
        [rules](IParser &parser, const std::shared_ptr<Ast> &out) -> bool {
            for (auto &rule : rules)
            {
                parser.beginLogBlock();
                parser.skipWhiteSpacesAndNewLines();

                std::shared_ptr<Ast> intermediateOut = std::make_shared<TokenTypeNode>();
                if (!rule->matchRet(parser, intermediateOut))
                    continue;

                intermediateOut->copyChildrenTo(out); // Add children only if we matched at least 1 rule.

                parser.endLogBlock(false);
                return true;
            }

            parser.logParserError(LogMessage("").add("Malformed unknown expression"), parser.peek());
            return false;
        },
        "anyOf");
}

std::shared_ptr<ParseRule> manyOf(size_t min, size_t max, const std::shared_ptr<ParseRule> &rule)
{
    return ParseRule::create(
        [min, max, rule](IParser &parser, const std::shared_ptr<Ast> &out) -> bool {
            parser.beginLogBlock(); // Start a block
            size_t matchCount = 0;
            std::shared_ptr<TokenTypeNode> intermediateOut = std::make_shared<TokenTypeNode>();

            parser.skipWhiteSpacesAndNewLines();
            while (rule->matchRet(parser, intermediateOut))
            {
                matchCount++;
                parser.skipWhiteSpacesAndNewLines();
            }

            parser.endLogBlock(false); // We can't know for sure what caused it, so we don't log errors.

            if (matchCount < min || matchCount > max)
            {
                parser.beginLogBlock();
                parser.logParserError(
                    LogMessage("").add("Expected at least {} items, maximum {} but got {}", min, max, matchCount),
                    parser.peek());
                parser.endLogBlock(true);

                return false;
            }

            intermediateOut->copyChildrenTo(out); // Only add children if matchCount is withing the provided range.
            return true;
        },
        "manyOf");
}

std::shared_ptr<ParseRule> matchIf(std::shared_ptr<ParseRule> ifRule, std::shared_ptr<ParseRule> nextRule)
{
    return ParseRule::create(
        [ifRule, nextRule](IParser &parser, const std::shared_ptr<Ast> &out) -> bool {
            parser.beginLogBlock();
            parser.skipWhiteSpacesAndNewLines();

            std::shared_ptr<Ast> intermediateOut = std::make_shared<TokenTypeNode>();
            if (ifRule->matchRet(parser, intermediateOut))
            {
                parser.skipWhiteSpacesAndNewLines();
                if (!nextRule->matchRet(parser, intermediateOut))
                {
                    parser.endLogBlock(true); // We need to emit the error, so we know why this failed.
                    return false;
                }
            }

            intermediateOut->copyChildrenTo(out);
            parser.endLogBlock(false); // Since it is optional, we don't want to log errors.
            return true;
        },
        "matchIf");
}

std::shared_ptr<ParseRule> optional(std::shared_ptr<ParseRule> rule)
{
    return ParseRule::create(
        [rule](IParser &parser, const std::shared_ptr<Ast> &out) -> bool {
            parser.beginLogBlock();
            parser.skipWhiteSpacesAndNewLines();

            rule->matchRet(parser, out);

            parser.endLogBlock(false); // Since it is optional, we don't want to log errors.
            return true;
        },
        "optional");
}

std::shared_ptr<ParseRule> sequence(const std::vector<std::shared_ptr<ParseRule>> &rules)
{
    return ParseRule::create(
        [rules](IParser &parser, const std::shared_ptr<Ast> &out) -> bool {
            std::shared_ptr<TokenTypeNode> intermediateOut = std::make_shared<TokenTypeNode>();
            for (auto &rule : rules)
            {
                parser.skipWhiteSpacesAndNewLines();

                if (!rule->matchRet(parser, intermediateOut))
                    return false;
            }

            // Only copy results if succeeded.
            for (auto &child : intermediateOut->getChildren())
                out->addChild(child);

            return true;
        },
        "secuence");
}

} // namespace grammar::combinators
