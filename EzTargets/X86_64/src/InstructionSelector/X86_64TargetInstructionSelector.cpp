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
#include "Function/MirFunctionBuilder.h"
#include "Function/MirFunctionRegisterInfo.h"
#include "Function/CallingConvDesc.h"
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

    switch (inst->getOpCode())
    {
        case MirInstructionOpCode::MOV:
        case MirInstructionOpCode::MOV_ADDR:
            return selectMOV(ctx, inst);
        case MirInstructionOpCode::JMP:
            return selectJMP(ctx, inst);
        case MirInstructionOpCode::BR_COND:
            return selectBR_COND(ctx, inst);
        case MirInstructionOpCode::TRY:
            return selectTRY(ctx, inst);
        case MirInstructionOpCode::PUSH_REGS:
            return selectPUSH_REGS(ctx, inst);
        case MirInstructionOpCode::POP_REGS:
            return selectPOP_REGS(ctx, inst);
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
        case MirInstructionOpCode::TRUNC:
            return selectTRUNC(ctx, inst);
        case MirInstructionOpCode::SEXT:
            return selectSEXT(ctx, inst);
        case MirInstructionOpCode::ZEXT:
            return selectZEXT(ctx, inst);
        case MirInstructionOpCode::SDIV:
        case MirInstructionOpCode::IDIV:
            return selectDIV(ctx, inst, /*isSigned=*/true, /*isRem=*/false);
        case MirInstructionOpCode::UDIV:
        case MirInstructionOpCode::DIV:
            return selectDIV(ctx, inst, /*isSigned=*/false, /*isRem=*/false);
        case MirInstructionOpCode::REM:
            return selectDIV(ctx, inst, /*isSigned=*/true, /*isRem=*/true);
        case MirInstructionOpCode::SHL:
        case MirInstructionOpCode::SHR:
        case MirInstructionOpCode::SAR:
            return selectSHIFT(ctx, inst);
        case MirInstructionOpCode::SUB:
            return selectSUB(ctx, inst);
        case MirInstructionOpCode::ADD:
            return selectADD(ctx, inst);
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
 * Lowers a TRY instruction into an unconditional branch to the protected body block.
 */
bool X86_64TargetInstructionSelector::selectTRY(MirBuilderContext *ctx, MirInstruction *inst)
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

    bool is64 = false;
    if (auto *r = cond->get<MirRegister>())
    {
        if (r->getRegClass())
        {
            is64 = (r->getRegClass() == findClass("GPR64"));
        }
        else if (r->getMirType() && r->getMirType()->getTotalSizeInBits() == 64)
        {
            is64 = true;
            r->setClass(findClass("GPR64"));
        }
        else
        {
            r->setClass(findClass("GPR32"));
        }
    }
    else if (cond->getMirType() && cond->getMirType()->getTotalSizeInBits() == 64)
    {
        is64 = true;
    }

    auto cmpOp = is64 ? x86_64TargetInst::CMP64ri : x86_64TargetInst::CMP32ri;
    auto *zeroImm = ob.buildInt(is64 ? ctx->getTypeTable()->i64() : ctx->getTypeTable()->i32(),
                                FlexInt(0, is64 ? 64 : 32));

    // Emit: CMP %cond, 0
    ib.buildTarget(x86_64TargetInst::getTargetDesc(cmpOp),
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
        if (!op)
        {
            continue;
        }

        if (op->getType() == MirOperandType::Reference)
        {
            callee = op;
            break;
        }

        if (op->getType() == MirOperandType::RuntimeSymbol)
        {
            auto *rtSym = static_cast<MirRuntimeSymbol *>(op);
            std::string_view symName = rtSym->getSymbolName();
            MirFunction *fn = nullptr;
            for (MirFunction *f : ctx->getFunctions())
            {
                if (f->getName() == symName)
                {
                    fn = f;
                    break;
                }
            }
            if (!fn)
            {
                MirFunctionBuilder fb(ctx);
                fn = fb.declare(ctx->getTypeTable()->_void(), std::initializer_list<MirType*>{}, symName, MirLinkage::External);
            }
            MirOperandBuilder ob(ctx);
            callee = ob.buildRef(fn);
            break;
        }

        if (op->getType() == MirOperandType::Register && op->getMirType() &&
            op->getMirType()->getKind() != MirTypeKind::BindingToken)
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
        auto *imm = rhs->get<MirInteger>();
        int64_t val = imm->getValue().getI64();
        if (is64 && (val < -2147483648LL || val > 2147483647LL))
        {
            MirRegister *vreg = ob.buildVReg(ctx->getTypeTable()->i64(), "imm64");
            vreg->setClass(findClass("GPR64"));
            ib.buildTarget(x86_64TargetInst::getTargetDesc(x86_64TargetInst::MOV64ri),
                           inst->getSourceRef(),
                           { vreg, rhs });
            rhs = vreg;
            auto cmpOp = x86_64TargetInst::CMP64rr;
            ib.buildTarget(x86_64TargetInst::getTargetDesc(cmpOp),
                           inst->getSourceRef(),
                           { lhs, rhs });
        }
        else
        {
            auto cmpOp = is64 ? x86_64TargetInst::CMP64ri : x86_64TargetInst::CMP32ri;
            ib.buildTarget(x86_64TargetInst::getTargetDesc(cmpOp),
                           inst->getSourceRef(),
                           { lhs, rhs });
        }
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

    if (!val->isOfType<MirRegister>())
    {
        MirType *valType = val->getMirType() ? val->getMirType() : ctx->getTypeTable()->i32();
        MirRegister *valReg = ob.buildVReg(valType, "store_val");
        valReg->setClass(findClass(valClass));
        ib.MOV(valReg, val);
        val = valReg;
    }
    else if (auto *r = val->get<MirRegister>())
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

    // 1. Address-of global variable, function, or stack slot: MOV %dst, @ref -> LEA64r %dst, @ref
    if (src->getType() == MirOperandType::Reference)
    {
        auto *ref = src->get<MirReference>();
        if (ref && (ref->isGlobalVar() || ref->isStackFrameObject() || ref->isFunction()))
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
    else if (src->getType() == MirOperandType::RuntimeSymbol)
    {
        auto *rtSym = static_cast<MirRuntimeSymbol *>(src);
        std::string_view symName = rtSym->getSymbolName();
        MirFunction *fn = nullptr;
        for (MirFunction *f : ctx->getFunctions())
        {
            if (f->getName() == symName)
            {
                fn = f;
                break;
            }
        }
        if (!fn)
        {
            MirFunctionBuilder fb(ctx);
            fn = fb.declare(ctx->getTypeTable()->_void(), std::initializer_list<MirType*>{}, symName, MirLinkage::External);
        }
        MirOperandBuilder ob(ctx);
        MirOperand *ref = ob.buildRef(fn);

        if (auto *r = dst->get<MirRegister>())
        {
            if (!r->getRegClass())
                r->setClass(findClass("GPR64"));
        }
        ib.buildTarget(
                x86_64TargetInst::getTargetDesc(x86_64TargetInst::LEA64r),
                inst->getSourceRef(),
                { dst, ref });
        inst->eraseFromOwner();
        return true;
    }

    // 2. Floating point register moves: MOVSSrr / MOVSDrr
    bool isFloat = (dst->getMirType() && dst->getMirType()->getKind() == MirTypeKind::FloatingPoint);

    if (isFloat)
    {
        bool isDouble = (dst->getMirType()->getTotalSizeInBits() == 64);
        std::string_view fprClass = isDouble ? "FPR64" : "FPR32";
        if (auto *r = dst->get<MirRegister>())
        {
            if (!r->getRegClass() || (r->getRegClass()->getName() != fprClass && r->getRegClass()->getName() != "VR128"))
                r->setClass(findClass(fprClass));
        }
        if (auto *r = src->get<MirRegister>())
        {
            if (!r->getRegClass() || (r->getRegClass()->getName() != fprClass && r->getRegClass()->getName() != "VR128"))
                r->setClass(findClass(fprClass));
        }

        auto op = isDouble ? x86_64TargetInst::MOVSDrr : x86_64TargetInst::MOVSSrr;
        ib.buildTarget(x86_64TargetInst::getTargetDesc(op),
                       inst->getSourceRef(),
                       { dst, src });
        inst->eraseFromOwner();
        return true;
    }

    // 2b. Vector register moves: MOVAPSrr
    bool isVector = (dst->getMirType() && dst->getMirType()->getKind() == MirTypeKind::Vector);
    if (isVector)
    {
        if (auto *r = dst->get<MirRegister>())
        {
            if (!r->getRegClass() || r->getRegClass()->getName() != std::string_view("VR128"))
                r->setClass(findClass("VR128"));
        }
        if (auto *r = src->get<MirRegister>())
        {
            if (!r->getRegClass() || r->getRegClass()->getName() != std::string_view("VR128"))
                r->setClass(findClass("VR128"));
        }

        ib.buildTarget(x86_64TargetInst::getTargetDesc(x86_64TargetInst::MOVAPSrr),
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

bool X86_64TargetInstructionSelector::selectTRUNC(MirBuilderContext *ctx, MirInstruction *inst)
{
    if (inst->getOperandCount() < 2)
        return false;

    auto *dst = inst->getOperand(0);
    auto *src = inst->getOperand(1);

    MirInstructionBuilder ib(ctx, inst, InsertionType::InsertBefore);
    size_t dstBits = dst->getMirType() ? dst->getMirType()->getTotalSizeInBits() : 32;

    if (src->getType() == MirOperandType::Integer)
    {
        auto op = (dstBits <= 8)  ? x86_64TargetInst::MOV8ri
                : (dstBits <= 16) ? x86_64TargetInst::MOV16ri
                : (dstBits <= 32) ? x86_64TargetInst::MOV32ri
                                  : x86_64TargetInst::MOV64ri;
        std::string_view gprClass = (dstBits <= 8)  ? "GPR8"
                                  : (dstBits <= 16) ? "GPR16"
                                  : (dstBits <= 32) ? "GPR32"
                                                    : "GPR64";
        if (auto *r = dst->get<MirRegister>())
        {
            if (!r->getRegClass())
                r->setClass(findClass(gprClass));
        }
        ib.buildTarget(x86_64TargetInst::getTargetDesc(op), inst->getSourceRef(), { dst, src });
        inst->eraseFromOwner();
        return true;
    }

    auto op = (dstBits <= 8)  ? x86_64TargetInst::MOV8rr
            : (dstBits <= 16) ? x86_64TargetInst::MOV16rr
            : (dstBits <= 32) ? x86_64TargetInst::MOV32rr
                              : x86_64TargetInst::MOV64rr;
    std::string_view regClass = (dstBits <= 8)  ? "GPR8"
                              : (dstBits <= 16) ? "GPR16"
                              : (dstBits <= 32) ? "GPR32"
                                                : "GPR64";

    if (auto *r = dst->get<MirRegister>())
    {
        if (!r->getRegClass())
            r->setClass(findClass(regClass));
    }
    if (auto *r = src->get<MirRegister>())
    {
        if (!r->getRegClass())
            r->setClass(findClass(regClass));
    }

    ib.buildTarget(x86_64TargetInst::getTargetDesc(op), inst->getSourceRef(), { dst, src });
    inst->eraseFromOwner();
    return true;
}

bool X86_64TargetInstructionSelector::selectSEXT(MirBuilderContext *ctx, MirInstruction *inst)
{
    if (inst->getOperandCount() < 2)
        return false;

    auto *dst = inst->getOperand(0);
    auto *src = inst->getOperand(1);

    MirInstructionBuilder ib(ctx, inst, InsertionType::InsertBefore);
    size_t dstBits = dst->getMirType() ? dst->getMirType()->getTotalSizeInBits() : 64;
    size_t srcBits = src->getMirType() ? src->getMirType()->getTotalSizeInBits() : 32;

    if (src->getType() == MirOperandType::Integer)
    {
        auto op = (dstBits == 64) ? x86_64TargetInst::MOV64ri : x86_64TargetInst::MOV32ri;
        std::string_view gprClass = (dstBits == 64) ? "GPR64" : "GPR32";
        if (auto *r = dst->get<MirRegister>())
        {
            if (!r->getRegClass())
                r->setClass(findClass(gprClass));
        }
        ib.buildTarget(x86_64TargetInst::getTargetDesc(op), inst->getSourceRef(), { dst, src });
        inst->eraseFromOwner();
        return true;
    }

    x86_64TargetInst::OpCode op = x86_64TargetInst::MOVSX64_32;
    std::string_view dstClass = "GPR64";
    std::string_view srcClass = "GPR32";

    if (dstBits > 32)
    {
        dstClass = "GPR64";
        if (srcBits <= 8)
        {
            op = x86_64TargetInst::MOVSX64_8;
            srcClass = "GPR8";
        }
        else if (srcBits <= 16)
        {
            op = x86_64TargetInst::MOVSX64_16;
            srcClass = "GPR16";
        }
        else
        {
            op = x86_64TargetInst::MOVSX64_32;
            srcClass = "GPR32";
        }
    }
    else
    {
        dstClass = "GPR32";
        if (srcBits <= 8)
        {
            op = x86_64TargetInst::MOVSX32_8;
            srcClass = "GPR8";
        }
        else
        {
            op = x86_64TargetInst::MOVSX32_16;
            srcClass = "GPR16";
        }
    }

    if (auto *r = dst->get<MirRegister>())
    {
        if (!r->getRegClass())
            r->setClass(findClass(dstClass));
    }
    if (auto *r = src->get<MirRegister>())
    {
        if (!r->getRegClass())
            r->setClass(findClass(srcClass));
    }

    ib.buildTarget(x86_64TargetInst::getTargetDesc(op), inst->getSourceRef(), { dst, src });
    inst->eraseFromOwner();
    return true;
}

bool X86_64TargetInstructionSelector::selectZEXT(MirBuilderContext *ctx, MirInstruction *inst)
{
    if (inst->getOperandCount() < 2)
        return false;

    auto *dst = inst->getOperand(0);
    auto *src = inst->getOperand(1);

    MirInstructionBuilder ib(ctx, inst, InsertionType::InsertBefore);
    size_t dstBits = dst->getMirType() ? dst->getMirType()->getTotalSizeInBits() : 64;
    size_t srcBits = src->getMirType() ? src->getMirType()->getTotalSizeInBits() : 32;

    if (src->getType() == MirOperandType::Integer)
    {
        auto op = (dstBits == 64) ? x86_64TargetInst::MOV64ri : x86_64TargetInst::MOV32ri;
        std::string_view gprClass = (dstBits == 64) ? "GPR64" : "GPR32";
        if (auto *r = dst->get<MirRegister>())
        {
            if (!r->getRegClass())
                r->setClass(findClass(gprClass));
        }
        ib.buildTarget(x86_64TargetInst::getTargetDesc(op), inst->getSourceRef(), { dst, src });
        inst->eraseFromOwner();
        return true;
    }

    x86_64TargetInst::OpCode op = x86_64TargetInst::MOV32rr;
    std::string_view dstClass = "GPR64";
    std::string_view srcClass = "GPR32";

    if (dstBits > 32)
    {
        dstClass = "GPR64";
        if (srcBits <= 8)
        {
            op = x86_64TargetInst::MOVZX64_8;
            srcClass = "GPR8";
        }
        else if (srcBits <= 16)
        {
            op = x86_64TargetInst::MOVZX64_16;
            srcClass = "GPR16";
        }
        else
        {
            op = x86_64TargetInst::MOV32rr;
            dstClass = "GPR32";
            srcClass = "GPR32";
        }
    }
    else
    {
        dstClass = "GPR32";
        if (srcBits <= 8)
        {
            op = x86_64TargetInst::MOVZX32_8;
            srcClass = "GPR8";
        }
        else
        {
            op = x86_64TargetInst::MOVZX32_16;
            srcClass = "GPR16";
        }
    }

    if (auto *r = dst->get<MirRegister>())
    {
        if (!r->getRegClass())
            r->setClass(findClass(dstClass));
    }
    if (auto *r = src->get<MirRegister>())
    {
        if (!r->getRegClass())
            r->setClass(findClass(srcClass));
    }

    ib.buildTarget(x86_64TargetInst::getTargetDesc(op), inst->getSourceRef(), { dst, src });
    inst->eraseFromOwner();
    return true;
}

/**
 * Lowers an integer division or remainder instruction (SDIV, IDIV, UDIV, DIV, REM) into
 * x86-64 IDIV32r/IDIV64r or DIV32r/DIV64r using fixed registers RAX/EAX and RDX/EDX.
 */
bool X86_64TargetInstructionSelector::selectDIV(MirBuilderContext *ctx, MirInstruction *inst, bool isSigned, bool isRem)
{
    if (inst->getOperandCount() < 3)
    {
        return false;
    }

    auto *dst = inst->getOperand(0);
    auto *lhs = inst->getOperand(1);
    auto *rhs = inst->getOperand(2);

    bool is64 = false;
    if (dst->getMirType() && dst->getMirType()->getTotalSizeInBits() == 64)
    {
        is64 = true;
    }
    else if (lhs->getMirType() && lhs->getMirType()->getTotalSizeInBits() == 64)
    {
        is64 = true;
    }

    MirType *type = is64 ? ctx->getTypeTable()->i64() : ctx->getTypeTable()->i32();
    std::string_view gprClass = is64 ? "GPR64" : "GPR32";

    if (auto *r = dst->get<MirRegister>())
    {
        if (!r->getRegClass())
        {
            r->setClass(findClass(gprClass));
        }
    }

    MirOperandBuilder ob(ctx);
    MirInstructionBuilder ib(ctx, inst, InsertionType::InsertBefore);

    // Register IDs from x86_64.tdesc: RAX = 0, RDX = 2
    MirRegister *raxPhys = ob.buildPhysReg(type, 0, is64 ? "rax" : "eax", findClass(gprClass));
    MirRegister *rdxPhys = ob.buildPhysReg(type, 2, is64 ? "rdx" : "edx", findClass(gprClass));

    // 1. Load dividend (lhs) into RAX / EAX
    if (lhs->getType() == MirOperandType::Integer)
    {
        auto movOp = is64 ? x86_64TargetInst::MOV64ri : x86_64TargetInst::MOV32ri;
        ib.buildTarget(x86_64TargetInst::getTargetDesc(movOp), inst->getSourceRef(), { raxPhys, lhs });
    }
    else
    {
        if (auto *r = lhs->get<MirRegister>())
        {
            if (!r->getRegClass())
            {
                r->setClass(findClass(gprClass));
            }
        }
        auto movOp = is64 ? x86_64TargetInst::MOV64rr : x86_64TargetInst::MOV32rr;
        ib.buildTarget(x86_64TargetInst::getTargetDesc(movOp), inst->getSourceRef(), { raxPhys, lhs });
    }

    // 2. Set up RDX / EDX
    if (isSigned)
    {
        // Sign-extend EAX/RAX into EDX/RDX: copy to EDX, then SAR by 31 (or 63)
        auto movOp = is64 ? x86_64TargetInst::MOV64rr : x86_64TargetInst::MOV32rr;
        ib.buildTarget(x86_64TargetInst::getTargetDesc(movOp), inst->getSourceRef(), { rdxPhys, raxPhys });

        auto sarOp = is64 ? x86_64TargetInst::SAR64ri : x86_64TargetInst::SAR32ri;
        auto *shiftAmt = ob.buildInt(ctx->getTypeTable()->i8(), FlexInt(is64 ? 63 : 31, 8));
        ib.buildTarget(x86_64TargetInst::getTargetDesc(sarOp), inst->getSourceRef(), { rdxPhys, rdxPhys, shiftAmt });
    }
    else
    {
        // Zero RDX / EDX
        auto movOp = is64 ? x86_64TargetInst::MOV64ri : x86_64TargetInst::MOV32ri;
        auto *zeroImm = ob.buildInt(type, FlexInt(0, is64 ? 64 : 32));
        ib.buildTarget(x86_64TargetInst::getTargetDesc(movOp), inst->getSourceRef(), { rdxPhys, zeroImm });
    }

    // 3. Ensure divisor (rhs) is in a register (IDIV does not take immediate operands)
    if (rhs->getType() == MirOperandType::Integer)
    {
        MirRegister *rhsReg = ob.buildVReg(type, "div_rhs");
        rhsReg->setClass(findClass(gprClass));
        auto movOp = is64 ? x86_64TargetInst::MOV64ri : x86_64TargetInst::MOV32ri;
        ib.buildTarget(x86_64TargetInst::getTargetDesc(movOp), inst->getSourceRef(), { rhsReg, rhs });
        rhs = rhsReg;
    }
    else if (auto *r = rhs->get<MirRegister>())
    {
        if (!r->getRegClass())
        {
            r->setClass(findClass(gprClass));
        }
    }

    // 4. Emit IDIV / DIV
    auto idivOp = isSigned
            ? (is64 ? x86_64TargetInst::IDIV64r : x86_64TargetInst::IDIV32r)
            : (is64 ? x86_64TargetInst::DIV64r : x86_64TargetInst::DIV32r);
    ib.buildTarget(x86_64TargetInst::getTargetDesc(idivOp), inst->getSourceRef(), { rhs });

    // 5. Retrieve result into dst (quotient in RAX/EAX, remainder in RDX/EDX)
    auto movOp = is64 ? x86_64TargetInst::MOV64rr : x86_64TargetInst::MOV32rr;
    if (isRem)
    {
        ib.buildTarget(x86_64TargetInst::getTargetDesc(movOp), inst->getSourceRef(), { dst, rdxPhys });
        // Touch raxPhys to keep it live across IDIV so rhs cannot collide with rax
        MirRegister *dummy = ob.buildVReg(type, "div_clobber");
        dummy->setClass(findClass(gprClass));
        ib.buildTarget(x86_64TargetInst::getTargetDesc(movOp), inst->getSourceRef(), { dummy, raxPhys });
    }
    else
    {
        ib.buildTarget(x86_64TargetInst::getTargetDesc(movOp), inst->getSourceRef(), { dst, raxPhys });
        // Touch rdxPhys to keep it live across IDIV so rhs cannot collide with rdx
        MirRegister *dummy = ob.buildVReg(type, "div_clobber");
        dummy->setClass(findClass(gprClass));
        ib.buildTarget(x86_64TargetInst::getTargetDesc(movOp), inst->getSourceRef(), { dummy, rdxPhys });
    }

    inst->eraseFromOwner();
    return true;
}

/**
 * Lowers integer shift instructions (SHL, SHR, SAR) for immediate and register shift amounts.
 */
bool X86_64TargetInstructionSelector::selectSHIFT(MirBuilderContext *ctx, MirInstruction *inst)
{
    if (inst->getOperandCount() < 3)
    {
        return false;
    }

    auto *dst = inst->getOperand(0);
    auto *src = inst->getOperand(1);
    auto *amt = inst->getOperand(2);

    bool is64 = false;
    if (dst->getMirType() && dst->getMirType()->getTotalSizeInBits() == 64)
    {
        is64 = true;
    }
    else if (src->getMirType() && src->getMirType()->getTotalSizeInBits() == 64)
    {
        is64 = true;
    }

    std::string_view gprClass = is64 ? "GPR64" : "GPR32";

    if (auto *r = dst->get<MirRegister>())
    {
        if (!r->getRegClass())
        {
            r->setClass(findClass(gprClass));
        }
    }
    if (auto *r = src->get<MirRegister>())
    {
        if (!r->getRegClass())
        {
            r->setClass(findClass(gprClass));
        }
    }

    MirOperandBuilder ob(ctx);
    MirInstructionBuilder ib(ctx, inst, InsertionType::InsertBefore);

    if (amt->getType() == MirOperandType::Integer)
    {
        int64_t rawAmt = amt->get<MirInteger>()->getValue().getI64();
        uint8_t mask = is64 ? 63 : 31;
        auto *amt8 = ob.buildInt(ctx->getTypeTable()->i8(), FlexInt(rawAmt & mask, 8));

        x86_64TargetInst::OpCode op;
        switch (inst->getOpCode())
        {
            case MirInstructionOpCode::SHL:
                op = is64 ? x86_64TargetInst::SHL64ri : x86_64TargetInst::SHL32ri;
                break;
            case MirInstructionOpCode::SHR:
                op = is64 ? x86_64TargetInst::SHR64ri : x86_64TargetInst::SHR32ri;
                break;
            case MirInstructionOpCode::SAR:
                op = is64 ? x86_64TargetInst::SAR64ri : x86_64TargetInst::SAR32ri;
                break;
            default:
                return false;
        }

        ib.buildTarget(x86_64TargetInst::getTargetDesc(op), inst->getSourceRef(), { dst, src, amt8 });
    }
    else
    {
        // Variable shift requires shift count in %cl (ECX/RCX, reg ID 1 in x86_64.tdesc)
        MirType *amtType = is64 ? ctx->getTypeTable()->i64() : ctx->getTypeTable()->i32();
        MirRegister *rcxPhys = ob.buildPhysReg(amtType, 1, is64 ? "rcx" : "ecx", findClass(gprClass));

        if (amt->getType() == MirOperandType::Register)
        {
            if (auto *r = amt->get<MirRegister>())
            {
                if (!r->getRegClass())
                {
                    r->setClass(findClass(gprClass));
                }
            }
            auto movOp = is64 ? x86_64TargetInst::MOV64rr : x86_64TargetInst::MOV32rr;
            ib.buildTarget(x86_64TargetInst::getTargetDesc(movOp), inst->getSourceRef(), { rcxPhys, amt });
        }

        x86_64TargetInst::OpCode op;
        switch (inst->getOpCode())
        {
            case MirInstructionOpCode::SHL:
                op = is64 ? x86_64TargetInst::SHL64rCL : x86_64TargetInst::SHL32rCL;
                break;
            case MirInstructionOpCode::SHR:
                op = is64 ? x86_64TargetInst::SHR64rCL : x86_64TargetInst::SHR32rCL;
                break;
            case MirInstructionOpCode::SAR:
                op = is64 ? x86_64TargetInst::SAR64rCL : x86_64TargetInst::SAR32rCL;
                break;
            default:
                return false;
        }

        ib.buildTarget(x86_64TargetInst::getTargetDesc(op), inst->getSourceRef(), { dst, src });

        // Keep rcxPhys live across the shift instruction so src/dst cannot collide with rcx
        MirRegister *dummy = ob.buildVReg(amtType, "cl_clobber");
        dummy->setClass(findClass(gprClass));
        auto movOp = is64 ? x86_64TargetInst::MOV64rr : x86_64TargetInst::MOV32rr;
        ib.buildTarget(x86_64TargetInst::getTargetDesc(movOp), inst->getSourceRef(), { dummy, rcxPhys });
    }

    inst->eraseFromOwner();
    return true;
}

bool X86_64TargetInstructionSelector::selectSUB(MirBuilderContext *ctx, MirInstruction *inst)
{
    if (inst->getOperandCount() < 3)
        return false;

    auto *dst = inst->getOperand(0);
    auto *lhs = inst->getOperand(1);
    auto *rhs = inst->getOperand(2);

    MirInstructionBuilder ib(ctx, inst, InsertionType::InsertBefore);
    MirOperandBuilder ob(ctx);

    bool is64 = (dst->getMirType() && dst->getMirType()->getTotalSizeInBits() == 64);
    std::string_view gprClass = is64 ? "GPR64" : "GPR32";

    if (rhs->getType() == MirOperandType::Integer)
    {
        auto *imm = rhs->get<MirInteger>();
        int64_t val = imm->getValue().getI64();
        if (val >= -2147483648LL && val <= 2147483647LL)
        {
            auto subOp = is64 ? x86_64TargetInst::SUB64ri : x86_64TargetInst::SUB32ri;
            ib.buildTarget(x86_64TargetInst::getTargetDesc(subOp),
                           inst->getSourceRef(),
                           { dst, lhs, rhs });
            inst->eraseFromOwner();
            return true;
        }
        else
        {
            MirRegister *vreg = ob.buildVReg(ctx->getTypeTable()->i64(), "imm64");
            vreg->setClass(findClass("GPR64"));
            ib.buildTarget(x86_64TargetInst::getTargetDesc(x86_64TargetInst::MOV64ri),
                           inst->getSourceRef(),
                           { vreg, rhs });
            rhs = vreg;
        }
    }

    if (auto *r = rhs->get<MirRegister>())
    {
        if (!r->getRegClass())
            r->setClass(findClass(gprClass));
    }
    if (auto *r = lhs->get<MirRegister>())
    {
        if (!r->getRegClass())
            r->setClass(findClass(gprClass));
    }
    if (auto *r = dst->get<MirRegister>())
    {
        if (!r->getRegClass())
            r->setClass(findClass(gprClass));
    }

    auto subOp = is64 ? x86_64TargetInst::SUB64rr : x86_64TargetInst::SUB32rr;
    ib.buildTarget(x86_64TargetInst::getTargetDesc(subOp),
                   inst->getSourceRef(),
                   { dst, lhs, rhs });
    inst->eraseFromOwner();
    return true;
}

bool X86_64TargetInstructionSelector::selectADD(MirBuilderContext *ctx, MirInstruction *inst)
{
    if (inst->getOperandCount() < 3)
        return false;

    auto *dst = inst->getOperand(0);
    auto *lhs = inst->getOperand(1);
    auto *rhs = inst->getOperand(2);

    MirInstructionBuilder ib(ctx, inst, InsertionType::InsertBefore);
    MirOperandBuilder ob(ctx);

    bool is64 = (dst->getMirType() && dst->getMirType()->getTotalSizeInBits() == 64);
    std::string_view gprClass = is64 ? "GPR64" : "GPR32";

    if (rhs->getType() == MirOperandType::Integer)
    {
        auto *imm = rhs->get<MirInteger>();
        int64_t val = imm->getValue().getI64();
        if (val >= -2147483648LL && val <= 2147483647LL)
        {
            auto addOp = is64 ? x86_64TargetInst::ADD64ri : x86_64TargetInst::ADD32ri;
            ib.buildTarget(x86_64TargetInst::getTargetDesc(addOp),
                           inst->getSourceRef(),
                           { dst, lhs, rhs });
            inst->eraseFromOwner();
            return true;
        }
        else
        {
            MirRegister *vreg = ob.buildVReg(ctx->getTypeTable()->i64(), "imm64");
            vreg->setClass(findClass("GPR64"));
            ib.buildTarget(x86_64TargetInst::getTargetDesc(x86_64TargetInst::MOV64ri),
                           inst->getSourceRef(),
                           { vreg, rhs });
            rhs = vreg;
        }
    }

    if (auto *r = rhs->get<MirRegister>())
    {
        if (!r->getRegClass())
            r->setClass(findClass(gprClass));
    }
    if (auto *r = lhs->get<MirRegister>())
    {
        if (!r->getRegClass())
            r->setClass(findClass(gprClass));
    }
    if (auto *r = dst->get<MirRegister>())
    {
        if (!r->getRegClass())
            r->setClass(findClass(gprClass));
    }

    auto addOp = is64 ? x86_64TargetInst::ADD64rr : x86_64TargetInst::ADD32rr;
    ib.buildTarget(x86_64TargetInst::getTargetDesc(addOp),
                   inst->getSourceRef(),
                   { dst, lhs, rhs });
    inst->eraseFromOwner();
    return true;
}

bool X86_64TargetInstructionSelector::selectPUSH_REGS(MirBuilderContext *ctx, MirInstruction *inst)
{
    MirInstructionBuilder ib(ctx, inst, InsertionType::InsertBefore);
    MirOperandBuilder ob(ctx);
    const auto *descPush = x86_64TargetInst::getTargetDesc(x86_64TargetInst::PUSH64r);

    if (inst->getOperandCount() == 0)
    {
        // Target-agnostic lowering: query calling convention descriptor dynamically
        MirFunction *func = (inst->getOwner() ? inst->getOwner()->getOwner() : nullptr);
        CallingConvDesc *cc = func ? func->getCallingConv() : nullptr;
        if (cc)
        {
            const auto &callees = cc->getAllCalleeSavedRegs();
            for (const auto &regRef : callees)
            {
                MirRegister *reg = ob.buildPhysReg(ctx->getTypeTable()->i64(), regRef.getId(), "", regRef.getClass());
                ib.buildTarget(descPush, inst->getSourceRef(), { reg });
            }
        }
    }
    else
    {
        for (size_t i = 0; i < inst->getOperandCount(); ++i)
        {
            auto *op = inst->getOperand(i);
            if (auto *r = op->get<MirRegister>())
            {
                if (!r->getRegClass())
                    r->setClass(findClass("GPR64"));
            }
            ib.buildTarget(descPush, inst->getSourceRef(), { op });
        }
    }

    inst->eraseFromOwner();
    return true;
}

bool X86_64TargetInstructionSelector::selectPOP_REGS(MirBuilderContext *ctx, MirInstruction *inst)
{
    MirInstructionBuilder ib(ctx, inst, InsertionType::InsertBefore);
    MirOperandBuilder ob(ctx);
    const auto *descPop = x86_64TargetInst::getTargetDesc(x86_64TargetInst::POP64r);

    if (inst->getOperandCount() == 0)
    {
        // Target-agnostic lowering: pop in reverse order of callee-saved registers
        MirFunction *func = (inst->getOwner() ? inst->getOwner()->getOwner() : nullptr);
        CallingConvDesc *cc = func ? func->getCallingConv() : nullptr;
        if (cc)
        {
            const auto &callees = cc->getAllCalleeSavedRegs();
            for (auto it = callees.rbegin(); it != callees.rend(); ++it)
            {
                MirRegister *reg = ob.buildPhysReg(ctx->getTypeTable()->i64(), it->getId(), "", it->getClass());
                ib.buildTarget(descPop, inst->getSourceRef(), { reg });
            }
        }
    }
    else
    {
        for (size_t i = 0; i < inst->getOperandCount(); ++i)
        {
            auto *op = inst->getOperand(i);
            if (auto *r = op->get<MirRegister>())
            {
                if (!r->getRegClass())
                    r->setClass(findClass("GPR64"));
            }
            ib.buildTarget(descPop, inst->getSourceRef(), { op });
        }
    }

    inst->eraseFromOwner();
    return true;
}

} // namespace EzTargets::X86_64
