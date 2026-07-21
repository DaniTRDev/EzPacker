#include "Printer/MirPrinter.h"

std::string MirPrinter::printToString(MirBlock *block, MirPrinterDetail detail)
{
    // Indent block headers slightly
    std::string result;

    if (!block->getName().empty())
        result = std::format("%block.name={}", block->getName());
    else
        result = std::format("%block.id={}", block->getId());

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

std::string MirPrinter::printToString(MirClass *_class, MirPrinterDetail detail)
{
    if (!_class)
        return "";

    std::string result = std::format("\n{:#^50}\n", " Class Dump ");

    std::string parentInfo = "";
    if (_class->getParentClass() != nullptr)
    {
        parentInfo = std::format(".parent={}", _class->getParentClass()->getName());
    }

    result += std::format("%class.name={}.size={}{}\n",
                          _class->getName(),
                          _class->getType()->getTotalSizeInBytes(),
                          parentInfo);

    result += " - Fields:\n";
    if (_class->getFields().empty())
    {
        result += "\t<None>\n";
    }
    else
    {
        for (auto &field : _class->getFields())
        {
            result += std::format("\t%field.name={}.type={}.offset={:#X}\n",
                                  field->m_name,
                                  field->m_type->getName(),
                                  field->m_offset);
        }
    }

    result += " - VTable Layout:\n";
    auto &vTable = _class->getVTable();
    if (vTable.empty())
    {
        result += "\t<None/Empty>\n";
    }
    else
    {
        for (size_t i = 0; i < vTable.size(); ++i)
        {
            MirFunction *func = vTable[i]->m_func;

            if (detail == MirPrinterDetail::Detailed)
            {
                // Detailed print dumps signature: ret Type class::name(param types)
                std::string paramsStr = "";
                bool firstParam = true;
                for (auto param : func->getParameters())
                {
                    if (!firstParam)
                        paramsStr += ", ";
                    paramsStr += param->getMirType()->getName();
                    firstParam = false;
                }
                result += std::format("\t[Slot {}] {} {}::{}({})\n",
                                      i,
                                      func->getReturnType()->getName(),
                                      _class->getName(),
                                      func->getName(),
                                      paramsStr);
            }
            else // MirPrinterDetail::General
            {
                // General print only dumps method slot names
                result += std::format("\t[Slot {}] {}::{}\n", i, _class->getName(), func->getName());
            }
        }
    }

    result += std::format("{:#^50}\n", " End Class Dump ");
    return result;
}

std::string MirPrinter::printToString(MirFunction *function, MirPrinterDetail detail)
{
    std::string result = std::format("\n{:#^50}\n", " Function Dump ");

    // Print header
    result += std::format("%func.return={}.name={}.paramCount={}\nType:{}\n",
                          function->getReturnType()->getName(),
                          function->getName(),
                          function->getParameters().size(),
                          function->getType()->getName());

    // Parameters
    result += " - Params: \n\t";
    bool firstParam = true;
    for (auto param : function->getParameters())
    {
        if (!firstParam)
            result += "\n\t";

        result += param->toString();
        firstParam = false;
    }
    result += '\n';

    if (detail == MirPrinterDetail::Detailed)
    {
        // Stack frame (Fixed spacing/delimiters)
        result += " - Stack Frame: \n\t";
        bool firstFrame = true;
        for (StackFrameObject *frameObj : function->getStackFrame()->getStackFrameObjects())
        {
            if (!firstFrame)
                result += "\n\t";

            result += std::format("{} %frame.id={}.src={}.offset={:#X})",
                                  frameObj->m_type->getName(),
                                  frameObj->m_id,
                                  static_cast<uint8_t>(frameObj->m_source),
                                  frameObj->m_offset);
            firstFrame = false;
        }
        result += '\n';

        // Cascade into Blocks
        result += " - Block list: \n";
        for (MirBlock *block : function->getBlocks())
        {
            result += printToString(block, detail) + '\n';
        }
        result += '\n';
    }

    result += std::format("{:#^50}\n", " End Function Dump ");
    return result;
}

std::string MirPrinter::printToString(MirGlobalVar *globalVar, MirPrinterDetail detail)
{
    if (!globalVar)
        return "";

    std::string linkageStr = "unknown";
    switch (globalVar->getLinkage())
    {
        case MirGlobalVarLinkage::External:
            linkageStr = "external";
            break;
        case MirGlobalVarLinkage::Internal:
            linkageStr = "internal";
            break;
        case MirGlobalVarLinkage::Weak:
            linkageStr = "weak";
            break;
    }

    std::string result = std::format("%gVar.name={}.id={}.type={}.linkage={}.constant={}\n",
                                     globalVar->getName(),
                                     globalVar->getId(),
                                     globalVar->getType()->getName(),
                                     linkageStr,
                                     globalVar->isConstant() ? "true" : "false");

    // Check the new structured initializer expression tree pointer
    MirOperand *initOperand = globalVar->getInitializer();
    if (!initOperand)
    {
        result += "  Initializer  : zero-initialized";
    }
    else
    {
        result += "  Initializer  : initialized";

        if (detail == MirPrinterDetail::Detailed)
        {
            result += std::format("    Initializer data: {}\n", initOperand->toString());
        }
    }

    return result;
}

std::string MirPrinter::printToString(MirInstruction *instr, MirPrinterDetail detail)
{
    std::string result = std::format("  {:<12}", instr->getMetadata().m_name);

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
