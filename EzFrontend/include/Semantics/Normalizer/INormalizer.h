#ifndef EZPACKER_INORMALIZER_H
#define EZPACKER_INORMALIZER_H

#include "EzFrontendCommon.h"
#include "Number/IBigNumber.h"
#include "Parser/Ast/Ast.h"
#include "Semantics/Tables/SymbolTable.h"
#include "Semantics/Tables/TypeTable.h"

/**
 * Structure used to be able to pass information to normalizers.
 */
struct NormalizerContext
{
    std::shared_ptr<SymbolTable> m_symbolTable;
    std::shared_ptr<TypeTable> m_typeTable;
};

template <typename NormalizedResult> class INormalizer
{
  public:
    /**
     * Tries to normalize given node. If failed it will return nullptr.
     * @param node
     * @param context
     * @return std::shared_ptr<NormalizedResult>
     */
    virtual std::shared_ptr<NormalizedResult> normalizeNode(std::shared_ptr<Ast> node,
                                                            const std::shared_ptr<NormalizerContext> &context) = 0;
};

#endif // EZPACKER_INORMALIZER_H
