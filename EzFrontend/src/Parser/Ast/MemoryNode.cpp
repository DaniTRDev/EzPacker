#include "parser/ast/MemoryNode.h"

MemoryNode::MemoryNode(IRMemoryReferenceType type) : m_refType(type)
{
    setType(AstType::Memory);
}

IRMemoryReferenceType MemoryNode::getMemRefType()
{
    return m_refType;
}

void MemoryNode::setMemRefType(IRMemoryReferenceType type)
{
    m_refType = type;
}

std::shared_ptr<Ast> MemoryNode::clone() const
{
    return std::make_shared<MemoryNode>(*this);
}