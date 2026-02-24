#include "AstNodes/Module.h"

ModuleHeader::ModuleHeader(std::string moduleName,
                           std::string returnType,
                           std::vector<std::shared_ptr<AstNode>> parameters) :
    m_moduleName(std::move(moduleName)), m_returnType(std::move(returnType)), m_parameters(std::move(parameters))
{
}

AstNodeType ModuleHeader::getType() const { return AstNodeType::ModuleHeader; }

const char *ModuleHeader::getAstNodeName() const { return "ModuleHeaderParser"; }

std::string ModuleHeader::getAsStr(AstNodeStringMode mode) const
{
    std::string str;
    str = std::format("@Module(type: {} name: {}) {{\n", m_returnType, m_moduleName);

    if (mode == AstNodeStringMode::Debug)
    {
        std::string parametersContent;
        for (size_t i = 0; i < m_parameters.size(); i++)
        {
            parametersContent += std::format("\t{} = {}\n", i, m_parameters[i]->getAsStr(mode));
        }
    }

    str += "}\n";
    return std::move(str);
}

const std::string &ModuleHeader::getModuleName() const { return m_moduleName; }

const std::string &ModuleHeader::getReturnTypeName() const { return m_returnType; }

const std::vector<std::shared_ptr<AstNode>> &ModuleHeader::getParameters() const { return m_parameters; }

Module::Module(std::shared_ptr<CodeScope> body, std::shared_ptr<ModuleHeader> header) :
    m_body(std::move(body)), m_header(std::move(header))
{
}

AstNodeType Module::getType() const { return AstNodeType::Module; }

const char *Module::getAstNodeName() const { return "Module"; }

const std::shared_ptr<CodeScope> &Module::getBody() const { return m_body; }

const std::shared_ptr<ModuleHeader> &Module::getHeader() const { return m_header; }

std::string Module::getAsStr(AstNodeStringMode mode) const
{
    std::string res = m_header->getAsStr(mode);
    res += m_body->getAsStr(mode);
    return std::move(res);
}
