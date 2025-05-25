#include "parser/ParseRule.h"

ParseRule::~ParseRule()
{
    m_result.reset();
}

bool ParseRule::matchRet(class IParser &parser, const std::shared_ptr<Ast> &out)
{
    size_t position = parser.getPosition(); // Automatic restoration of the context if rule failed.
    /**
     * Since in some nodes there's a call to map while in others there isn't we need a way to distinguish what nodes
     * should be pushed as a child of out (call map<T>) and what nodes should push its children to out (combinators and
     * tokenType).
     */
    
    if (!m_mapped)
        m_result = std::make_shared<TokenTypeNode>();

    bool matched = m_matcher(parser, m_result);

    if (!matched)
    {
        parser.restore(position);
        m_result->clearChildren(); // Clear result.

        return false;
    }
    else if (m_callback)
    {
        matched = m_callback(parser, m_result);
    }

    if (!m_mapped)
    {
        // Result's children should be pushed to out.
        m_result->copyChildrenTo(out);
    }
    else
    {
        // Result should be pushed as a child of out (combinators).
        out->addChild(m_result->clone());
    }

    m_result->clearChildren(); // Clear result.
    return matched;
}

std::shared_ptr<ParseRule> ParseRule::setName(const std::string &name)
{
    m_name = name;
    return shared_from_this();
}

std::shared_ptr<ParseRule> ParseRule::then(const MatchedCallback &callback)
{
    m_callback = callback;
    return shared_from_this();
}

std::shared_ptr<ParseRule> ParseRule::create(const Matcher &matcher, std::string_view name)
{
    // Can't use std::make_shared because it calls the constructor internally. Since we made it private it's not
    // accessible. This is the only "good" way without going right into obscure C++ STL fix ups.
    return std::shared_ptr<ParseRule>(new ParseRule(matcher, name));
}

ParseRule::ParseRule() : m_mapped(false), m_callback(), m_matcher(), m_result()
{
}

ParseRule::ParseRule(const Matcher &matcher, std::string_view name)
    : m_mapped(false), m_callback(), m_matcher(matcher), m_name(name), m_result()
{
}
