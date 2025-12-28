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

const std::string &ModuleHeader::getReturnType() const { return m_returnType; }

const std::vector<std::shared_ptr<AstNode>> &ModuleHeader::getParameters() const { return m_parameters; }

ModuleBody::ModuleBody(std::vector<ModuleBodyExpr> expressions) : m_expressions(std::move(expressions)) {};

AstNodeType ModuleBody::getType() const { return AstNodeType::ModuleBody; }

const char *ModuleBody::getAstNodeName() const { return "ModuleBodyParser"; }

const std::vector<ModuleBodyExpr> &ModuleBody::getExpressions() const { return m_expressions; }

std::string ModuleBody::getAsStr(AstNodeStringMode mode) const
{
    std::string res = "{\n";
    for (size_t i = 0; i < m_expressions.size(); i++)
    {
        auto &expression = m_expressions[i];
        std::shared_ptr<AstNode> node;

        if (holds_alternative<std::shared_ptr<Instruction>>(expression))
        {
            node = get<std::shared_ptr<Instruction>>(expression);
        }
        else if (holds_alternative<std::shared_ptr<Label>>(expression))
        {
            node = get<std::shared_ptr<Label>>(expression);
        }

        res += std::format("\t{} = {}\n", i, node->getAsStr(mode));
    }

    res += "}\n";
    return std::move(res);
}

Module::Module(std::shared_ptr<ModuleBody> body, std::shared_ptr<ModuleHeader> header) :
    m_body(std::move(body)), m_header(std::move(header))
{
}

AstNodeType Module::getType() const { return AstNodeType::Module; }

const char *Module::getAstNodeName() const { return "Module"; }

const std::shared_ptr<ModuleBody> &Module::getBody() const { return m_body; }

const std::shared_ptr<ModuleHeader> &Module::getHeader() const { return m_header; }

std::string Module::getAsStr(AstNodeStringMode mode) const
{
    std::string res = m_header->getAsStr(mode);
    res += m_body->getAsStr(mode);
    return std::move(res);
}
