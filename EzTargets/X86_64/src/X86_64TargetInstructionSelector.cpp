#include "X86_64TargetInstructionSelector.h"
#include "x86_64TargetInstructionTable.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Operand/MirOperands.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirRegisterClass.h"
#include "Operand/MirRegisterBank.h"
#include "Builder/MirBuilderContext.h"
#include "Type/MirTypeTable.h"
#include "FlexNumber/FlexInt.h"
#include "Block/MirBlock.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionRegisterInfo.h"
#include <map>

namespace EzTargets::X86_64
{

/**
 * Forwards the target descriptor to the generated base selector and caches it for class lookups.
 */
X86_64TargetInstructionSelector::X86_64TargetInstructionSelector(TargetDesc *targetDesc) :
    x86_64InstructionSelector(targetDesc), m_targetDesc(targetDesc)
{
}

/**
 * Selects an instruction by first delegating to the generated base selector, then falling back to
 * the hand-written rules for control flow, calls, comparisons, memory and floating point.
 */
bool X86_64TargetInstructionSelector::select(MirBuilderContext *ctx, MirInstruction *inst)
{
    if (!ctx || !inst)
    {
        return false;
    }

    // 1. Try base generated instruction selector first
    if (x86_64InstructionSelector::select(ctx, inst))
    {
        return true;
    }

    // 2. Fallback selection for control flow, comparisons, calls, and memory
    switch (inst->getOpCode())
    {
        case MirInstructionOpCode::FADD:
        case MirInstructionOpCode::FSUB:
        case MirInstructionOpCode::FMUL:
        case MirInstructionOpCode::FDIV:
            return selectFloatALU(ctx, inst);
        case MirInstructionOpCode::SITOFP:
        case MirInstructionOpCode::FPTOSI:
            return selectFloatCvt(ctx, inst);
        case MirInstructionOpCode::MOV:
            return selectMOV(ctx, inst);
        case MirInstructionOpCode::JMP:
            return selectJMP(ctx, inst);
        case MirInstructionOpCode::BR_COND:
            return selectBR_COND(ctx, inst);
        case MirInstructionOpCode::CALL:
            return selectCALL(ctx, inst);
        case MirInstructionOpCode::LOAD:
            return selectLOAD(ctx, inst);
        case MirInstructionOpCode::STORE:
            return selectSTORE(ctx, inst);
        case MirInstructionOpCode::PHI:
            return selectPHI(ctx, inst);
        case MirInstructionOpCode::CMP_EQ:
        case MirInstructionOpCode::CMP_NE:
        case MirInstructionOpCode::CMP_SLT:
        case MirInstructionOpCode::CMP_SLE:
        case MirInstructionOpCode::CMP_SGT:
        case MirInstructionOpCode::CMP_SGE:
        case MirInstructionOpCode::CMP_ULT:
        case MirInstructionOpCode::CMP_ULE:
        case MirInstructionOpCode::CMP_UGT:
        case MirInstructionOpCode::CMP_UGE:
            return selectCMP(ctx, inst);
        default:
            break;
    }

    return false;
}

/**
 * Replaces a generic JMP with a target JMP to its destination operand.
 */
bool X86_64TargetInstructionSelector::selectJMP(MirBuilderContext *ctx, MirInstruction *inst)
{
    if (inst->getOperandCount() < 1)
    {
        return false;
    }

    MirInstructionBuilder ib(ctx, inst, InsertionType::InsertBefore);
    ib.buildTarget(x86_64TargetInst::getTargetDesc(x86_64TargetInst::JMP),
                   inst->getSourceRef(),
                   { inst->getOperand(0) });
    inst->eraseFromOwner();
    return true;
}

/**
 * Lowers a conditional branch into CMP against zero followed by JNE to the true block and JMP to
 * the false block.
 */
bool X86_64TargetInstructionSelector::selectBR_COND(MirBuilderContext *ctx, MirInstruction *inst)
{
    if (inst->getOperandCount() < 3)
    {
        return false;
    }

    auto *cond = inst->getOperand(0);
    auto *trueBlock = inst->getOperand(1);
    auto *falseBlock = inst->getOperand(2);

    MirInstructionBuilder ib(ctx, inst, InsertionType::InsertBefore);
    MirOperandBuilder ob(ctx);

    if (auto *r = cond->get<MirRegister>())
    {
        if (!r->getRegClass())
        {
            r->setClass(findClass("GPR32"));
        }
    }

    auto *zeroImm = ob.buildInt(ctx->getTypeTable()->i32(), FlexInt(static_cast<int32_t>(0)));

    // Emit: CMP32ri %cond, 0
    ib.buildTarget(x86_64TargetInst::getTargetDesc(x86_64TargetInst::CMP32ri),
                   inst->getSourceRef(),
                   { cond, zeroImm });

    // Emit: JNE %trueBlock
    ib.buildTarget(x86_64TargetInst::getTargetDesc(x86_64TargetInst::JNE),
                   inst->getSourceRef(),
                   { trueBlock });

    // Emit: JMP %falseBlock
    ib.buildTarget(x86_64TargetInst::getTargetDesc(x86_64TargetInst::JMP),
                   inst->getSourceRef(),
                   { falseBlock });

    inst->eraseFromOwner();
    return true;
}

/**
 * Lowers a CALL, choosing the first reference or non-token register operand as the callee and
 * pinning it to a 64-bit GPR when unconstrained.
 */
bool X86_64TargetInstructionSelector::selectCALL(MirBuilderContext *ctx, MirInstruction *inst)
{
    if (inst->getOperandCount() < 1)
    {
        return false;
    }

    MirOperand *callee = inst->getOperand(0);
    for (size_t i = 0; i < inst->getOperandCount(); ++i)
    {
        auto *op = inst->getOperand(i);
        if (op &&
            (op->getType() == MirOperandType::Reference ||
             (op->getType() == MirOperandType::Register && op->getMirType() &&
              op->getMirType()->getKind() != MirTypeKind::BindingToken)))
        {
            callee = op;
            break;
        }
    }

    MirInstructionBuilder ib(ctx, inst, InsertionType::InsertBefore);

    if (auto *r = callee->get<MirRegister>())
    {
        if (!r->getRegClass())
        {
            r->setClass(findClass("GPR64"));
        }
    }

    ib.buildTarget(x86_64TargetInst::getTargetDesc(x86_64TargetInst::CALL),
                   inst->getSourceRef(),
                   { callee });
    inst->eraseFromOwner();
    return true;
}

/**
 * Lowers a comparison into a zeroed destination, a CMP of the two operands, and the matching
 * SETcc that materializes the boolean result.
 */
bool X86_64TargetInstructionSelector::selectCMP(MirBuilderContext *ctx, MirInstruction *inst)
{
    if (inst->getOperandCount() < 3)
    {
        return false;
    }

    auto *dst = inst->getOperand(0);
    auto *lhs = inst->getOperand(1);
    auto *rhs = inst->getOperand(2);

    MirInstructionBuilder ib(ctx, inst, InsertionType::InsertBefore);
    MirOperandBuilder ob(ctx);

    bool is64 = (lhs->getMirType() && lhs->getMirType()->getTotalSizeInBits() == 64);
    std::string_view gprClass = is64 ? "GPR64" : "GPR32";

    if (auto *r = lhs->get<MirRegister>())
    {
        if (!r->getRegClass())
        {
            r->setClass(findClass(gprClass));
        }
    }

    // 1. Zero destination register first: MOV32ri %dst, 0
    if (auto *r = dst->get<MirRegister>())
    {
        if (!r->getRegClass())
        {
            r->setClass(findClass("GPR32"));
        }
    }
    auto *zeroImm = ob.buildInt(ctx->getTypeTable()->i32(), FlexInt(static_cast<int32_t>(0)));
    ib.buildTarget(x86_64TargetInst::getTargetDesc(x86_64TargetInst::MOV32ri),
                   inst->getSourceRef(),
                   { dst, zeroImm });

    // 2. Emit CMP (CMP64ri/rr or CMP32ri/rr)
    if (rhs->getType() == MirOperandType::Integer)
    {
        auto cmpOp = is64 ? x86_64TargetInst::CMP64ri : x86_64TargetInst::CMP32ri;
        ib.buildTarget(x86_64TargetInst::getTargetDesc(cmpOp),
                       inst->getSourceRef(),
                       { lhs, rhs });
    }
    else if (rhs->getType() == MirOperandType::Register)
    {
        if (auto *r = rhs->get<MirRegister>())
        {
            if (!r->getRegClass())
            {
                r->setClass(findClass(gprClass));
            }
        }
        auto cmpOp = is64 ? x86_64TargetInst::CMP64rr : x86_64TargetInst::CMP32rr;
        ib.buildTarget(x86_64TargetInst::getTargetDesc(cmpOp),
                       inst->getSourceRef(),
                       { lhs, rhs });
    }
    else
    {
        return false;
    }

    // 3. Emit SETcc %dst
    x86_64TargetInst::OpCode setccOp = x86_64TargetInst::SETE;
    switch (inst->getOpCode())
    {
        case MirInstructionOpCode::CMP_EQ:
            setccOp = x86_64TargetInst::SETE;
            break;
        case MirInstructionOpCode::CMP_NE:
            setccOp = x86_64TargetInst::SETNE;
            break;
        case MirInstructionOpCode::CMP_SLT:
            setccOp = x86_64TargetInst::SETL;
            break;
        case MirInstructionOpCode::CMP_SLE:
            setccOp = x86_64TargetInst::SETLE;
            break;
        case MirInstructionOpCode::CMP_SGT:
            setccOp = x86_64TargetInst::SETG;
            break;
        case MirInstructionOpCode::CMP_SGE:
            setccOp = x86_64TargetInst::SETGE;
            break;
        case MirInstructionOpCode::CMP_ULT:
            setccOp = x86_64TargetInst::SETB;
            break;
        case MirInstructionOpCode::CMP_ULE:
            setccOp = x86_64TargetInst::SETBE;
            break;
        case MirInstructionOpCode::CMP_UGT:
            setccOp = x86_64TargetInst::SETA;
            break;
        case MirInstructionOpCode::CMP_UGE:
            setccOp = x86_64TargetInst::SETAE;
            break;
        default:
            return false;
    }

    ib.buildTarget(x86_64TargetInst::getTargetDesc(setccOp),
                   inst->getSourceRef(),
                   { dst });

    inst->eraseFromOwner();
    return true;
}

/**
 * Lowers a LOAD, selecting an integer/float and sized variant (8/16/32/64) from the destination
 * type, and wrapping a bare register source into [reg + 0].
 */
bool X86_64TargetInstructionSelector::selectLOAD(MirBuilderContext *ctx, MirInstruction *inst)
{
    if (inst->getOperandCount() < 2)
    {
        return false;
    }

    auto *dst = inst->getOperand(0);
    auto *src = inst->getOperand(1);

    MirInstructionBuilder ib(ctx, inst, InsertionType::InsertBefore);
    MirOperandBuilder ob(ctx);

    bool isFloat = (dst->getMirType() && dst->getMirType()->getKind() == MirTypeKind::FloatingPoint);
    size_t sizeInBits = dst->getMirType() ? dst->getMirType()->getTotalSizeInBits() : 64;
    x86_64TargetInst::OpCode loadOp = x86_64TargetInst::LOAD64;
    std::string_view dstClass = "GPR64";

    if (isFloat)
    {
        if (sizeInBits == 32)
        {
            loadOp = x86_64TargetInst::LOAD32;
            dstClass = "FPR32";
        }
        else
        {
            loadOp = x86_64TargetInst::LOAD64;
            dstClass = "FPR64";
        }
    }
    else
    {
        if (sizeInBits <= 8)
        {
            loadOp = x86_64TargetInst::LOAD8;
            dstClass = "GPR8";
        }
        else if (sizeInBits <= 16)
        {
            loadOp = x86_64TargetInst::LOAD16;
            dstClass = "GPR16";
        }
        else if (sizeInBits <= 32)
        {
            loadOp = x86_64TargetInst::LOAD32;
            dstClass = "GPR32";
        }
    }

    if (auto *r = dst->get<MirRegister>())
    {
        if (!r->getRegClass())
        {
            r->setClass(findClass(dstClass));
        }
    }

    MirOperand *memOp = src;
    if (auto *r = src->get<MirRegister>())
    {
        if (!r->getRegClass())
        {
            r->setClass(findClass("GPR64"));
        }
        auto *disp0 = ob.buildInt(ctx->getTypeTable()->i32(), FlexInt(static_cast<int32_t>(0)));
        memOp = ob.buildMem(ctx->getTypeTable()->i32(), r, disp0);
    }
    else if (auto *m = src->get<MirMemory>())
    {
        if (m->getBase() && !m->getBase()->getRegClass())
        {
            m->getBase()->setClass(findClass("GPR64"));
        }
        if (m->getIndex() && !m->getIndex()->getRegClass())
        {
            m->getIndex()->setClass(findClass("GPR64"));
        }
    }

    ib.buildTarget(x86_64TargetInst::getTargetDesc(loadOp),
                   inst->getSourceRef(),
                   { dst, memOp });
    inst->eraseFromOwner();
    return true;
}

/**
 * Lowers a STORE, selecting an integer/float and sized variant from the value type, and wrapping
 * a bare register destination into [reg + 0].
 */
bool X86_64TargetInstructionSelector::selectSTORE(MirBuilderContext *ctx, MirInstruction *inst)
{
    if (inst->getOperandCount() < 2)
    {
        return false;
    }

    auto *dest = inst->getOperand(0);
    auto *val = inst->getOperand(1);

    MirInstructionBuilder ib(ctx, inst, InsertionType::InsertBefore);
    MirOperandBuilder ob(ctx);

    bool isFloat = (val->getMirType() && val->getMirType()->getKind() == MirTypeKind::FloatingPoint);
    size_t sizeInBits = val->getMirType() ? val->getMirType()->getTotalSizeInBits() : 64;
    x86_64TargetInst::OpCode storeOp = x86_64TargetInst::STORE64;
    std::string_view valClass = "GPR64";

    if (isFloat)
    {
        if (sizeInBits == 32)
        {
            storeOp = x86_64TargetInst::STORE32;
            valClass = "FPR32";
        }
        else
        {
            storeOp = x86_64TargetInst::STORE64;
            valClass = "FPR64";
        }
    }
    else
    {
        if (sizeInBits <= 8)
        {
            storeOp = x86_64TargetInst::STORE8;
            valClass = "GPR8";
        }
        else if (sizeInBits <= 16)
        {
            storeOp = x86_64TargetInst::STORE16;
            valClass = "GPR16";
        }
        else if (sizeInBits <= 32)
        {
            storeOp = x86_64TargetInst::STORE32;
            valClass = "GPR32";
        }
    }

    if (auto *r = val->get<MirRegister>())
    {
        if (!r->getRegClass())
        {
            r->setClass(findClass(valClass));
        }
    }

    MirOperand *memOp = dest;
    if (auto *r = dest->get<MirRegister>())
    {
        if (!r->getRegClass())
        {
            r->setClass(findClass("GPR64"));
        }
        auto *disp0 = ob.buildInt(ctx->getTypeTable()->i32(), FlexInt(static_cast<int32_t>(0)));
        memOp = ob.buildMem(ctx->getTypeTable()->i32(), r, disp0);
    }
    else if (auto *m = dest->get<MirMemory>())
    {
        if (m->getBase() && !m->getBase()->getRegClass())
        {
            m->getBase()->setClass(findClass("GPR64"));
        }
        if (m->getIndex() && !m->getIndex()->getRegClass())
        {
            m->getIndex()->setClass(findClass("GPR64"));
        }
    }

    ib.buildTarget(x86_64TargetInst::getTargetDesc(storeOp),
                   inst->getSourceRef(),
                   { memOp, val });
    inst->eraseFromOwner();
    return true;
}

/**
 * Eliminates a PHI by inserting a MOV into the destination virtual register at the end of each
 * predecessor block, in block-id order matching the incoming operand slots. Erases the PHI,
 * including when its destination is unused or undefined.
 */
bool X86_64TargetInstructionSelector::selectPHI(MirBuilderContext *ctx, MirInstruction *inst)
{
    if (inst->getOperandCount() < 1)
    {
        return false;
    }

    auto *dst = inst->getOperand(0)->get<MirRegister>();
    if (!dst)
    {
        inst->eraseFromOwner();
        return true;
    }

    MirBlock *currBlock = inst->getOwner();
    MirFunction *func = currBlock ? currBlock->getOwner() : nullptr;
    if (!func)
    {
        inst->eraseFromOwner();
        return true;
    }

    // Check if dst is actually used anywhere in the function. The SSA register tracker already
    // holds the use list, avoiding a full instruction scan per PHI.
    MirFunctionRegisterInfo *regInfo = func->getRegisterInfo();
    bool isUsed = false;
    if (regInfo)
    {
        isUsed = regInfo->getUseCount(dst->getRegId()) > 0;
    }
    else
    {
        for (MirBlock *b : func->getBlocks())
        {
            if (!b)
                continue;
            for (MirInstruction *i : b->getInstructions())
            {
                if (i == inst)
                    continue;
                for (MirOperand *op : i->getOperands())
                {
                    if (op && op->isOfType<MirRegister>() && op->get<MirRegister>()->getRegId() == dst->getRegId())
                    {
                        isUsed = true;
                        break;
                    }
                }
                if (isUsed)
                    break;
            }
            if (isUsed)
                break;
        }
    }

    if (!isUsed)
    {
        inst->eraseFromOwner();
        return true;
    }

    // Set register class for dst
    size_t sizeInBits = dst->getMirType() ? dst->getMirType()->getTotalSizeInBits() : 64;
    std::string_view gprClass = (sizeInBits == 64) ? "GPR64" : "GPR32";
    if (!dst->getRegClass())
    {
        dst->setClass(findClass(gprClass));
    }

    // Use the CFG's explicit predecessor metadata. Operand 1 + i corresponds to the i-th
    // predecessor in ascending MirId order, exactly as NonSsaToSsaPass filled the PHI slots.
    const auto &predecessorBlocks = currBlock->getPredecessors();

    size_t predIdx = 0;
    for (MirBlock *predBlock : predecessorBlocks)
    {
        size_t opIdx = 1 + predIdx;
        predIdx++;

        if (!predBlock || opIdx >= inst->getOperandCount())
        {
            continue;
        }

        MirOperand *incoming = inst->getOperand(opIdx);
        auto *inReg = incoming ? incoming->get<MirRegister>() : nullptr;
        if (!inReg)
        {
            continue;
        }

        // An undefined virtual register (an SSA "undef" phantom) has no reaching definition, so
        // there is nothing to move; detect it from the SSA metadata rather than the register name.
        bool isParam = false;
        if (func)
        {
            for (MirRegister *param : func->getParameters())
            {
                if (param && param->getRegId() == inReg->getRegId())
                {
                    isParam = true;
                    break;
                }
            }
        }
        const bool isUndefined = inReg->getName() == "undef" ||
                (!isParam && inReg->isVirtual() && regInfo && regInfo->getDef(inReg->getRegId()) == nullptr);
        if (isUndefined)
        {
            continue;
        }

        if (!inReg->getRegClass())
        {
            inReg->setClass(findClass(gprClass));
        }

        // Insert MOV %dst, %inReg before the first branch / jump instruction in predBlock
        auto it = predBlock->getInstructions().end();
        for (auto bit = predBlock->getInstructions().begin(); bit != predBlock->getInstructions().end(); ++bit)
        {
            MirInstructionFlags flags = (*bit)->getFlags();
            if ((*bit)->getTargetDesc())
            {
                flags = (*bit)->getTargetDesc()->getTargetFlags();
            }
            bool isBr = bool(flags & MirInstructionFlags::IsBranch) ||
                    bool(flags & MirInstructionFlags::IsTerminator) ||
                    ((*bit)->getOpCode() == MirInstructionOpCode::JMP) ||
                    ((*bit)->getOpCode() == MirInstructionOpCode::BR_COND);
            if (isBr)
            {
                it = bit;
                break;
            }
        }
        MirInstructionBuilder pib(ctx, predBlock, InsertionType::InsertBefore, it);
        pib.MOV(dst, inReg);
    }

    inst->eraseFromOwner();
    return true;
}

/**
 * Lowers a scalar floating-point arithmetic instruction to the matching SSE opcode, choosing the
 * single- or double-precision form from the destination type.
 */
bool X86_64TargetInstructionSelector::selectFloatALU(MirBuilderContext *ctx, MirInstruction *inst)
{
    if (inst->getOperandCount() < 3)
    {
        return false;
    }

    auto *dst = inst->getOperand(0);
    auto *lhs = inst->getOperand(1);
    auto *rhs = inst->getOperand(2);

    bool isDouble = (dst->getMirType() && dst->getMirType()->getTotalSizeInBits() == 64);
    std::string_view fprClass = isDouble ? "FPR64" : "FPR32";

    if (auto *r = dst->get<MirRegister>())
    {
        if (!r->getRegClass())
            r->setClass(findClass(fprClass));
    }
    if (auto *r = lhs->get<MirRegister>())
    {
        if (!r->getRegClass())
            r->setClass(findClass(fprClass));
    }
    if (auto *r = rhs->get<MirRegister>())
    {
        if (!r->getRegClass())
            r->setClass(findClass(fprClass));
    }

    x86_64TargetInst::OpCode op = x86_64TargetInst::ADDSS;
    switch (inst->getOpCode())
    {
        case MirInstructionOpCode::FADD:
            op = isDouble ? x86_64TargetInst::ADDSD : x86_64TargetInst::ADDSS;
            break;
        case MirInstructionOpCode::FSUB:
            op = isDouble ? x86_64TargetInst::SUBSD : x86_64TargetInst::SUBSS;
            break;
        case MirInstructionOpCode::FMUL:
            op = isDouble ? x86_64TargetInst::MULSD : x86_64TargetInst::MULSS;
            break;
        case MirInstructionOpCode::FDIV:
            op = isDouble ? x86_64TargetInst::DIVSD : x86_64TargetInst::DIVSS;
            break;
        default:
            return false;
    }

    MirInstructionBuilder ib(ctx, inst, InsertionType::InsertBefore);
    ib.buildTarget(x86_64TargetInst::getTargetDesc(op),
                   inst->getSourceRef(),
                   { dst, lhs, rhs });
    inst->eraseFromOwner();
    return true;
}

/**
 * Lowers integer-to-float (SITOFP) and float-to-integer (FPTOSI) conversions to CVTSI2SS/SD and
 * CVTTSS2SI/CVTTSD2SI, picking the GPR/FPR classes from the operand widths.
 */
bool X86_64TargetInstructionSelector::selectFloatCvt(MirBuilderContext *ctx, MirInstruction *inst)
{
    if (inst->getOperandCount() < 2)
    {
        return false;
    }

    auto *dst = inst->getOperand(0);
    auto *src = inst->getOperand(1);

    MirInstructionBuilder ib(ctx, inst, InsertionType::InsertBefore);

    if (inst->getOpCode() == MirInstructionOpCode::SITOFP)
    {
        bool isDstDouble = (dst->getMirType() && dst->getMirType()->getTotalSizeInBits() == 64);
        bool isSrc64 = (src->getMirType() && src->getMirType()->getTotalSizeInBits() == 64);

        if (auto *r = dst->get<MirRegister>())
        {
            if (!r->getRegClass())
                r->setClass(findClass(isDstDouble ? "FPR64" : "FPR32"));
        }
        if (auto *r = src->get<MirRegister>())
        {
            if (!r->getRegClass())
                r->setClass(findClass(isSrc64 ? "GPR64" : "GPR32"));
        }

        auto op = isDstDouble ? x86_64TargetInst::CVTSI2SD : x86_64TargetInst::CVTSI2SS;
        ib.buildTarget(x86_64TargetInst::getTargetDesc(op),
                       inst->getSourceRef(),
                       { dst, src });
    }
    else if (inst->getOpCode() == MirInstructionOpCode::FPTOSI)
    {
        bool isSrcDouble = (src->getMirType() && src->getMirType()->getTotalSizeInBits() == 64);
        bool isDst64 = (dst->getMirType() && dst->getMirType()->getTotalSizeInBits() == 64);

        if (auto *r = dst->get<MirRegister>())
        {
            if (!r->getRegClass())
                r->setClass(findClass(isDst64 ? "GPR64" : "GPR32"));
        }
        if (auto *r = src->get<MirRegister>())
        {
            if (!r->getRegClass())
                r->setClass(findClass(isSrcDouble ? "FPR64" : "FPR32"));
        }

        auto op = isSrcDouble ? x86_64TargetInst::CVTTSD2SI : x86_64TargetInst::CVTTSS2SI;
        ib.buildTarget(x86_64TargetInst::getTargetDesc(op),
                       inst->getSourceRef(),
                       { dst, src });
    }
    else
    {
        return false;
    }

    inst->eraseFromOwner();
    return true;
}

/**
 * Lowers a MOV: address-of global/stack references become LEA64r, float moves use
 * MOVSSrr/MOVSDrr, and integer moves use MOV32/64rr or MOV32/64ri.
 */
bool X86_64TargetInstructionSelector::selectMOV(MirBuilderContext *ctx, MirInstruction *inst)
{
    if (inst->getOperandCount() < 2)
    {
        return false;
    }

    auto *dst = inst->getOperand(0);
    auto *src = inst->getOperand(1);

    MirInstructionBuilder ib(ctx, inst, InsertionType::InsertBefore);

    // 1. Address-of global variable or stack slot: MOV %dst, @ref -> LEA64r %dst, @ref
    if (src->getType() == MirOperandType::Reference)
    {
        auto *ref = src->get<MirReference>();
        if (ref && (ref->isGlobalVar() || ref->isStackFrameObject()))
        {
            if (auto *r = dst->get<MirRegister>())
            {
                if (!r->getRegClass())
                    r->setClass(findClass("GPR64"));
            }
            ib.buildTarget(
                    x86_64TargetInst::getTargetDesc(x86_64TargetInst::LEA64r),
                    inst->getSourceRef(),
                    { dst, src });
            inst->eraseFromOwner();
            return true;
        }
    }

    // 2. Floating point register moves: MOVSSrr / MOVSDrr
    bool isFloat = (dst->getMirType() && dst->getMirType()->getKind() == MirTypeKind::FloatingPoint);
    if (isFloat)
    {
        bool isDouble = (dst->getMirType()->getTotalSizeInBits() == 64);
        std::string_view fprClass = isDouble ? "FPR64" : "FPR32";
        if (auto *r = dst->get<MirRegister>())
        {
            if (!r->getRegClass())
                r->setClass(findClass(fprClass));
        }
        if (auto *r = src->get<MirRegister>())
        {
            if (!r->getRegClass())
                r->setClass(findClass(fprClass));
        }

        auto op = isDouble ? x86_64TargetInst::MOVSDrr : x86_64TargetInst::MOVSSrr;
        ib.buildTarget(x86_64TargetInst::getTargetDesc(op),
                       inst->getSourceRef(),
                       { dst, src });
        inst->eraseFromOwner();
        return true;
    }

    // 3. Fallback for general register-to-register or integer immediate
    size_t sizeInBits = dst->getMirType() ? dst->getMirType()->getTotalSizeInBits() : 64;
    std::string_view gprClass = (sizeInBits == 64) ? "GPR64" : "GPR32";
    if (auto *r = dst->get<MirRegister>())
    {
        if (!r->getRegClass())
            r->setClass(findClass(gprClass));
    }

    if (src->getType() == MirOperandType::Register)
    {
        if (auto *r = src->get<MirRegister>())
        {
            if (!r->getRegClass())
                r->setClass(findClass(gprClass));
        }
        auto op = (sizeInBits == 64) ? x86_64TargetInst::MOV64rr : x86_64TargetInst::MOV32rr;
        ib.buildTarget(x86_64TargetInst::getTargetDesc(op),
                       inst->getSourceRef(),
                       { dst, src });
        inst->eraseFromOwner();
        return true;
    }
    else if (src->getType() == MirOperandType::Integer)
    {
        auto op = (sizeInBits == 64) ? x86_64TargetInst::MOV64ri : x86_64TargetInst::MOV32ri;
        ib.buildTarget(x86_64TargetInst::getTargetDesc(op),
                       inst->getSourceRef(),
                       { dst, src });
        inst->eraseFromOwner();
        return true;
    }

    return false;
}

} // namespace EzTargets::X86_64
