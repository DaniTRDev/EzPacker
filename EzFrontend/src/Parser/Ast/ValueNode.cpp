#include "parser/ast/ValueNode.h"

ValueNode::ValueNode(ValueNodeType valueType) : m_valueType(valueType)
{
    setType(AstType::Value);
}

ValueNodeType ValueNode::getValueType()
{
    return m_valueType;
}

void ValueNode::setValueType(ValueNodeType type)
{
    m_valueType = type;
}

std::shared_ptr<Ast> ValueNode::clone() const
{
    return std::make_shared<ValueNode>(*this);
}