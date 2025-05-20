#include "ast/IdentifierNode.h"

IdentifierNode::IdentifierNode()
{
    setType(AstType::Identifier);
}

void IdentifierNode::setIdentifier(const std::string &identifier)
{
    m_identifier = identifier;
}

const std::string &IdentifierNode::getIdentifier()
{
    return m_identifier;
}

std::shared_ptr<Ast> IdentifierNode::clone() const
{
    return std::make_shared<IdentifierNode>(*this);
}