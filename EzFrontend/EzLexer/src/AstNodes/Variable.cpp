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

std::string Variable::getAsStr(AstNodeStringMode mode) const
{
    std::string str = std::format("@Variable(type: {} name: {} isArray: {}) {{\n",
                                  getVariableDataType(),
                                  getVariableName(),
                                  getIsArray());

    if (mode == AstNodeStringMode::Debug)
    {
        for (const void *obj : *getExpressions())
        {
            AstNode *node = (AstNode *)obj;
            str += std::format("\t{}\n", node->getAsStr(mode));
        }
        str += "}\n";
        return std::move(str);
    }

    str += "}\n";
    return std::move(str);
}

const std::string_view &Variable::getVariableDataType() const { return m_variableDataType; }

const std::string_view &Variable::getVariableName() const { return m_variableName; }
