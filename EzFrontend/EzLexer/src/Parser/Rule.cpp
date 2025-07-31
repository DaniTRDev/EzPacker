#include "Parser/Rule.h"

bool Rule::match(class IParsingContext &ctx, const std::shared_ptr<AstNode> &out)
{
    size_t position = ctx.getPosition(); // Automatic restoration of the context if rule failed.
    const std::shared_ptr<AstNode> &ruleResult = m_builder->build(); // Dummy node.

    ctx.skipUselessTokens();

    /**
     * This line might be confusing but it just does this:
     * If matching fails, restore context position and clear current results. If matcher succeeds, it will call the
     * callback if it's set (not null). If callback was set and fails, produces the same result as when matching fails.
     */
    if ((!m_matcher(ctx, ruleResult)) || (m_callback && !m_callback(ctx, ruleResult)))
    {
        ctx.restore(position);
        ruleResult->clear(); // Clear result.
        return false;
    }

    if (m_encapsulate)
    {
        // If the result was encapsulated into its own node, push it as a child of out.
        out->addChild(std::move(ruleResult));
    }
    else
    {
        // If result is not encapsulated, we need to copy its children into out.
        ruleResult->copyChildrenTo(out);
    }

    return true;
}

std::shared_ptr<Rule> Rule::create(const Matcher &matcher)
{
    /*
     * Can't use std::make_shared because it calls the constructor internally. Since we made it private it's not
     * accessible.
     */
    return std::shared_ptr<Rule>(new Rule(matcher));
}

std::shared_ptr<Rule> Rule::encapsulate(AstNodeBuilder *builder)
{
    m_builder = builder;
    m_encapsulate = true;
    return shared_from_this();
}

std::shared_ptr<Rule> Rule::then(const MatchedCallback &callback)
{
    m_callback = callback;
    return shared_from_this();
}

std::shared_ptr<Rule> Rule::then(const std::shared_ptr<Rule> &rule)
{
    m_callback = [rule](IParsingContext &parser, const std::shared_ptr<AstNode> &out) -> bool
    { return rule->match(parser, out); };
    return shared_from_this();
}

Rule::Rule() : m_builder(&AstNodes::Null()), m_encapsulate(false), m_callback(), m_matcher() {}

Rule::Rule(const Matcher &matcher) :
    m_builder(&AstNodes::Null()), m_encapsulate(false), m_callback(), m_matcher(matcher)
{
}
