#include "Semantics/Normalizer/VirtualVariableNormalizer.h"

std::shared_ptr<NormalizedVirtualVariable> VirtualVariableNormalizer::normalizeNode(
    std::shared_ptr<Ast> node, const std::shared_ptr<NormalizerContext> &context)
{
    auto virtualVariableNode = Ast::cast<VirtualVariableNode>(node);
    auto symbol = virtualVariableNode->castChildTo<IdentifierNode>(0);

    if (!symbol)
    {
        context->m_logger->logError(LogMessage("").add("Given variable name is invalid"));
        return nullptr;
    }

    std::shared_ptr<NormalizedVirtualVariable> virtualVariable = std::make_shared<NormalizedVirtualVariable>();
    virtualVariable->m_symbol = SymbolNormalizer().normalizeNode(symbol, context);
    virtualVariable->m_symbol->m_symbolEntry->m_type = SymbolType::VirtualVariable; // Update internal symbol type.

    return std::move(virtualVariable);
}
