#include "Printer/MirPrinter.h"
#include <format>

std::string MirPrinter::printToString(MirFunction *function, MirPrinterDetail detail)
{
    std::string result = std::format("{:#^50}\n", " Function Dump ");

    // Print header
    result += std::format("%func.return={}.name={}.paramCount={}\n",
                          function->getReturnType()->getName(),
                          function->getName(),
                          function->getParameters().size());

    // Parameters
    result += " - Params: ";
    bool firstParam = true;
    for (auto param : function->getParameters())
    {
        if (!firstParam)
            result += ", ";

        result += param->toString();
        firstParam = false;
    }
    result += '\n';

    if (detail == MirPrinterDetail::Detailed)
    {
        // Stack frame (Fixed spacing/delimiters)
        result += " - Stack Frame: ";
        bool firstFrame = true;
        for (StackFrameObject *frameObj : function->getStackFrame()->getStackFrameObjects())
        {
            if (!firstFrame)
                result += " | ";

            result += std::format("%frame.id={}.size={}.src={}.offset={:#X})",
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
            result += printToString(block, detail);
        }
    }

    result += std::format("{:#^50}\n", " End Function Dump ");
    return result;
}

std::string MirPrinter::printToString(MirBlock *block, MirPrinterDetail detail)
{
    // Indent block headers slightly
    std::string result;

    if (!block->getName().empty())
        result = std::format("  %block.name={}", block->getName());
    else
        result = std::format("  %block.id={}", block->getId());

    result += std::format(".instrCount={}\n", block->getInstructions().size());

    if (detail == MirPrinterDetail::Detailed)
    {
        // Cascade into Instructions
        for (MirInstruction *instr : block->getInstructions())
        {
            result += printToString(instr, detail);
        }
    }

    return result;
}

std::string MirPrinter::printToString(MirInstruction *instr, MirPrinterDetail detail)
{
    // Indent instructions to sit visually "inside" the block
    std::string result = std::format("    {}", instr->getMetadata().m_name);

    // Print operands
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

std::string MirPrinter::printToString(MirOperand *operand) { return operand->toString(); }
