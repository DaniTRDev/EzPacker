#include "AstNodes/Label.h"

Label::Label(std::string name) : m_name(std::move(name)) {}

AstNodeType Label::getType() const { return AstNodeType::Label; }

const char *Label::getAstNodeName() const { return "Label"; }

void Label::setCodeScope(const std::shared_ptr<CodeScope> &codeScope) {m_codeScope = codeScope;}

const std::shared_ptr<CodeScope> &Label::getCodeScope() const { return m_codeScope; }

std::string Label::getAsStr(AstNodeStringMode mode) const
{
    std::string res;
    res += std::format("@Label(name {}) {{ {} }}\n", getLabelName(), m_codeScope->getAsStr(mode));
    return std::move(res);
}

const std::string &Label::getLabelName() const { return m_name; }
