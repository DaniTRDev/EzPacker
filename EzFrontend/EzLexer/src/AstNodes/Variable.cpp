#include "AstNodes/Variable.h"

Variable::Variable(bool isArray,
                   std::string dataType,
                   std::string variableName,
                   std::vector<std::shared_ptr<AstNode>> initializers) :
    m_isArray(isArray), m_variableDataType(dataType), m_variableName(variableName),
    m_initializers(std::move(initializers))
{
}

AstNodeType Variable::getType() const { return AstNodeType::Variable; }

bool Variable::getIsArray() const { return m_isArray; }

bool Variable::getIsLocal() const { return m_isLocal; }

const char *Variable::getAstNodeName() const { return "Variable"; }

std::string Variable::getAsStr(AstNodeStringMode mode) const
{
    std::string str = std::format("@Variable(type: {} name: {} isLocal: {} isArray: {}) {{\n",
                                  getVariableDataType(),
                                  getVariableName(),
                                  getIsLocal(),
                                  getIsArray());

    if (mode == AstNodeStringMode::Debug)
    {
        for (size_t i = 0; i < m_initializers.size(); i++)
        {
            auto &initializer = m_initializers[i];
            str += std::format("\t{}\n", initializer->getAsStr(mode));
        }
        str += "}\n";
        return std::move(str);
    }

    str += "}\n";
    return std::move(str);
}

const std::string &Variable::getVariableDataType() const { return m_variableDataType; }

const std::string &Variable::getVariableName() const { return m_variableName; }

const std::vector<std::shared_ptr<AstNode>> &Variable::getInitializers() const { return m_initializers; }
