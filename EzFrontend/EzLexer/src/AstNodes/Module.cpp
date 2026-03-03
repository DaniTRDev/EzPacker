#include "AstNodes/Module.h"

ModuleHeader::ModuleHeader(TypedPoolSlice<AstNode> *parameters, std::string_view moduleName, std::string_view returnType) :
    m_moduleName(std::move(moduleName)), m_returnType(std::move(returnType))
{
    AstNodeContainer::setExpressions(parameters);
}

AstNodeType ModuleHeader::getType() const { return AstNodeType::ModuleHeader; }

bool ModuleHeader::accept(AstNodeVisitor *visitor)
{
    if (visitor)
    {
        return visitor->visit(this);
    }
    return false;
}

const char *ModuleHeader::getAstNodeName() const { return "ModuleHeader"; }

std::string ModuleHeader::getAsStr(AstNodeStringMode mode) const
{
    std::string str;
    str = std::format("@Module(type: {} name: {}) {{\n", m_returnType, m_moduleName);

    if (mode == AstNodeStringMode::Debug)
    {
        auto param = getExpressions()->m_head;
        size_t i = 0;
        std::string parametersContent;

        while (param)
        {
            AstNode *node = (AstNode *)param->m_object;
            parametersContent += std::format("\t{} = {}\n", i, node->getAsStr(mode));
            param = param->m_next;
            i++;
        }
    }

    str += "}\n";
    return std::move(str);
}

const std::string_view &ModuleHeader::getModuleName() const { return m_moduleName; }

const std::string_view &ModuleHeader::getReturnTypeName() const { return m_returnType; }

Module::Module(CodeScope *body, ModuleHeader *header) : m_body(std::move(body)), m_header(std::move(header)) {}

AstNodeType Module::getType() const { return AstNodeType::Module; }

bool Module::accept(AstNodeVisitor *visitor)
{
    if (visitor)
    {
        return visitor->visit(this);
    }
    return false;
}

CodeScope *Module::getBody() const { return m_body; }

const char *Module::getAstNodeName() const { return "Module"; }

ModuleHeader *Module::getHeader() const { return m_header; }

std::string Module::getAsStr(AstNodeStringMode mode) const
{
    std::string res = m_header->getAsStr(mode);
    res += m_body->getAsStr(mode);
    return std::move(res);
}
