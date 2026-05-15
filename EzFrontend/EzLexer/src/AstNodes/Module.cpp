#include "AstNodes/Module.h"

ModuleHeader::ModuleHeader(TypedPoolLinkedList<AstNode> *parameters, std::string_view moduleName, std::string_view returnType) :
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