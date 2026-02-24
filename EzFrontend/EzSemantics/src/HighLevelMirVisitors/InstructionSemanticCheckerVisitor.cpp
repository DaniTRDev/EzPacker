#include "HighLevelMirVisitors/InstructionSemanticCheckerVisitor.h"

// --- Module & Block Traversal ---

bool InstructionSemanticCheckerVisitor::visit(const class HighLevelMirModule &module)
{
    return visit((const HighLevelMirBlock &)module);
}

bool InstructionSemanticCheckerVisitor::visit(const class HighLevelMirBlock &block)
{
    for (auto &instruction : block.m_instructions)
    {
        if (!visit(instruction))
        {
            // We don't necessarily need to emit a generic error here because the
            // inner visit() or expectOperandsType() will have emitted a highly specific one.
            return false;
        }
    }

    return visitScopeBody(block);
}

bool InstructionSemanticCheckerVisitor::visit(const class HighLevelMirInstruction &instruction)
{
    // Fetch the metadata and operands
    const HighLevelMirMetadata &meta = getMeta(instruction.getOpCode());
    const std::vector<HighLevelMirInstructionOperand> &operands = instruction.getOperands();

    // Verify constraints
    if (!expectOperandsType(instruction, meta.m_flags, operands))
    {
        return false;
    }

    return true;
}

bool InstructionSemanticCheckerVisitor::visitScopeBody(const HighLevelMirBlock &block)
{
    // It should be 'while (currentBlock)', not 'while (!currentBlock)'.
    std::shared_ptr<HighLevelMirBlock> currentBlock = block.m_next;

    while (currentBlock)
    {
        if (!visit(*currentBlock))
        {
            return false;
        }
        currentBlock = currentBlock->m_next;
    }
    return true;
}

bool InstructionSemanticCheckerVisitor::expectOperandsType(const HighLevelMirInstruction &instruction,
                                                           uint32_t instrFlags,
                                                           const std::vector<HighLevelMirInstructionOperand> &operands)
{
    // Grab the source reference once for error reporting
    const std::shared_ptr<SourceReference> &sourceRef = instruction.getSourceRefs().front();
    const std::string contextLoc = "InstructionSemanticCheckerVisitor::expectOperandsType";

    // Check Operand 1 (dest / target)
    if (operands.size() > 0)
    {
        const auto &op1 = operands[0];
        HighLevelMirOperandType type1 = op1.getType();

        if ((instrFlags & Op1_MustBeReg) && type1 != HighLevelMirOperandType::Register)
        {
            getSemanticContext()->emitError(
                    ErrorSeverity::Fatal,
                    std::format("Instruction operand type mismatch: Expected Register for Operand 1 but got {}",
                                g_HighLevelMirOperandType2Str[type1]),
                    contextLoc,
                    sourceRef);
            return false;
        }

        if ((instrFlags & Op1_MustBeMem) && type1 != HighLevelMirOperandType::Memory)
        {
            getSemanticContext()->emitError(
                    ErrorSeverity::Fatal,
                    std::format("Instruction operand type mismatch: Expected Memory for Operand 1 but got {}",
                                g_HighLevelMirOperandType2Str[type1]),
                    contextLoc,
                    sourceRef);
            return false;
        }
    }

    if (operands.size() != 2)
    {
        return true;
    }

    // Check Operand 2 (src)
    const auto &op2 = operands[1];
    HighLevelMirOperandType type2 = op2.getType();

    if ((instrFlags & Op2_MustBeReg) && type2 != HighLevelMirOperandType::Register)
    {
        getSemanticContext()->emitError(
                ErrorSeverity::Fatal,
                std::format("Instruction operand type mismatch: Expected Register for Operand 2 but got {}",
                            g_HighLevelMirOperandType2Str[type2]),
                contextLoc,
                sourceRef);
        return false;
    }

    if ((instrFlags & Op2_MustBeMem) && type2 != HighLevelMirOperandType::Memory)
    {
        getSemanticContext()->emitError(
                ErrorSeverity::Fatal,
                std::format("Instruction operand type mismatch: Expected Memory for Operand 2 but got {}",
                            g_HighLevelMirOperandType2Str[type2]),
                contextLoc,
                sourceRef);
        return false;
    }

    if ((instrFlags & Op2_MustBeImm) && type2 != HighLevelMirOperandType::Immediate)
    {
        getSemanticContext()->emitError(
                ErrorSeverity::Fatal,
                std::format("Instruction operand type mismatch: Expected Immediate for Operand 2 but got {}",
                            g_HighLevelMirOperandType2Str[type2]),
                contextLoc,
                sourceRef);
        return false;
    }

    size_t size1 = operands[0].getBitSize();
    size_t size2 = operands[1].getBitSize();

    if ((instrFlags & SizeMatch) && size1 != size2)
    {
        getSemanticContext()->emitError(
                ErrorSeverity::Fatal,
                std::format("Operand bit-width mismatch: Op1 is i{} but Op2 is i{}", size1, size2),
                contextLoc,
                sourceRef);
        return false;
    }

    if ((instrFlags & DestLarger) && size1 <= size2)
    {
        getSemanticContext()->emitError(
                ErrorSeverity::Fatal,
                std::format("Extension bit-width mismatch: Op1 (i{}) must be larger than Op2 (i{})", size1, size2),
                contextLoc,
                sourceRef);
        return false;
    }

    if ((instrFlags & DestSmaller) && size1 >= size2)
    {
        getSemanticContext()->emitError(
                ErrorSeverity::Fatal,
                std::format("Truncation bit-width mismatch: Op1 (i{}) must be smaller than Op2 (i{})", size1, size2),
                contextLoc,
                sourceRef);
        return false;
    }

    return true;
}