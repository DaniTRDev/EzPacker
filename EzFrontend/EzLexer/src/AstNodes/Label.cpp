#include "AstNodes/Label.h"

Label::Label(std::string_view name) : m_name(std::move(name)) {}

AstNodeType Label::getType() const { return AstNodeType::Label; }

bool Label::accept(AstNodeVisitor *visitor)
{
    if (visitor)
    {
        return visitor->visit(this);
    }
    return false;
}

CodeScope *Label::getCodeScope() const { return m_codeScope; }

const char *Label::getAstNodeName() const { return "Label"; }

void Label::setCodeScope(CodeScope *codeScope) { m_codeScope = codeScope; }

const std::string_view &Label::getLabelName() const { return m_name; }
