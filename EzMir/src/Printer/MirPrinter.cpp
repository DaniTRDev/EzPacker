#include "Block/MirBlock.h"
#include "Class/MirClass.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirTargetInstructionDesc.h"
#include "Instruction/MirInstructionSet.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionStackFrame.h"
#include "GlobalVar/MirGlobalVar.h"
#include "Operand/MirOperand.h"
#include "Operand/MirOperands.h"
#include "Operand/MirRegisterClass.h"
#include "Operand/MirRegisterReference.h"
#include "Printer/MirPrinter.h"
#include "Type/MirType.h"

std::string MirPrinter::printToString(MirBlock *block, MirPrinterDetail detail)
{
    if (!block)
        return "<null block>\n";

    std::string result;
    const std::pmr::string &blockName = block->getName();

    if (blockName.empty())
    {
        result = std::format("%block_{} (id={}, instrs={}):\n",
                             block->getId(),
                             block->getId(),
                             block->getInstructions().size());
    }
    else
    {
        result = std::format("%{} (id={}, instrs={}):\n", blockName, block->getId(), block->getInstructions().size());
    }

    if (detail == MirPrinterDetail::Detailed)
    {
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

    std::string result = std::format("\n{:=^60}\n", " Class Dump ");

    std::string parentInfo;
    if (_class->getParentClass() != nullptr)
    {
        parentInfo = std::format(" : {}", _class->getParentClass()->getName());
    }

    result += std::format("class {}{} [size: {} bytes]\n",
                          _class->getName(),
                          parentInfo,
                          _class->getType()->getTotalSizeInBytes());

    result += "  Fields:\n";
    if (_class->getFields().empty())
    {
        result += "    <none>\n";
    }
    else
    {
        for (const auto &field : _class->getFields())
        {
            result += std::format("    +0x{:02X}: {} {}\n", field->m_offset, field->m_type->getName(), field->m_name);
        }
    }

    result += "  VTable:\n";
    const auto &vTable = _class->getVTable();
    if (vTable.empty())
    {
        result += "    <empty>\n";
    }
    else
    {
        for (size_t i = 0; i < vTable.size(); ++i)
        {
            MirFunction *func = vTable[i]->m_func;

            if (detail == MirPrinterDetail::Detailed)
            {
                std::string paramsStr;
                bool firstParam = true;
                for (auto *param : func->getParameters())
                {
                    if (!firstParam)
                        paramsStr += ", ";
                    paramsStr += param->getMirType()->getName();
                    firstParam = false;
                }
                result += std::format("    [{:>2}] {} {}::{}({})\n",
                                      i,
                                      func->getReturnType()->getName(),
                                      _class->getName(),
                                      func->getName(),
                                      paramsStr);
            }
            else
            {
                result += std::format("    [{:>2}] {}::{}\n", i, _class->getName(), func->getName());
            }
        }
    }

    result += std::format("{:=^60}\n", " End Class Dump ");
    return result;
}

std::string MirPrinter::printToString(MirFunction *function, MirPrinterDetail detail)
{
    if (!function)
        return "<null function>\n";

    std::string result = std::format("\n{:=^60}\n", " Function Dump ");

    // Signature header
    std::pmr::string fnName = !function->getName().empty() ? function->getName() : "<anonymous>";
    result += std::format("fn {}() -> {} [params: {}]\n",
                          fnName,
                          function->getReturnType()->getName(),
                          function->getParameters().size());

    // Parameters
    result += "  Params:\n";
    if (function->getParameters().empty())
    {
        result += "    <none>\n";
    }
    else
    {
        for (auto *param : function->getParameters())
        {
            result += std::format("    {}\n", param->toString());
        }
    }

    if (detail == MirPrinterDetail::Detailed)
    {
        // Stack Frame
        result += "  Stack Frame:\n";
        auto *frame = function->getStackFrame();
        if (!frame || frame->getObjects().empty())
        {
            result += "    <empty>\n";
        }
        else
        {
            for (const StackFrameObject *frameObj : frame->getObjects())
            {
                result += std::format("    {}\n", printToString(frameObj));
            }
        }

        // Basic Blocks & Instructions
        result += "  Blocks:\n";
        if (function->getBlocks().empty())
        {
            result += "    <no blocks>\n";
        }
        else
        {
            for (MirBlock *block : function->getBlocks())
            {
                result += printToString(block, detail);
            }
        }
    }

    result += std::format("{:=^60}\n", " End Function Dump ");
    return result;
}

std::string MirPrinter::printToString(MirGlobalVar *globalVar, MirPrinterDetail detail)
{
    if (!globalVar)
        return "";

    std::string_view linkageStr = "internal";
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

    std::string result = std::format("@{} [id: {}, type: {}, linkage: {}, const: {}]\n",
                                     globalVar->getName(),
                                     globalVar->getId(),
                                     globalVar->getType()->getName(),
                                     linkageStr,
                                     globalVar->isConstant() ? "true" : "false");

    MirOperand *initOperand = globalVar->getInitializer();
    if (!initOperand)
    {
        result += "  init: <zeroinit>\n";
    }
    else
    {
        if (detail == MirPrinterDetail::Detailed)
        {
            result += std::format("  init: {}\n", initOperand->toString());
        }
        else
        {
            result += "  init: <initialized>\n";
        }
    }

    return result;
}

std::string MirPrinter::printToString(MirInstruction *instr, MirPrinterDetail /*detail*/)
{
    if (!instr)
        return "    <null instruction>\n";

    std::string_view tier = "HL ";
    switch (instr->getTier())
    {
        case MirInstructionTier::HighLevel:
            tier = "HL ";
            break;
        case MirInstructionTier::PassInternal:
            tier = "INT";
            break;
        case MirInstructionTier::TargetLow:
            tier = "TL";
            break;
    }

    const MirTargetInstructionDesc *desc = instr->getTargetDesc();
    std::string opName;

    if (instr->getOpCode() == MirInstructionOpCode::TARGET_INST && desc)
    {
        opName = desc->getName();
    }
    else
    {
        opName = instr->getMetadata().m_name;
    }

    // Align Tier, Opcode, and metadata tag
    std::string targetTag = desc ? std::format("({}:{})", desc->getId(), desc->getName()) : "(unselected)";
    std::string result = std::format("  {} {:<12} {:<20}", tier, opName, targetTag);

    // Operands formatting
    const auto &operands = instr->getOperands();
    if (!operands.empty())
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

std::string MirPrinter::printToString(MirOperand *operand) { return operand ? operand->toString() : "<null operand>"; }

std::string MirPrinter::printToString(const MirRegisterRef &ref)
{
    char prefix = ref.isVirtual() ? 'v' : 'p';
    const char *className = ref.getClass() ? ref.getClass()->getName() : "unselected";
    return std::format("%{}{}({})", prefix, ref.getId(), className);
}

std::string MirPrinter::printToString(const StackFrameObject *obj)
{
    if (!obj)
        return "<null frame object>";

    std::string_view src = "var";
    switch (obj->m_source)
    {
        case StackFrameObjectSource::Parameter:
            src = "param";
            break;
        case StackFrameObjectSource::Spill:
            src = "spill";
            break;
        case StackFrameObjectSource::Variable:
            src = "var";
            break;
    }

    int64_t off = obj->m_offset;
    std::string offsetStr = (off >= 0) ? std::format("+0x{:X}", off) : std::format("-0x{:X}", -off);

    return std::format("[%frame.{:<2}] type: {:<6} offset: {:<8} (src: {})",
                       obj->m_id,
                       obj->m_type ? obj->m_type->getName() : "void",
                       offsetStr,
                       src);
}