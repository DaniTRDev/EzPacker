#ifndef EZPACKER_NORMALIZEDINSTRUCTION_H
#define EZPACKER_NORMALIZEDINSTRUCTION_H

#include "EzFrontendCommon.h"
#include "Semantics/Normalizer/INormalizer.h"
#include "Semantics/Normalizer/OperandNormalizer.h"
#include "Semantics/Tables/InstructionTable.h"
#include "SourceManager/SourceManager.h"

struct NormalizedInstruction
{
    std::shared_ptr<InstructionEntry> m_instructionEntry;
    std::shared_ptr<SourceReference> m_sourceRef;
    std::vector<std::shared_ptr<NormalizedOperand>> m_operands;
};

class InstructionNormalizer : public INormalizer<NormalizedInstruction>
{
  public:
    /**
     * Tries to normalize the given instruction node. This function assumes given node is valid and its type is
     * AstType::Instruction.
     * @param node
     * @param context
     * @return std::shared_ptr<NormalizedMemoryNode>
     */
    std::shared_ptr<NormalizedInstruction> normalizeNode(std::shared_ptr<Ast> node,
                                                         const std::shared_ptr<NormalizerContext> &context) override;
};

#endif // EZPACKER_NORMALIZEDINSTRUCTION_H
