#include "AstNodes/Variable.h"

Variable::Variable(TypedPoolSlice<AstNode> *initializers,
                   std::string_view dataType,
                   std::string_view variableName) : m_variableDataType(dataType), m_variableName(variableName)
{
    m_isArray = initializers != nullptr && initializers->m_numElems > 1;
    AstNodeContainer::setExpressions(initializers);
}

AstNodeType Variable::getType() const { return AstNodeType::Variable; }

bool Variable::accept(AstNodeVisitor *visitor)
{
    if (visitor)
    {
        return visitor->visit(this);
    }
    return false;
}

bool Variable::getIsArray() const { return m_isArray; }

const char *Variable::getAstNodeName() const { return "Variable"; }

const std::string_view &Variable::getVariableDataType() const { return m_variableDataType; }

const std::string_view &Variable::getVariableName() const { return m_variableName; }
