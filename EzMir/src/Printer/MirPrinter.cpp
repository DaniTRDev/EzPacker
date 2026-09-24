#include "Builder/MirBuilderContext.h"
#include "Block/MirBlock.h"
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

/**
 * Formats an entire module. In Diagnostic mode it prints detailed dumps of all globals and
 * functions; in Parseable mode it emits round-trippable global declarations followed by functions.
 */
std::string MirPrinter::printModule(MirBuilderContext *ctx, MirPrinterMode mode)
{
    if (!ctx)
        return "";

    if (mode == MirPrinterMode::Diagnostic)
    {
        std::string result = "=== MIR Module Diagnostic Dump ===\n";
        for (MirGlobalVar *var : ctx->getGlobalVars())
        {
            result += printToString(var, MirPrinterDetail::Detailed);
        }
        for (MirFunction *fn : ctx->getFunctions())
        {
            result += printToString(fn, MirPrinterDetail::Detailed);
        }
        return result;
    }

    std::string result;
    for (MirGlobalVar *var : ctx->getGlobalVars())
    {
        result += printGlobalVar(var, mode);
        result += '\n';
    }

    if (!ctx->getGlobalVars().empty() && !ctx->getFunctions().empty())
    {
        result += '\n';
    }

    bool firstFunc = true;
    for (MirFunction *fn : ctx->getFunctions())
    {
        if (!firstFunc)
        {
            result += '\n';
        }
        result += printFunction(fn, mode);
        firstFunc = false;
    }

    return result;
}

/**
 * Formats a global variable declaration (@name = linkage const|var type [= init];), or a detailed
 * dump in Diagnostic mode.
 */
std::string MirPrinter::printGlobalVar(MirGlobalVar *var, MirPrinterMode mode)
{
    if (!var)
        return "";

    if (mode == MirPrinterMode::Diagnostic)
    {
        return printToString(var, MirPrinterDetail::Detailed);
    }

    std::string_view linkageStr = "internal";
    switch (var->getLinkage())
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

    std::string result = std::format("@{} = {} {} {}",
                                     var->getName(),
                                     linkageStr,
                                     var->isConstant() ? "const" : "var",
                                     var->getType() ? var->getType()->getName() : "void");

    if (var->getInitializer())
    {
        if (var->getInitializer()->getType() == MirOperandType::Integer)
        {
            auto *imm = static_cast<MirInteger *>(var->getInitializer());
            result += std::format(" = {}", imm->getValue().getI64());
        }
        else if (var->getInitializer()->getType() == MirOperandType::FloatingPoint)
        {
            auto *fImm = static_cast<MirFloat *>(var->getInitializer());
            result += std::format(" = {}", fImm->getValue().toString(10));
        }
        else
        {
            result += std::format(" = {}", printOperand(var->getInitializer(), mode));
        }
    }
    result += ';';
    return result;
}

/**
 * Formats a function as either a "declare" prototype (no blocks) or a full "fn ... { blocks }"
 * definition, or a detailed dump in Diagnostic mode.
 */
std::string MirPrinter::printFunction(MirFunction *function, MirPrinterMode mode)
{
    if (!function)
        return "";

    if (mode == MirPrinterMode::Diagnostic)
    {
        return printToString(function, MirPrinterDetail::Detailed);
    }

    std::string fnName = !function->getName().empty() ? std::string(function->getName()) : "anonymous";
    std::string retType = function->getReturnType() ? std::string(function->getReturnType()->getName()) : "void";

    if (function->getBlocks().empty())
    {
        std::string result = std::format("declare @{}(", fnName);
        bool firstParam = true;
        for (MirRegister *param : function->getParameters())
        {
            if (!firstParam)
                result += ", ";
            result += param->getMirType() ? param->getMirType()->getName() : "i64";
            firstParam = false;
        }
        result += std::format(") -> {};\n", retType);
        return result;
    }

    std::string result = std::format("fn @{}(", fnName);
    bool firstParam = true;
    for (MirRegister *param : function->getParameters())
    {
        if (!firstParam)
            result += ", ";
        std::string typeStr = param->getMirType() ? std::string(param->getMirType()->getName()) : "i64";
        std::string pName =
                !param->getName().empty() ? std::string(param->getName()) : std::format("%v{}", param->getRegId());
        if (!pName.starts_with("%"))
            pName = "%" + pName;
        result += std::format("{} {}", typeStr, pName);
        firstParam = false;
    }
    result += std::format(") -> {} {{\n", retType);

    for (MirBlock *block : function->getBlocks())
    {
        result += printBlock(block, mode);
    }

    result += "}\n";
    return result;
}

/**
 * Formats a block as "label:" followed by its instructions, or a detailed dump in Diagnostic mode.
 */
std::string MirPrinter::printBlock(MirBlock *block, MirPrinterMode mode)
{
    if (!block)
        return "";

    if (mode == MirPrinterMode::Diagnostic)
    {
        return printToString(block, MirPrinterDetail::Detailed);
    }

    std::string blkName =
            !block->getName().empty() ? std::string(block->getName()) : std::format("block_{}", block->getId());
    if (blkName.starts_with("%"))
    {
        blkName = blkName.substr(1);
    }

    std::string result = std::format("{}:\n", blkName);
    for (MirInstruction *instr : block->getInstructions())
    {
        result += printInstruction(instr, mode);
    }
    return result;
}

/**
 * Formats an instruction as indented "opcode op, op, ...;" using the target mnemonic for selected
 * TARGET_INST instructions, or a detailed dump in Diagnostic mode.
 */
std::string MirPrinter::printInstruction(MirInstruction *instr, MirPrinterMode mode)
{
    if (!instr)
        return "";

    if (mode == MirPrinterMode::Diagnostic)
    {
        return printToString(instr, MirPrinterDetail::Detailed);
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

    std::string result = std::format("    {}", opName);
    const auto &operands = instr->getOperands();
    if (!operands.empty())
    {
        result += " ";
        bool firstOp = true;
        for (MirOperand *op : operands)
        {
            if (!firstOp)
            {
                result += ", ";
            }
            result += printOperand(op, mode);
            firstOp = false;
        }
    }
    result += ";\n";
    return result;
}

/**
 * Formats an operand in parseable syntax, switching on its concrete kind (register, immediate,
 * reference, runtime symbol or memory address); falls back to the operand's own toString and
 * delegates to the diagnostic formatter in Diagnostic mode.
 */
std::string MirPrinter::printOperand(MirOperand *operand, MirPrinterMode mode)
{
    return formatOperand(operand, mode != MirPrinterMode::Diagnostic);
}

/**
 * Formats a block's header (name, ID, instruction count) and, when Detailed, each instruction line.
 */
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

/**
 * Formats a function between decorative banners, always listing the signature and parameters and,
 * when Detailed, the stack frame objects and all blocks/instructions.
 */
std::string MirPrinter::printToString(MirFunction *function, MirPrinterDetail detail)
{
    if (!function)
        return "<null function>\n";

    std::string result = std::format("\n{:=^60}\n", " Function Dump ");

    // Signature header
    std::pmr::string fnName = !function->getName().empty() ? function->getName() : "<anonymous>";
    std::string_view retTypeName = function->getReturnType() ? function->getReturnType()->getName() : "void";
    result += std::format("fn {}() -> {} [params: {}]\n", fnName, retTypeName, function->getParameters().size());

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

/**
 * Formats a global variable's header and initializer; Detailed prints the full initializer operand,
 * otherwise only whether it is initialized.
 */
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

    std::string_view varTypeName = globalVar->getType() ? globalVar->getType()->getName() : "void";
    std::string result = std::format("@{} [id: {}, type: {}, linkage: {}, const: {}]\n",
                                     globalVar->getName(),
                                     globalVar->getId(),
                                     varTypeName,
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

/**
 * Formats an instruction with an aligned tier, opcode and target tag column followed by its
 * operands. The detail parameter is currently unused.
 */
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

/**
 * Returns the operand's own textual form, or a placeholder for a null operand.
 */
std::string MirPrinter::printToString(MirOperand *operand) { return operand ? operand->toString() : "<null operand>"; }

/**
 * Formats a register reference as %v<n>(class) for virtual or %p<n>(class) for physical registers.
 */
std::string MirPrinter::printToString(const MirRegisterRef &ref)
{
    char prefix = ref.isVirtual() ? 'v' : 'p';
    const char *className = ref.getClass() ? ref.getClass()->getName() : "unselected";
    return std::format("%{}{}({})", prefix, ref.getId(), className);
}

/**
 * Formats a stack frame object as its ID, type, signed offset and source origin.
 */
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