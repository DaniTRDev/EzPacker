#include "Parser/Rules.h"

namespace Rules
{
std::shared_ptr<Rule> ConditionalManyOf(const std::shared_ptr<Rule> &condition, const std::shared_ptr<Rule> &rule)
{
    return Rule::create(
            [condition, rule](IParsingContext &ctx, const std::shared_ptr<AstNode> &out) -> bool
            {
                ctx.getErrorCollector()->enterScope();
                while (true)
                {
                    if (!condition->match(ctx, out))
                        break;

                    // Condition was matched; it's REQUIRED that rule is matched too.
                    if (!rule->match(ctx, out))
                    {
                        ctx.getErrorCollector()->error(LogMessage("Expected rule match after condition"),
                                                       ctx.peek().m_sourceReference);
                        ctx.getErrorCollector()->exitScope(ErrorHandleType::Propagate);
                        return false;
                    }
                }
                ctx.getErrorCollector()->exitScope(ErrorHandleType::Propagate);
                return true;
            });
}

std::shared_ptr<Rule> ManyOf(const std::shared_ptr<Rule> &rule)
{
    return Rule::create(
            [rule](IParsingContext &ctx, const std::shared_ptr<AstNode> &out) -> bool
            {
                ctx.getErrorCollector()->enterScope();
                while (true)
                {
                    if (!rule->match(ctx, out))
                        break;
                }
                ctx.getErrorCollector()->exitScope(ErrorHandleType::Propagate);
                return true;
            });
}

std::shared_ptr<Rule> MatchIf(const std::shared_ptr<Rule> &condition, const std::shared_ptr<Rule> &then)
{
    return Rule::create(
            [condition, then](IParsingContext &ctx, const std::shared_ptr<AstNode> &out) -> bool
            {
                ctx.getErrorCollector()->enterScope();
                if (condition->match(ctx, out))
                {
                    if (!then->match(ctx, out))
                    {
                        ctx.getErrorCollector()->exitScope(ErrorHandleType::Propagate);
                        return false;
                    }
                }
                ctx.getErrorCollector()->exitScope(ErrorHandleType::Discard);
                return true;
            });
}

std::shared_ptr<Rule> Optional(const std::shared_ptr<Rule> &rule)
{
    return Rule::create(
            [rule](IParsingContext &ctx, const std::shared_ptr<AstNode> &out) -> bool
            {
                ctx.getErrorCollector()->enterScope();
                if (rule->match(ctx, out))
                    ctx.getErrorCollector()->exitScope(ErrorHandleType::Discard);
                else
                    ctx.getErrorCollector()->exitScope(ErrorHandleType::Propagate);
                return true; // Always succeeds
            });
}

std::shared_ptr<Rule> StrictManyOf(const std::shared_ptr<Rule> &start, const std::shared_ptr<Rule> &rule)
{
    return Rule::create(
            [start, rule](IParsingContext &ctx, const std::shared_ptr<AstNode> &out) -> bool
            {
                ctx.getErrorCollector()->enterScope();
                while (true)
                {
                    ctx.getErrorCollector()->enterScope();
                    size_t pos = ctx.getPosition();
                    std::shared_ptr<AstNode> dummy = AstNodes::Null().build();

                    /*
                     * A dummy out node for the start condition is needed, so the real out is not bloated with stuff
                     * from the "start condition".
                     */

                    if (!start->match(ctx, dummy))
                    {
                        ctx.restore(pos);
                        break;
                    }

                    // Restore position so rule doesn't fail.
                    ctx.restore(pos);

                    // Start was matched, it's REQUIRED that rule is also matched too.
                    if (!rule->match(ctx, out))
                    {
                        ctx.getErrorCollector()->exitScope(ErrorHandleType::Propagate);
                        return false;
                    }

                    ctx.getErrorCollector()->exitScope(ErrorHandleType::Discard);
                }
                ctx.getErrorCollector()->exitScope(ErrorHandleType::Propagate);
                return true;
            });
}
} // namespace Rules
