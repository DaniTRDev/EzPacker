#include "Semantics/Normalizer/ModuleNormalizer.h"

std::shared_ptr<NormalizedModule> ModuleNormalizer::normalizeNode(std::shared_ptr<Ast> node,
                                                                  const std::shared_ptr<NormalizerContext> &context)
{
    size_t bodyStart = 0, bodyEnd = 0;
    std::shared_ptr<NormalizedModule> normalizedModule = std::make_shared<NormalizedModule>();

    if (!normalizeHeader(bodyStart, node, normalizedModule, context))
    {
        context->m_logger->logError(LogMessage("").add("Error in module header"),
                                    node->getSourceRef());
        return nullptr;
    }

    if (!normalizeBody(bodyEnd, bodyStart, node, normalizedModule, context))
    {
        context->m_logger->logError(LogMessage("").add("Error in module body"),
                                    node->getSourceRef());
        return nullptr;
    }

    std::shared_ptr<IdentifierNode> moduleStartIdentifier = node->castChildTo<IdentifierNode>(bodyEnd);
    std::shared_ptr<TokenTypeNode> moduleEndToken = moduleStartIdentifier->castChildTo<TokenTypeNode>(0);
    std::string moduleEndStr = moduleEndToken->getContent();

    if (moduleEndStr != "end")
    {
        context->m_logger->logError(LogMessage("").add("Invalid module end declaration"),
                                    moduleEndToken->getSourceRef());
        return nullptr;
    }

    return std::move(normalizedModule);
}

bool ModuleNormalizer::normalizeHeader(size_t &bodyStart, std::shared_ptr<Ast> node,
                                       std::shared_ptr<NormalizedModule> module,
                                       const std::shared_ptr<NormalizerContext> &context)
{
    std::shared_ptr<ModuleNode> moduleNode = Ast::cast<ModuleNode>(node);
    auto &children = moduleNode->getChildren();
    constexpr size_t minimumModuleNodes = 1 + 1 + 1; // .module, name, .end

    if (children.size() < minimumModuleNodes)
    {
        context->m_logger->logError(LogMessage("").add("Invalid module declaration"), node->getSourceRef());
        return false;
    }

    std::shared_ptr<IdentifierNode> moduleStartIdentifier = moduleNode->castChildTo<IdentifierNode>(0);
    std::shared_ptr<TokenTypeNode> moduleStartToken = moduleStartIdentifier->castChildTo<TokenTypeNode>(0);
    std::string moduleStartStr = moduleStartToken->getContent();

    if (moduleStartStr != "module")
    {
        context->m_logger->logError(LogMessage("").add("Invalid module start declaration"),
                                    moduleStartToken->getSourceRef());
        return false;
    }

    std::shared_ptr<NormalizedSymbol> moduleSymbol = SymbolNormalizer().normalizeNode(children[1], context);
    if (!moduleSymbol)
    {
        context->m_logger->logError(LogMessage("").add("Invalid module name"), children[1]->getSourceRef());
        return false;
    }

    moduleSymbol->m_symbolEntry->m_type = SymbolType::Module;

    bool expectedParameter = false;
    std::shared_ptr<NormalizedType> parameterType;
    std::shared_ptr<NormalizedVirtualVariable> parameter;
    std::vector<std::shared_ptr<NormalizedVirtualVariable>> parameters;
    for (size_t i = 2; i < children.size() - 1; i++)
    {
        auto &child = children[i];
        if (child->getType() != AstType::ModuleParameter)
        {
            // We finished parsing parameters.
            bodyStart = i;
            break;
        }

        parameterType = TypeNormalizer().normalizeNode(child->getChildren()[0], context);
        if (!parameterType)
        {
            context->m_logger->logError(LogMessage("").add("Could not identify parameter's type"),
                                        child->getSourceRef());
            return false;
        }
        expectedParameter = true;
        parameter = VirtualVariableNormalizer().normalizeNode(child->getChildren()[1], context);
        if (!parameter)
        {
            context->m_logger->logError(LogMessage("").add("Could not identify parameter definition"),
                                        child->getSourceRef());
            return false;
        }
        expectedParameter = false;
        parameter->m_type = std::move(parameterType);
        parameters.push_back(std::move(parameter));
    }

    if (expectedParameter)
    {
        context->m_logger->logError(LogMessage("").add("Expected parameter after type"),
                                    children[children.size() - 1]->getSourceRef());
        return false;
    }

    module->m_symbol = std::move(moduleSymbol);
    module->m_parameters = std::move(parameters);
    return true;
}

bool ModuleNormalizer::normalizeBody(size_t &bodyEnd, size_t bodyStart, std::shared_ptr<Ast> node,
                                     std::shared_ptr<NormalizedModule> module,
                                     const std::shared_ptr<NormalizerContext> &context)
{
    std::shared_ptr<ModuleNode> moduleNode = Ast::cast<ModuleNode>(node);
    auto &children = moduleNode->getChildren();

    if (bodyStart >= children.size())
    {
        context->m_logger->logError(LogMessage("").add("Invalid module body"), moduleNode->getSourceRef());
        return false;
    }

    bodyEnd = bodyStart;
    std::vector<std::shared_ptr<NormalizedInstruction>> instructions;
    for (size_t i = bodyStart; i < children.size(); i++)
    {
        auto &child = children[i];
        if (child->getType() != AstType::Instruction)
        {
            bodyEnd = i;
            break;
        }

        std::shared_ptr<NormalizedInstruction> instr = InstructionNormalizer().normalizeNode(child, context);
        if (!instr)
        {
            context->m_logger->logError(LogMessage("").add("Invalid minstruction"), child->getSourceRef());
            return false;
        }

        instructions.push_back(std::move(instr));
    }

    module->m_instructions = std::move(instructions);
    return true;
}
