#include "parser/grammar/Combinators.h"

namespace grammar::combinators
{
std::shared_ptr<ParseRule> anyOf(const std::vector<std::shared_ptr<ParseRule>> &rules)
{
    return ParseRule::create(
        [rules](IParser &parser, const std::shared_ptr<Ast> &out) -> bool {
            parser.getErrorCollector()->enterRule();
            for (auto &rule : rules)
            {
                parser.skipWhiteSpacesAndNewLines();

                std::shared_ptr<Ast> intermediateOut = std::make_shared<TokenTypeNode>();
                if (!rule->matchRet(parser, intermediateOut))
                    continue;

                intermediateOut->copyChildrenTo(out); // Add children only if we matched at least 1 rule.
                parser.getErrorCollector()->exitRule(ErrorHandleType::Discard);
                return true;
            }
            parser.getErrorCollector()->collect(LogMessage("").add("Malformed unknown expression"),
                                                parser.peek().m_sourceReference);
            parser.getErrorCollector()->exitRule(ErrorHandleType::Propagate);
            return false;
        },
        "anyOf");
}

std::shared_ptr<ParseRule> manyOf(size_t min, size_t max, const std::shared_ptr<ParseRule> &rule)
{
    return ParseRule::create(
        [min, max, rule](IParser &parser, const std::shared_ptr<Ast> &out) -> bool {
            size_t matchCount = 0;
            std::shared_ptr<TokenTypeNode> intermediateOut = std::make_shared<TokenTypeNode>();

            parser.getErrorCollector()->enterRule();
            parser.skipWhiteSpacesAndNewLines();

            while (true)
            {
                parser.getErrorCollector()->enterRule();

                if (!rule->matchRet(parser, intermediateOut))
                {
                    parser.getErrorCollector()->exitRule(ErrorHandleType::Propagate);
                    break;
                }

                matchCount++;
                parser.getErrorCollector()->exitRule(ErrorHandleType::Discard);
                parser.skipWhiteSpacesAndNewLines();
            }

            if (matchCount < min || matchCount > max)
            {
                parser.getErrorCollector()->collect(
                    LogMessage("").add("Expected at least {} items, maximum {} but got {}", min, max, matchCount),
                    parser.peek().m_sourceReference);

                parser.getErrorCollector()->exitRule(ErrorHandleType::Propagate);
                return false;
            }

            parser.getErrorCollector()->exitRule(ErrorHandleType::Discard);
            intermediateOut->copyChildrenTo(out); // Only add children if matchCount is withing the provided range.
            return true;
        },
        "manyOf");
}

std::shared_ptr<ParseRule> matchIf(std::shared_ptr<ParseRule> ifRule, std::shared_ptr<ParseRule> nextRule)
{
    return ParseRule::create(
        [ifRule, nextRule](IParser &parser, const std::shared_ptr<Ast> &out) -> bool {
            parser.getErrorCollector()->enterRule();
            parser.skipWhiteSpacesAndNewLines();
            std::shared_ptr<Ast> intermediateOut = std::make_shared<TokenTypeNode>();
            if (ifRule->matchRet(parser, intermediateOut))
            {
                parser.skipWhiteSpacesAndNewLines();
                if (!nextRule->matchRet(parser, intermediateOut))
                {
                    parser.getErrorCollector()->exitRule(ErrorHandleType::Propagate);
                    return false;
                }
            }
            parser.getErrorCollector()->exitRule(ErrorHandleType::Discard);

            intermediateOut->copyChildrenTo(out);
            return true;
        },
        "matchIf");
}

std::shared_ptr<ParseRule> optional(std::shared_ptr<ParseRule> rule)
{
    return ParseRule::create(
        [rule](IParser &parser, const std::shared_ptr<Ast> &out) -> bool {
            parser.getErrorCollector()->enterRule();
            parser.skipWhiteSpacesAndNewLines();
            rule->matchRet(parser, out);
            parser.getErrorCollector()->exitRule(ErrorHandleType::Discard);
            return true;
        },
        "optional");
}

std::shared_ptr<ParseRule> sequence(const std::vector<std::shared_ptr<ParseRule>> &rules)
{
    return ParseRule::create(
        [rules](IParser &parser, const std::shared_ptr<Ast> &out) -> bool {
            parser.getErrorCollector()->enterRule();

            std::shared_ptr<TokenTypeNode> intermediateOut = std::make_shared<TokenTypeNode>();
            for (auto &rule : rules)
            {
                parser.skipWhiteSpacesAndNewLines();

                if (!rule->matchRet(parser, intermediateOut))
                {
                    parser.getErrorCollector()->exitRule(ErrorHandleType::Propagate);
                    return false;
                }
            }

            // Only copy results if succeeded.
            for (auto &child : intermediateOut->getChildren())
                out->addChild(child);

            parser.getErrorCollector()->exitRule(ErrorHandleType::Discard);
            return true;
        },
        "sequence");
}

} // namespace grammar::combinators
