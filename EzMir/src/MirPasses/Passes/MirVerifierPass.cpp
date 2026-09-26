#include "MirPasses/Passes/MirVerifierPass.h"
#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Function/MirFunction.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionSet.h"
#include "Operand/MirOperands.h"
#include "Type/MirType.h"

namespace
{

bool matchesExpectedType(MirOperandType actualKind, ExpectedOperandType expectedMask)
{
    switch (actualKind)
    {
        case MirOperandType::Register:
            return (expectedMask & ExpectedOperandType::Register);
        case MirOperandType::Integer:
            return (expectedMask & ExpectedOperandType::Integer);
        case MirOperandType::FloatingPoint:
            return (expectedMask & ExpectedOperandType::FloatingPoint);
        case MirOperandType::Memory:
            return (expectedMask & ExpectedOperandType::Memory);
        case MirOperandType::Reference:
            return (expectedMask & ExpectedOperandType::Reference);
        case MirOperandType::RuntimeSymbol:
            return (expectedMask & ExpectedOperandType::RuntimeSymbol);
        default:
            return false;
    }
}

const char *getOperandTypeName(MirOperandType type)
{
    switch (type)
    {
        case MirOperandType::Register:
            return "Register";
        case MirOperandType::Integer:
            return "Integer";
        case MirOperandType::FloatingPoint:
            return "FloatingPoint";
        case MirOperandType::Memory:
            return "Memory";
        case MirOperandType::Reference:
            return "Reference";
        case MirOperandType::RuntimeSymbol:
            return "RuntimeSymbol";
        default:
            return "Unknown";
    }
}

} // namespace

MirVerifierPass::MirVerifierPass(MirBuilderContext *ctx) : m_ctx(ctx) {}

const char *MirVerifierPass::getName() const
{
    return "MirVerifierPass";
}

MirPassIterationPlace MirVerifierPass::getIterationPlace() const
{
    return MirPassIterationPlace::Function;
}

MirPassResult MirVerifierPass::run(IntrusiveLinkedList<MirFunction>::const_iterator it, MirPassManager *)
{
    MirFunction *func = *it;
    if (!func)
    {
        return { .m_modifiedMir = false, .m_executed = true, .m_succeeded = false };
    }

    m_result.reset();

    const bool valid = verifyFunction(func);
    return { .m_modifiedMir = false, .m_executed = true, .m_succeeded = valid && (m_result.m_errorCount == 0) };
}

void MirVerifierPass::printResult()
{
    if (!m_ctx || !m_ctx->getDiagCollector())
    {
        return;
    }

    if (m_result.isValid())
    {
        m_ctx->getDiagCollector()->trace(
                "MirVerifierPass",
                "Verification passed: {} instructions verified with 0 errors",
                m_result.m_instructionCount);
    }
    else
    {
        m_ctx->getDiagCollector()->error(
                "MirVerifierPass",
                "Verification failed: {} errors found across {} instructions",
                m_result.m_errorCount,
                m_result.m_instructionCount);
    }
}

void MirVerifierPass::reset()
{
    m_result.reset();
}

bool MirVerifierPass::verifyFunction(MirFunction *func)
{
    if (func->isDeclaration())
    {
        return true;
    }

    bool allBlocksValid = true;
    for (MirBlock *block : func->getBlocks())
    {
        if (!block)
        {
            if (m_ctx && m_ctx->getDiagCollector())
            {
                m_ctx->getDiagCollector()->error("MirVerifierPass",
                                                 "Function '{}' contains a null basic block entry",
                                                 func->getName());
            }
            m_result.m_errorCount++;
            allBlocksValid = false;
            continue;
        }

        if (!verifyBlock(block, func))
        {
            allBlocksValid = false;
        }
    }

    return allBlocksValid;
}

bool MirVerifierPass::verifyBlock(MirBlock *block, MirFunction *func)
{
    bool allInstsValid = true;

    for (MirInstruction *inst : block->getInstructions())
    {
        if (!inst)
        {
            if (m_ctx && m_ctx->getDiagCollector())
            {
                m_ctx->getDiagCollector()->error(
                        "MirVerifierPass",
                        "Block '{}' in function '{}' contains a null instruction node",
                        block->getName(),
                        func->getName());
            }
            m_result.m_errorCount++;
            allInstsValid = false;
            continue;
        }

        m_result.m_instructionCount++;

        if (!verifyInstruction(inst, block, func))
        {
            allInstsValid = false;
        }
    }

    return allInstsValid;
}

bool MirVerifierPass::verifyInstruction(MirInstruction *inst, MirBlock *block, MirFunction *)
{
    const MirInstructionOpCode opcode = inst->getOpCode();
    if (opcode == MirInstructionOpCode::INVALID || opcode >= MirInstructionOpCode::OPCODE_COUNT)
    {
        if (m_ctx && m_ctx->getDiagCollector())
        {
            m_ctx->getDiagCollector()->error(
                    "MirVerifierPass",
                    "Block '{}' contains instruction with invalid or out-of-range opcode {}",
                    block->getName(),
                    static_cast<uint16_t>(opcode))
                    << inst->getSourceRef();
        }
        m_result.m_errorCount++;
        return false;
    }

    const MirInstructionMetadata &meta = inst->getMetadata();

    bool valid = true;
    if (!verifyOperandCount(inst, meta))
    {
        valid = false;
    }

    if (!verifyOperandKinds(inst, meta))
    {
        valid = false;
    }

    if (!verifyFlagsAndSizes(inst, meta))
    {
        valid = false;
    }

    if (!verifyTypeConsistency(inst, meta))
    {
        valid = false;
    }

    if (!verifyTerminatorPlacement(inst, block))
    {
        valid = false;
    }

    return valid;
}

bool MirVerifierPass::verifyOperandCount(MirInstruction *inst, const MirInstructionMetadata &meta)
{
    const size_t actualCount = inst->getOperandCount();

    // Special case: RET can be void with 0 operands or value-returning with 1 operand.
    if (inst->hasOpcode(MirInstructionOpCode::RET))
    {
        if (actualCount > 1)
        {
            if (m_ctx && m_ctx->getDiagCollector())
            {
                m_ctx->getDiagCollector()->error(
                        "MirVerifierPass",
                        "Instruction 'RET' expects at most 1 operand, but found {}",
                        actualCount)
                        << inst->getSourceRef();
            }
            m_result.m_errorCount++;
            return false;
        }
        return true;
    }

    const bool isVariadic = (meta.m_flags & MirInstructionFlags::VariadicArgs);
    if (isVariadic)
    {
        const size_t minRequired = meta.m_operandMeta.m_count > 0 ? meta.m_operandMeta.m_count - 1 : 0;
        if (actualCount < minRequired)
        {
            if (m_ctx && m_ctx->getDiagCollector())
            {
                m_ctx->getDiagCollector()->error(
                        "MirVerifierPass",
                        "Variadic instruction '{}' requires at least {} operands, but found {}",
                        meta.m_name,
                        minRequired,
                        actualCount)
                        << inst->getSourceRef();
            }
            m_result.m_errorCount++;
            return false;
        }
        return true;
    }

    if (actualCount != meta.m_operandMeta.m_count)
    {
        if (m_ctx && m_ctx->getDiagCollector())
        {
            m_ctx->getDiagCollector()->error(
                    "MirVerifierPass",
                    "Instruction '{}' expects {} operands, but found {}",
                    meta.m_name,
                    meta.m_operandMeta.m_count,
                    actualCount)
                    << inst->getSourceRef();
        }
        m_result.m_errorCount++;
        return false;
    }

    return true;
}

bool MirVerifierPass::verifyOperandKinds(MirInstruction *inst, const MirInstructionMetadata &meta)
{
    bool valid = true;
    const size_t operandCount = inst->getOperandCount();
    const bool isVariadic = (meta.m_flags & MirInstructionFlags::VariadicArgs);

    for (size_t i = 0; i < operandCount; ++i)
    {
        const MirOperand *op = inst->getConstOperand(i);
        if (!op)
        {
            if (m_ctx && m_ctx->getDiagCollector())
            {
                m_ctx->getDiagCollector()->error(
                        "MirVerifierPass",
                        "Instruction '{}' operand at index {} is null",
                        meta.m_name,
                        i)
                        << inst->getSourceRef();
            }
            m_result.m_errorCount++;
            valid = false;
            continue;
        }

        // Determine expected slot metadata
        ExpectedOperandType expectedType = ExpectedOperandType::Any;
        MirOperandFlag expectedFlag = MirOperandFlag::Read;

        if (i < meta.m_operandMeta.m_count)
        {
            expectedType = meta.m_operandMeta.m_slots[i].type;
            expectedFlag = meta.m_operandMeta.m_slots[i].flags;
        }
        else if (isVariadic && meta.m_operandMeta.m_count > 0)
        {
            // Expand with variadic slot metadata (at index m_count - 1)
            expectedType = meta.m_operandMeta.m_slots[meta.m_operandMeta.m_count - 1].type;
            expectedFlag = meta.m_operandMeta.m_slots[meta.m_operandMeta.m_count - 1].flags;
        }

        // For CALL instructions, operand 1 is the call target: can be a symbolic reference (@func),
        // named runtime symbol, or indirect register/address value.
        if (inst->hasOpcode(MirInstructionOpCode::CALL) && i == 1)
        {
            expectedType = ExpectedOperandType::Register | ExpectedOperandType::Reference |
                           ExpectedOperandType::RuntimeSymbol | ExpectedOperandType::Integer;
        }

        // If variadic slot is ExpectedOperandType::VariadicArgs, accept Any
        if (expectedType & ExpectedOperandType::VariadicArgs)
        {
            expectedType = ExpectedOperandType::Any;
        }

        if (!matchesExpectedType(op->getType(), expectedType))
        {
            if (m_ctx && m_ctx->getDiagCollector())
            {
                m_ctx->getDiagCollector()->error(
                        "MirVerifierPass",
                        "Instruction '{}' operand {} ('{}'): unexpected operand kind '{}'",
                        meta.m_name,
                        i,
                        op->toString(),
                        getOperandTypeName(op->getType()))
                        << op->getSourceRef();
            }
            m_result.m_errorCount++;
            valid = false;
        }

        // Write (DEF) operands must be registers
        if (expectedFlag & MirOperandFlag::Write)
        {
            if (op->getType() != MirOperandType::Register)
            {
                if (m_ctx && m_ctx->getDiagCollector())
                {
                    m_ctx->getDiagCollector()->error(
                            "MirVerifierPass",
                            "Instruction '{}' operand {} is marked Write (DEF), but is not a register",
                            meta.m_name,
                            i)
                            << op->getSourceRef();
                }
                m_result.m_errorCount++;
                valid = false;
            }
        }
    }

    return valid;
}

bool MirVerifierPass::verifyFlagsAndSizes(MirInstruction *inst, const MirInstructionMetadata &meta)
{
    bool valid = true;
    const size_t operandCount = inst->getOperandCount();

    // 1. SizeMatch
    if (meta.m_flags & MirInstructionFlags::SizeMatch)
    {
        if (meta.m_category == MirInstructionCategory::MirCat_Compare)
        {
            // For relational comparisons (CMP_EQ, CMP_SLT, etc.), destination holds boolean (i1),
            // while operands 1 and 2 (lhs and rhs) are compared and must share identical bit-width.
            if (operandCount >= 3)
            {
                const MirOperand *lhs = inst->getConstOperand(1);
                const MirOperand *rhs = inst->getConstOperand(2);
                if (lhs && rhs && lhs->getMirType() && rhs->getMirType())
                {
                    const size_t lhsBits = lhs->getMirType()->getTotalSizeInBits();
                    const size_t rhsBits = rhs->getMirType()->getTotalSizeInBits();
                    if (lhsBits != rhsBits)
                    {
                        if (m_ctx && m_ctx->getDiagCollector())
                        {
                            m_ctx->getDiagCollector()->error(
                                    "MirVerifierPass",
                                    "Instruction '{}' has SizeMatch flag: compared source operands lhs ({} bits) and "
                                    "rhs ({} bits) bit-widths do not match",
                                    meta.m_name,
                                    lhsBits,
                                    rhsBits)
                                    << inst->getSourceRef();
                        }
                        m_result.m_errorCount++;
                        valid = false;
                    }
                }
            }
        }
        else
        {
            // For standard ALU, bitwise, vector, bitcast: all typed operands must have identical bit-width.
            size_t expectedBits = 0;
            size_t referenceIndex = 0;

            for (size_t i = 0; i < operandCount; ++i)
            {
                const MirOperand *op = inst->getConstOperand(i);
                if (!op || !op->getMirType())
                {
                    continue;
                }

                const size_t bits = op->getMirType()->getTotalSizeInBits();
                if (bits == 0)
                {
                    continue;
                }

                if (expectedBits == 0)
                {
                    expectedBits = bits;
                    referenceIndex = i;
                }
                else if (bits != expectedBits)
                {
                    if (m_ctx && m_ctx->getDiagCollector())
                    {
                        m_ctx->getDiagCollector()->error(
                                "MirVerifierPass",
                                "Instruction '{}' has SizeMatch flag: operand {} bit-width ({} bits) does not match "
                                "operand {} bit-width ({} bits)",
                                meta.m_name,
                                i,
                                bits,
                                referenceIndex,
                                expectedBits)
                                << inst->getSourceRef();
                    }
                    m_result.m_errorCount++;
                    valid = false;
                    break;
                }
            }
        }
    }

    // 2. DestLarger (e.g. ZEXT, SEXT, FPEXT)
    if (meta.m_flags & MirInstructionFlags::DestLarger)
    {
        if (operandCount >= 2)
        {
            const MirOperand *dst = inst->getConstOperand(0);
            const MirOperand *src = inst->getConstOperand(1);
            if (dst && src && dst->getMirType() && src->getMirType())
            {
                const size_t dstBits = dst->getMirType()->getTotalSizeInBits();
                const size_t srcBits = src->getMirType()->getTotalSizeInBits();
                if (dstBits <= srcBits)
                {
                    if (m_ctx && m_ctx->getDiagCollector())
                    {
                        m_ctx->getDiagCollector()->error(
                                "MirVerifierPass",
                                "Instruction '{}' has DestLarger flag: destination bit-width ({} bits) must exceed "
                                "source bit-width ({} bits)",
                                meta.m_name,
                                dstBits,
                                srcBits)
                                << inst->getSourceRef();
                    }
                    m_result.m_errorCount++;
                    valid = false;
                }
            }
        }
    }

    // 3. DestSmaller (e.g. TRUNC, FPTRUNC)
    if (meta.m_flags & MirInstructionFlags::DestSmaller)
    {
        if (operandCount >= 2)
        {
            const MirOperand *dst = inst->getConstOperand(0);
            const MirOperand *src = inst->getConstOperand(1);
            if (dst && src && dst->getMirType() && src->getMirType())
            {
                const size_t dstBits = dst->getMirType()->getTotalSizeInBits();
                const size_t srcBits = src->getMirType()->getTotalSizeInBits();
                if (dstBits >= srcBits)
                {
                    if (m_ctx && m_ctx->getDiagCollector())
                    {
                        m_ctx->getDiagCollector()->error(
                                "MirVerifierPass",
                                "Instruction '{}' has DestSmaller flag: destination bit-width ({} bits) must be "
                                "smaller than source bit-width ({} bits)",
                                meta.m_name,
                                dstBits,
                                srcBits)
                                << inst->getSourceRef();
                    }
                    m_result.m_errorCount++;
                    valid = false;
                }
            }
        }
    }

    return valid;
}

bool MirVerifierPass::verifyTypeConsistency(MirInstruction *inst, const MirInstructionMetadata &meta)
{
    bool valid = true;
    const size_t operandCount = inst->getOperandCount();

    // 1. TreatAsSigned requires integer scalar types
    if (meta.m_flags & MirInstructionFlags::TreatAsSigned)
    {
        for (size_t i = 0; i < operandCount; ++i)
        {
            const MirOperand *op = inst->getConstOperand(i);
            if (!op || !op->getMirType())
            {
                continue;
            }

            if (op->getMirType()->getKind() == MirTypeKind::FloatingPoint)
            {
                if (m_ctx && m_ctx->getDiagCollector())
                {
                    m_ctx->getDiagCollector()->error(
                            "MirVerifierPass",
                            "Instruction '{}' has TreatAsSigned flag, but operand {} has floating-point type",
                            meta.m_name,
                            i)
                            << op->getSourceRef();
                }
                m_result.m_errorCount++;
                valid = false;
            }
        }
    }

    // 2. Pure float ALU instructions (FADD, FSUB, FMUL, FDIV, FREM, FCMP) require floating-point operands
    const bool isFloatAlu = (inst->hasOpcode(MirInstructionOpCode::FADD) ||
                             inst->hasOpcode(MirInstructionOpCode::FSUB) ||
                             inst->hasOpcode(MirInstructionOpCode::FMUL) ||
                             inst->hasOpcode(MirInstructionOpCode::FDIV) ||
                             inst->hasOpcode(MirInstructionOpCode::FCMP));

    if (isFloatAlu)
    {
        // For FCMP, destination is boolean i1, but lhs and rhs must be float
        const size_t startIdx = inst->hasOpcode(MirInstructionOpCode::FCMP) ? 1 : 0;
        for (size_t i = startIdx; i < operandCount; ++i)
        {
            const MirOperand *op = inst->getConstOperand(i);
            if (op && op->getMirType() && op->getMirType()->getKind() != MirTypeKind::FloatingPoint)
            {
                if (m_ctx && m_ctx->getDiagCollector())
                {
                    m_ctx->getDiagCollector()->error(
                            "MirVerifierPass",
                            "Floating-point instruction '{}' operand {} must have floating-point type",
                            meta.m_name,
                            i)
                            << op->getSourceRef();
                }
                m_result.m_errorCount++;
                valid = false;
            }
        }
    }

    // 3. Conversions SITOFP and FPTOSI
    if (inst->hasOpcode(MirInstructionOpCode::SITOFP) && operandCount >= 2)
    {
        const MirOperand *dst = inst->getConstOperand(0);
        const MirOperand *src = inst->getConstOperand(1);
        if (dst && dst->getMirType() && dst->getMirType()->getKind() != MirTypeKind::FloatingPoint)
        {
            if (m_ctx && m_ctx->getDiagCollector())
            {
                m_ctx->getDiagCollector()->error(
                        "MirVerifierPass",
                        "Instruction 'SITOFP' destination must have floating-point type")
                        << dst->getSourceRef();
            }
            m_result.m_errorCount++;
            valid = false;
        }
        if (src && src->getMirType() && src->getMirType()->getKind() != MirTypeKind::Integer)
        {
            if (m_ctx && m_ctx->getDiagCollector())
            {
                m_ctx->getDiagCollector()->error(
                        "MirVerifierPass",
                        "Instruction 'SITOFP' source must have integer type")
                        << src->getSourceRef();
            }
            m_result.m_errorCount++;
            valid = false;
        }
    }
    else if (inst->hasOpcode(MirInstructionOpCode::FPTOSI) && operandCount >= 2)
    {
        const MirOperand *dst = inst->getConstOperand(0);
        const MirOperand *src = inst->getConstOperand(1);
        if (dst && dst->getMirType() && dst->getMirType()->getKind() != MirTypeKind::Integer)
        {
            if (m_ctx && m_ctx->getDiagCollector())
            {
                m_ctx->getDiagCollector()->error(
                        "MirVerifierPass",
                        "Instruction 'FPTOSI' destination must have integer type")
                        << dst->getSourceRef();
            }
            m_result.m_errorCount++;
            valid = false;
        }
        if (src && src->getMirType() && src->getMirType()->getKind() != MirTypeKind::FloatingPoint)
        {
            if (m_ctx && m_ctx->getDiagCollector())
            {
                m_ctx->getDiagCollector()->error(
                        "MirVerifierPass",
                        "Instruction 'FPTOSI' source must have floating-point type")
                        << src->getSourceRef();
            }
            m_result.m_errorCount++;
            valid = false;
        }
    }

    return valid;
}

bool MirVerifierPass::verifyTerminatorPlacement(MirInstruction *inst, MirBlock *block)
{
    if (inst->getFlags() & MirInstructionFlags::IsTerminator)
    {
        if (inst->getNext() != nullptr)
        {
            if (m_ctx && m_ctx->getDiagCollector())
            {
                m_ctx->getDiagCollector()->warn(
                        "MirVerifierPass",
                        "Instruction '{}' is a terminator, but is followed by dead instruction '{}' in block '{}'",
                        inst->getOpCodeName(),
                        inst->getNext()->getOpCodeName(),
                        block->getName())
                        << inst->getSourceRef();
            }
            m_result.m_warningCount++;
        }
    }

    return true;
}
