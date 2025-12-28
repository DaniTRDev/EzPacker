#include "AstNodes/Label.h"

Label::Label(std::string name, std::vector<LabelExpression> expressions) :
    m_name(std::move(name)), m_expressions(std::move(expressions))
{
}

AstNodeType Label::getType() const { return AstNodeType::Label; }

const char *Label::getAstNodeName() const { return "Label"; }

std::string Label::getAsStr(AstNodeStringMode mode) const
{
    std::string res;
    res += std::format("@Label(name {}) {{\n", getLabelName());

    for (size_t i = 0; i < m_expressions.size(); i++)
    {
        auto &expression = m_expressions[i];
        std::shared_ptr<AstNode> node;

        if (holds_alternative<std::shared_ptr<Instruction>>(expression))
        {
            node = std::dynamic_pointer_cast<AstNode>(std::get<std::shared_ptr<Instruction>>(expression));
        }
        else if (holds_alternative<std::shared_ptr<Label>>(expression))
        {
            node = std::dynamic_pointer_cast<AstNode>(std::get<std::shared_ptr<Label>>(expression));
        }

        res += std::format("\t{} = {}\n", i, node->getAsStr(mode));
    }

    res += "}\n";
    return std::move(res);
}

const std::string &Label::getLabelName() const { return m_name; }

const std::vector<LabelExpression> &Label::getExpressions() const { return m_expressions; }
