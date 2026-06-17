#include "PromoteScalarActionVerifier.h"

PromoteScalarActionVerifier::PromoteScalarActionVerifier(MirBuilderContext *ctx, PromoteScalarAction *action) :
    m_ctx(ctx), MirVerifier(action)
{
}

PromoteScalarActionVerifier &PromoteScalarActionVerifier::beginBlock(MirBlock *block)
{
    EXPECT_NE(block, nullptr);
    m_targetBlock = block;

    return *this;
}

PromoteScalarActionVerifier &PromoteScalarActionVerifier::verifyExtension(size_t index,
                                                                          MirInstructionOpCode opcode,
                                                                          MirType *origType,
                                                                          MirType *newType)
{
    auto &instructions = m_targetBlock->getInstructions();
    EXPECT_LT(index, instructions.size()) << "Instruction index out of bounds.";

    auto it = instructions.begin();
    std::advance(it, index);
    MirInstruction *instr = *it;

    auto verifier = MirInstructionVerifier(instr);
    verifier.opcode(opcode).operandCount(2);

    // Operand 0: Destination should have been promoted properly to the new type.
    verifier.operandVerifier(0).type(MirOperandType::Register).mirTypeVerifier().id(newType->getId());

    // Operand 1: It is the original variable.
    verifier.operandVerifier(1).type(MirOperandType::Register).mirTypeVerifier().id(origType->getId());

    return *this;
}

PromoteScalarActionVerifier &PromoteScalarActionVerifier::verifyExtensionTruncation(size_t index,
                                                                                    MirInstructionOpCode opcode,
                                                                                    MirType *origType,
                                                                                    MirType *newType)
{
    auto &instructions = m_targetBlock->getInstructions();
    EXPECT_LT(index, instructions.size()) << "Instruction index out of bounds.";

    auto it = instructions.begin();
    std::advance(it, index);
    MirInstruction *instr = *it;

    // Ensure it is structurally a TRUNC instruction
    auto verifier = MirInstructionVerifier(instr);
    verifier.opcode(MirInstructionOpCode::TRUNC).operandCount(2);

    // Operand 0: Destination register (it is the promoted legal register).
    verifier.operandVerifier(0).type(MirOperandType::Register).mirTypeVerifier().id(newType->getId());

    // Operand 1: How many bits to TRUNC, must match newType's bit size.
    verifier.operandVerifier(1).type(MirOperandType::Integer).verifyInteger(nullptr, newType->getTotalSizeInBits());

    return *this;
}
