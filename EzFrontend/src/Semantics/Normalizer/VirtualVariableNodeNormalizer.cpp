#include "Semantics/Normalizer/VirtualVariableNormalizer.h"

std::shared_ptr<NormalizedVirtualVariable> VirtualVariableNormalizer::normalizeNode(
    std::shared_ptr<Ast> node, const std::shared_ptr<NormalizerContext> &context)
{
    if (!node || node->getType() != AstType::VirtualVariable)
    {
        // TODO: Given node is not a virtual variable node.
        return nullptr;
    }
    
    auto virtualVariableNode = Ast::cast<VirtualVariableNode>(node);
    auto symbol = virtualVariableNode->castChildTo<IdentifierNode>(0);

    if (!symbol)
    {
        // TODO: Given given variable name is invalid
        return nullptr;
    }
    
    std::shared_ptr<NormalizedVirtualVariable> virtualVariable = std::make_shared<NormalizedVirtualVariable>();
    virtualVariable->m_symbol = SymbolNormalizer().normalizeNode(symbol, context);
    virtualVariable->m_symbol->m_type = SymbolType::VirtualVariable; // Updated internal's symbol type.
    
    return virtualVariable;
}
