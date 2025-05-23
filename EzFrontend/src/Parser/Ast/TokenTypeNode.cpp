#include "parser/ast/TokenTypeNode.h"

TokenTypeNode::TokenTypeNode()
{
    setType(AstType::TokenTypeNode);
}

const std::string &TokenTypeNode::getContent()
{
    return m_content;
}

void TokenTypeNode::setContent(const std::string &content)
{
    m_content = content;
}

void TokenTypeNode::setTokenType(IRTokenType type)
{
    m_tokenType = type;
}

IRTokenType TokenTypeNode::getTokenType() const
{
    return m_tokenType;
}

std::shared_ptr<Ast> TokenTypeNode::clone() const
{
    return std::make_shared<TokenTypeNode>(*this);
}
