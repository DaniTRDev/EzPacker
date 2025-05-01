#ifndef EZPACKER_IINSTRUCTIONLIFTER_H
#define EZPACKER_IINSTRUCTIONLIFTER_H

#include "Decoder/IDecodedOperand.h"
#include "Decoder/ParsedDecodedInstruction.h"
#include "EzLifterCommon.h"

struct InstructionLiftContext
{
    llvm::IRBuilder<> &m_builder;
    const std::shared_ptr<ParsedInstructionData> &m_data;
    std::unique_ptr<llvm::LLVMContext> &m_context;
    const std::vector<std::shared_ptr<IDecodedOperand>> &m_operands;
};

/**
 * This class will take as input an instruction (ParsedDecodedInstruction), with its operands, and will produce
 * the corresponding LLVM's IR (aka lift) into the given builder.
 */
class IInstructionLifter
{
  public:
    virtual ~IInstructionLifter() = default;

    /**
     * Lifts the instruction with the given context and returns true if succeeded.
     * @param func
     * @return bool
     */
    virtual bool liftFunction(const InstructionLiftContext &liftContext) = 0;
};

#endif // EZPACKER_IINSTRUCTIONLIFTER_H
