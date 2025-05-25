#ifndef EZPACKER_NORMALIZEDOPERAND_H
#define EZPACKER_NORMALIZEDOPERAND_H

#include "EzFrontendCommon.h"
#include "MemoryNodeNormalizer.h"
#include "VirtualVariableNormalizer.h"

enum class NormalizedOperandType : uint8_t
{
    Invalid = 0,
    Memory,
    Value,
    VirtualVariable,
};

/**
 * Only 1 of them will be filled.
 */
struct NormalizedOperand
{
    NormalizedOperandType m_operandType;
    std::shared_ptr<NormalizedMemoryNode> m_memoryNode;
    std::shared_ptr<NormalizedValue> m_value;
    std::shared_ptr<NormalizedVirtualVariable> m_virtualVariable;
    std::shared_ptr<NormalizedType> m_dataType;
};

class OperandNormalizer : public INormalizer<NormalizedOperand>
{
  public:
    /**
     * Tries to normalize the given operand (a memory node or a virtual variable node). These operands are generated
     * without being tied to any type. Caller should be responsible of setting their appropriated data type. Assumes
     * node is valid and its type is AstType::Memory, AstType::Value or AstType::VirtualVariable.
     * @param node
     * @param context
     * @return std::shared_ptr<NormalizedOperand>
     */
    std::shared_ptr<NormalizedOperand> normalizeNode(std::shared_ptr<Ast> node,
                                                     const std::shared_ptr<NormalizerContext> &context) override;
};

#endif // EZPACKER_NORMALIZEDOPERAND_H
