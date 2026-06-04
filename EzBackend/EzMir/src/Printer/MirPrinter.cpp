#include "Printer/MirPrinter.h"
#include <format>

std::string MirPrinter::printToString(MirFunction *function) const
{
    std::string result = std::format("{:#^50}\n", " Function Dump ");

    // Print header
    result += std::format("{} {} (param count: {})\n",
                          function->getReturnType()->getName(),
                          function->getName(),
                          function->getParameters().size());

    // Parameters
    result += " - Params: ";
    bool firstParam = true;
    for (auto param : function->getParameters())
    {
        MirOperand *operand = param->m_reg;

        if (!firstParam)
            result += ", ";

        result += operand->toString();
        firstParam = false;
    }
    result += '\n';

    // Stack frame (Fixed spacing/delimiters)
    result += " - Stack Frame: ";
    bool firstFrame = true;
    for (StackFrameObject *frameObj : function->getStackFrame()->getStackFrameObjects())
    {
        if (!firstFrame)
            result += " | ";

        result += std::format("Frame(id: {}, size: {}, source: {}, offset: {:#X})",
                              frameObj->m_id,
                              frameObj->m_sizeInBytes,
                              static_cast<uint8_t>(frameObj->m_source),
                              frameObj->m_offset);
        firstFrame = false;
    }
    result += '\n';

    // Cascade into Blocks
    for (MirBlock *block : function->getBlocks())
    {
        result += printToString(block);
    }

    result += std::format("{:#^50}\n", " End Function Dump ");
    return result;
}

std::string MirPrinter::printToString(MirBlock *block) const
{
    // Indent block headers slightly
    std::string result = std::format("  Block(id: {}):\n", block->getId());

    // Cascade into Instructions
    for (MirInstruction *instr : block->getInstructions())
    {
        result += printToString(instr);
    }

    return result;
}

std::string MirPrinter::printToString(MirInstruction *instr) const
{
    // Indent instructions to sit visually "inside" the block
    std::string result = std::format("    {}", instr->getMetadata().m_name);

    // Print operands (e.g., "MOV @reg(1), @int(5)")
    auto operands = instr->getOperands();
    if (operands.size() > 0)
    {
        result += " ";
        bool firstOp = true;
        for (MirOperand *op : operands)
        {
            if (!firstOp)
                result += ", ";

            result += printToString(op);
            firstOp = false;
        }
    }

    result += '\n';
    return result;
}

std::string MirPrinter::printToString(MirOperand *operand) const { return operand->toString(); }
