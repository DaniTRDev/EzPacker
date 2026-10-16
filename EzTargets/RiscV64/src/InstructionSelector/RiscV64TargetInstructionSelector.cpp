#include "RiscV64TargetInstructionSelector.h"
#include "riscv64TargetInstructionTable.h"
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

namespace EzTargets::RiscV64
{

RiscV64TargetInstructionSelector::RiscV64TargetInstructionSelector(TargetDesc *targetDesc) :
    riscv64InstructionSelector(targetDesc), m_targetDesc(targetDesc)
{
}

bool RiscV64TargetInstructionSelector::select(MirBuilderContext *ctx, MirInstruction *inst)
{
    if (!ctx || !inst)
    {
        return false;
    }

    // 1. Try base generated instruction selector first
    if (riscv64InstructionSelector::select(ctx, inst))
    {
        return true;
    }

    switch (inst->getOpCode())
    {
        case MirInstructionOpCode::MOV:
            return selectMOV(ctx, inst);
        case MirInstructionOpCode::JMP:
            return selectJMP(ctx, inst);
        case MirInstructionOpCode::BR_COND:
            return selectBR_COND(ctx, inst);
        case MirInstructionOpCode::TRY:
            return selectTRY(ctx, inst);
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

bool RiscV64TargetInstructionSelector::selectJMP(MirBuilderContext *ctx, MirInstruction *inst)
{
    if (inst->getOperandCount() < 1)
    {
        return false;
    }

    MirInstructionBuilder ib(ctx, inst, InsertionType::InsertBefore);
    MirOperandBuilder ob(ctx);
    MirOperand *zeroReg = ob.buildPhysReg(ctx->getTypeTable()->i64(), 0, "zero", findClass("GPR64"));

    auto *target = inst->getOperand(0);
    if (target->getType() == MirOperandType::Register)
    {
        MirOperand *disp0 = ob.buildInt(ctx->getTypeTable()->i64(), FlexInt(static_cast<int64_t>(0)));
        ib.buildTarget(riscv64TargetInst::getTargetDesc(riscv64TargetInst::JALR),
                       inst->getSourceRef(),
                       { zeroReg, target, disp0 });
    }
    else
    {
        ib.buildTarget(riscv64TargetInst::getTargetDesc(riscv64TargetInst::JAL),
                       inst->getSourceRef(),
                       { zeroReg, target });
    }

    inst->eraseFromOwner();
    return true;
}

bool RiscV64TargetInstructionSelector::selectTRY(MirBuilderContext *ctx, MirInstruction *inst)
{
    return selectJMP(ctx, inst);
}

bool RiscV64TargetInstructionSelector::selectBR_COND(MirBuilderContext *ctx, MirInstruction *inst)
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
            r->setClass(findClass("GPR64"));
        }
    }

    MirOperand *zeroReg = ob.buildPhysReg(ctx->getTypeTable()->i64(), 0, "zero", findClass("GPR64"));

    // BNE %cond, zero, %trueBlock
    ib.buildTarget(riscv64TargetInst::getTargetDesc(riscv64TargetInst::BNE),
                   inst->getSourceRef(),
                   { cond, zeroReg, trueBlock });

    // JAL zero, %falseBlock
    ib.buildTarget(riscv64TargetInst::getTargetDesc(riscv64TargetInst::JAL),
                   inst->getSourceRef(),
                   { zeroReg, falseBlock });

    inst->eraseFromOwner();
    return true;
}

bool RiscV64TargetInstructionSelector::selectCALL(MirBuilderContext *ctx, MirInstruction *inst)
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
            continue;

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
    MirOperandBuilder ob(ctx);

    if (callee->getType() == MirOperandType::Register)
    {
        auto *r = callee->get<MirRegister>();
        if (r && !r->getRegClass())
        {
            r->setClass(findClass("GPR64"));
        }
        MirOperand *raReg = ob.buildPhysReg(ctx->getTypeTable()->i64(), 1, "ra", findClass("GPR64"));
        MirOperand *disp0 = ob.buildInt(ctx->getTypeTable()->i64(), FlexInt(static_cast<int64_t>(0)));
        ib.buildTarget(riscv64TargetInst::getTargetDesc(riscv64TargetInst::JALR),
                       inst->getSourceRef(),
                       { raReg, callee, disp0 });
    }
    else
    {
        ib.buildTarget(riscv64TargetInst::getTargetDesc(riscv64TargetInst::CALL),
                       inst->getSourceRef(),
                       { callee });
    }

    inst->eraseFromOwner();
    return true;
}

bool RiscV64TargetInstructionSelector::selectCMP(MirBuilderContext *ctx, MirInstruction *inst)
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

    if (auto *r = dst->get<MirRegister>())
    {
        if (!r->getRegClass())
            r->setClass(findClass("GPR64"));
    }
    if (auto *r = lhs->get<MirRegister>())
    {
        if (!r->getRegClass())
            r->setClass(findClass(gprClass));
    }
    if (auto *r = rhs->get<MirRegister>())
    {
        if (!r->getRegClass())
            r->setClass(findClass(gprClass));
    }

    MirOperand *zeroReg = ob.buildPhysReg(ctx->getTypeTable()->i64(), 0, "zero", findClass("GPR64"));

    // If rhs is immediate and we need it in register
    MirOperand *effectiveRhs = rhs;
    if (rhs->getType() == MirOperandType::Integer)
    {
        int64_t immVal = rhs->get<MirInteger>()->getValue().getI64();
        if (inst->getOpCode() == MirInstructionOpCode::CMP_SLT && immVal >= -2048 && immVal <= 2047)
        {
            // Direct SLTI
            ib.buildTarget(riscv64TargetInst::getTargetDesc(riscv64TargetInst::SLTI64ri),
                           inst->getSourceRef(),
                           { dst, lhs, rhs });
            inst->eraseFromOwner();
            return true;
        }
        if (inst->getOpCode() == MirInstructionOpCode::CMP_ULT && immVal >= -2048 && immVal <= 2047)
        {
            // Direct SLTIU
            ib.buildTarget(riscv64TargetInst::getTargetDesc(riscv64TargetInst::SLTIU64ri),
                           inst->getSourceRef(),
                           { dst, lhs, rhs });
            inst->eraseFromOwner();
            return true;
        }

        // Materialize immediate into temp register
        MirOperand *tmpRhs = ob.buildVReg(ctx->getTypeTable()->i64(), "rhs_imm", nullptr, findClass("GPR64"));
        ib.buildTarget(riscv64TargetInst::getTargetDesc(riscv64TargetInst::ADDI64ri),
                       inst->getSourceRef(),
                       { tmpRhs, zeroReg, rhs });
        effectiveRhs = tmpRhs;
    }

    switch (inst->getOpCode())
    {
        case MirInstructionOpCode::CMP_SLT:
        {
            ib.buildTarget(riscv64TargetInst::getTargetDesc(riscv64TargetInst::SLT64rr),
                           inst->getSourceRef(),
                           { dst, lhs, effectiveRhs });
            break;
        }
        case MirInstructionOpCode::CMP_ULT:
        {
            ib.buildTarget(riscv64TargetInst::getTargetDesc(riscv64TargetInst::SLTU64rr),
                           inst->getSourceRef(),
                           { dst, lhs, effectiveRhs });
            break;
        }
        case MirInstructionOpCode::CMP_SGT:
        {
            ib.buildTarget(riscv64TargetInst::getTargetDesc(riscv64TargetInst::SLT64rr),
                           inst->getSourceRef(),
                           { dst, effectiveRhs, lhs });
            break;
        }
        case MirInstructionOpCode::CMP_UGT:
        {
            ib.buildTarget(riscv64TargetInst::getTargetDesc(riscv64TargetInst::SLTU64rr),
                           inst->getSourceRef(),
                           { dst, effectiveRhs, lhs });
            break;
        }
        case MirInstructionOpCode::CMP_EQ:
        {
            // sub tmp, lhs, rhs; sltiu dst, tmp, 1
            auto subOp = is64 ? riscv64TargetInst::SUB64rr : riscv64TargetInst::SUB32rr;
            MirOperand *tmp = ob.buildVReg(ctx->getTypeTable()->i64(), "cmp_eq_tmp", nullptr, findClass("GPR64"));
            ib.buildTarget(riscv64TargetInst::getTargetDesc(subOp),
                           inst->getSourceRef(),
                           { tmp, lhs, effectiveRhs });
            MirOperand *imm1 = ob.buildInt(ctx->getTypeTable()->i64(), FlexInt(static_cast<int64_t>(1)));
            ib.buildTarget(riscv64TargetInst::getTargetDesc(riscv64TargetInst::SLTIU64ri),
                           inst->getSourceRef(),
                           { dst, tmp, imm1 });
            break;
        }
        case MirInstructionOpCode::CMP_NE:
        {
            // sub tmp, lhs, rhs; sltu dst, zero, tmp
            auto subOp = is64 ? riscv64TargetInst::SUB64rr : riscv64TargetInst::SUB32rr;
            MirOperand *tmp = ob.buildVReg(ctx->getTypeTable()->i64(), "cmp_ne_tmp", nullptr, findClass("GPR64"));
            ib.buildTarget(riscv64TargetInst::getTargetDesc(subOp),
                           inst->getSourceRef(),
                           { tmp, lhs, effectiveRhs });
            ib.buildTarget(riscv64TargetInst::getTargetDesc(riscv64TargetInst::SLTU64rr),
                           inst->getSourceRef(),
                           { dst, zeroReg, tmp });
            break;
        }
        case MirInstructionOpCode::CMP_SLE:
        {
            // slt tmp, rhs, lhs; xori dst, tmp, 1
            MirOperand *tmp = ob.buildVReg(ctx->getTypeTable()->i64(), "cmp_sle_tmp", nullptr, findClass("GPR64"));
            ib.buildTarget(riscv64TargetInst::getTargetDesc(riscv64TargetInst::SLT64rr),
                           inst->getSourceRef(),
                           { tmp, effectiveRhs, lhs });
            MirOperand *imm1 = ob.buildInt(ctx->getTypeTable()->i64(), FlexInt(static_cast<int64_t>(1)));
            ib.buildTarget(riscv64TargetInst::getTargetDesc(riscv64TargetInst::XORI64ri),
                           inst->getSourceRef(),
                           { dst, tmp, imm1 });
            break;
        }
        case MirInstructionOpCode::CMP_SGE:
        {
            // slt tmp, lhs, rhs; xori dst, tmp, 1
            MirOperand *tmp = ob.buildVReg(ctx->getTypeTable()->i64(), "cmp_sge_tmp", nullptr, findClass("GPR64"));
            ib.buildTarget(riscv64TargetInst::getTargetDesc(riscv64TargetInst::SLT64rr),
                           inst->getSourceRef(),
                           { tmp, lhs, effectiveRhs });
            MirOperand *imm1 = ob.buildInt(ctx->getTypeTable()->i64(), FlexInt(static_cast<int64_t>(1)));
            ib.buildTarget(riscv64TargetInst::getTargetDesc(riscv64TargetInst::XORI64ri),
                           inst->getSourceRef(),
                           { dst, tmp, imm1 });
            break;
        }
        case MirInstructionOpCode::CMP_ULE:
        {
            // sltu tmp, rhs, lhs; xori dst, tmp, 1
            MirOperand *tmp = ob.buildVReg(ctx->getTypeTable()->i64(), "cmp_ule_tmp", nullptr, findClass("GPR64"));
            ib.buildTarget(riscv64TargetInst::getTargetDesc(riscv64TargetInst::SLTU64rr),
                           inst->getSourceRef(),
                           { tmp, effectiveRhs, lhs });
            MirOperand *imm1 = ob.buildInt(ctx->getTypeTable()->i64(), FlexInt(static_cast<int64_t>(1)));
            ib.buildTarget(riscv64TargetInst::getTargetDesc(riscv64TargetInst::XORI64ri),
                           inst->getSourceRef(),
                           { dst, tmp, imm1 });
            break;
        }
        case MirInstructionOpCode::CMP_UGE:
        {
            // sltu tmp, lhs, rhs; xori dst, tmp, 1
            MirOperand *tmp = ob.buildVReg(ctx->getTypeTable()->i64(), "cmp_uge_tmp", nullptr, findClass("GPR64"));
            ib.buildTarget(riscv64TargetInst::getTargetDesc(riscv64TargetInst::SLTU64rr),
                           inst->getSourceRef(),
                           { tmp, lhs, effectiveRhs });
            MirOperand *imm1 = ob.buildInt(ctx->getTypeTable()->i64(), FlexInt(static_cast<int64_t>(1)));
            ib.buildTarget(riscv64TargetInst::getTargetDesc(riscv64TargetInst::XORI64ri),
                           inst->getSourceRef(),
                           { dst, tmp, imm1 });
            break;
        }
        default:
            return false;
    }

    inst->eraseFromOwner();
    return true;
}

bool RiscV64TargetInstructionSelector::selectLOAD(MirBuilderContext *ctx, MirInstruction *inst)
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
    riscv64TargetInst::OpCode loadOp = riscv64TargetInst::LD;
    std::string_view dstClass = "GPR64";

    if (isFloat)
    {
        if (sizeInBits == 32)
        {
            loadOp = riscv64TargetInst::FLW;
            dstClass = "FPR32";
        }
        else
        {
            loadOp = riscv64TargetInst::FLD;
            dstClass = "FPR64";
        }
    }
    else
    {
        if (sizeInBits <= 8)
        {
            loadOp = riscv64TargetInst::LB;
            dstClass = "GPR32";
        }
        else if (sizeInBits <= 16)
        {
            loadOp = riscv64TargetInst::LH;
            dstClass = "GPR32";
        }
        else if (sizeInBits <= 32)
        {
            loadOp = riscv64TargetInst::LW;
            dstClass = "GPR32";
        }
    }

    if (auto *r = dst->get<MirRegister>())
    {
        if (!r->getRegClass())
            r->setClass(findClass(dstClass));
    }

    MirOperand *baseOp = src;
    MirOperand *dispOp = nullptr;

    if (auto *r = src->get<MirRegister>())
    {
        if (!r->getRegClass())
            r->setClass(findClass("GPR64"));
        baseOp = r;
        dispOp = ob.buildInt(ctx->getTypeTable()->i64(), FlexInt(static_cast<int64_t>(0)));
    }
    else if (auto *m = src->get<MirMemory>())
    {
        if (m->getBase() && !m->getBase()->getRegClass())
            m->getBase()->setClass(findClass("GPR64"));
        baseOp = m->getBase();
        dispOp = m->getDisplacement() ? static_cast<MirOperand *>(m->getDisplacement())
                                      : ob.buildInt(ctx->getTypeTable()->i64(), FlexInt(static_cast<int64_t>(0)));
    }
    else if (src->getType() == MirOperandType::Reference)
    {
        // Stack slot reference: base is s0 (FP)
        baseOp = ob.buildPhysReg(ctx->getTypeTable()->i64(), 8, "s0", findClass("GPR64"));
        dispOp = src;
    }

    if (!baseOp || !dispOp)
    {
        return false;
    }

    ib.buildTarget(riscv64TargetInst::getTargetDesc(loadOp),
                   inst->getSourceRef(),
                   { dst, baseOp, dispOp });
    inst->eraseFromOwner();
    return true;
}

bool RiscV64TargetInstructionSelector::selectSTORE(MirBuilderContext *ctx, MirInstruction *inst)
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
    riscv64TargetInst::OpCode storeOp = riscv64TargetInst::SD;
    std::string_view valClass = "GPR64";

    if (isFloat)
    {
        if (sizeInBits == 32)
        {
            storeOp = riscv64TargetInst::FSW;
            valClass = "FPR32";
        }
        else
        {
            storeOp = riscv64TargetInst::FSD;
            valClass = "FPR64";
        }
    }
    else
    {
        if (sizeInBits <= 8)
        {
            storeOp = riscv64TargetInst::SB;
            valClass = "GPR32";
        }
        else if (sizeInBits <= 16)
        {
            storeOp = riscv64TargetInst::SH;
            valClass = "GPR32";
        }
        else if (sizeInBits <= 32)
        {
            storeOp = riscv64TargetInst::SW;
            valClass = "GPR32";
        }
    }

    if (auto *r = val->get<MirRegister>())
    {
        if (!r->getRegClass())
            r->setClass(findClass(valClass));
    }

    MirOperand *baseOp = dest;
    MirOperand *dispOp = nullptr;

    if (auto *r = dest->get<MirRegister>())
    {
        if (!r->getRegClass())
            r->setClass(findClass("GPR64"));
        baseOp = r;
        dispOp = ob.buildInt(ctx->getTypeTable()->i64(), FlexInt(static_cast<int64_t>(0)));
    }
    else if (auto *m = dest->get<MirMemory>())
    {
        if (m->getBase() && !m->getBase()->getRegClass())
            m->getBase()->setClass(findClass("GPR64"));
        baseOp = m->getBase();
        dispOp = m->getDisplacement() ? static_cast<MirOperand *>(m->getDisplacement())
                                      : ob.buildInt(ctx->getTypeTable()->i64(), FlexInt(static_cast<int64_t>(0)));
    }
    else if (dest->getType() == MirOperandType::Reference)
    {
        baseOp = ob.buildPhysReg(ctx->getTypeTable()->i64(), 8, "s0", findClass("GPR64"));
        dispOp = dest;
    }

    if (!baseOp || !dispOp)
    {
        return false;
    }

    ib.buildTarget(riscv64TargetInst::getTargetDesc(storeOp),
                   inst->getSourceRef(),
                   { val, baseOp, dispOp });
    inst->eraseFromOwner();
    return true;
}

bool RiscV64TargetInstructionSelector::selectPHI(MirBuilderContext *ctx, MirInstruction *inst)
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

    size_t sizeInBits = dst->getMirType() ? dst->getMirType()->getTotalSizeInBits() : 64;
    std::string_view gprClass = (sizeInBits == 64) ? "GPR64" : "GPR32";
    if (!dst->getRegClass())
    {
        dst->setClass(findClass(gprClass));
    }

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

bool RiscV64TargetInstructionSelector::selectMOV(MirBuilderContext *ctx, MirInstruction *inst)
{
    if (inst->getOperandCount() < 2)
    {
        return false;
    }

    auto *dst = inst->getOperand(0);
    auto *src = inst->getOperand(1);

    MirInstructionBuilder ib(ctx, inst, InsertionType::InsertBefore);
    MirOperandBuilder ob(ctx);

    if (auto *r = dst->get<MirRegister>())
    {
        if (!r->getRegClass())
            r->setClass(findClass("GPR64"));
    }

    MirOperand *zeroReg = ob.buildPhysReg(ctx->getTypeTable()->i64(), 0, "zero", findClass("GPR64"));

    // 1. Reference (global, stack, function)
    if (src->getType() == MirOperandType::Reference)
    {
        auto *ref = src->get<MirReference>();
        if (ref && ref->isStackFrameObject())
        {
            MirOperand *s0Reg = ob.buildPhysReg(ctx->getTypeTable()->i64(), 8, "s0", findClass("GPR64"));
            ib.buildTarget(riscv64TargetInst::getTargetDesc(riscv64TargetInst::ADDI64ri),
                           inst->getSourceRef(),
                           { dst, s0Reg, src });
            inst->eraseFromOwner();
            return true;
        }
        // Global or function reference: addi dst, zero, ref
        ib.buildTarget(riscv64TargetInst::getTargetDesc(riscv64TargetInst::ADDI64ri),
                       inst->getSourceRef(),
                       { dst, zeroReg, src });
        inst->eraseFromOwner();
        return true;
    }

    // 2. Runtime symbol
    if (src->getType() == MirOperandType::RuntimeSymbol)
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
        MirOperand *ref = ob.buildRef(fn);
        ib.buildTarget(riscv64TargetInst::getTargetDesc(riscv64TargetInst::ADDI64ri),
                       inst->getSourceRef(),
                       { dst, zeroReg, ref });
        inst->eraseFromOwner();
        return true;
    }

    // 3. Register to register move
    if (src->getType() == MirOperandType::Register)
    {
        auto *rSrc = src->get<MirRegister>();
        bool isFpr = false;
        if (rSrc && rSrc->getRegClass())
        {
            isFpr = (std::string_view(rSrc->getRegClass()->getName()).find("FPR") != std::string_view::npos);
        }
        if (isFpr)
        {
            if (auto *rDst = dst->get<MirRegister>())
            {
                if (!rDst->getRegClass())
                    rDst->setClass(findClass("FPR64"));
            }
            ib.buildTarget(riscv64TargetInst::getTargetDesc(riscv64TargetInst::FMV_D),
                           inst->getSourceRef(),
                           { dst, src });
        }
        else
        {
            if (rSrc && !rSrc->getRegClass())
                rSrc->setClass(findClass("GPR64"));
            MirOperand *imm0 = ob.buildInt(ctx->getTypeTable()->i64(), FlexInt(static_cast<int64_t>(0)));
            ib.buildTarget(riscv64TargetInst::getTargetDesc(riscv64TargetInst::ADDI64ri),
                           inst->getSourceRef(),
                           { dst, src, imm0 });
        }
        inst->eraseFromOwner();
        return true;
    }

    // 4. Integer immediate
    if (src->getType() == MirOperandType::Integer)
    {
        int64_t val = src->get<MirInteger>()->getValue().getI64();
        if (val >= -2048 && val <= 2047)
        {
            ib.buildTarget(riscv64TargetInst::getTargetDesc(riscv64TargetInst::ADDI64ri),
                           inst->getSourceRef(),
                           { dst, zeroReg, src });
        }
        else
        {
            int32_t val32 = static_cast<int32_t>(val);
            int32_t hi20 = (val32 + 0x800) >> 12;
            int32_t lo12 = val32 - (hi20 << 12);
            MirOperand *hiImm = ob.buildInt(ctx->getTypeTable()->i64(), FlexInt(static_cast<int64_t>(hi20)));
            MirOperand *loImm = ob.buildInt(ctx->getTypeTable()->i64(), FlexInt(static_cast<int64_t>(lo12)));
            ib.buildTarget(riscv64TargetInst::getTargetDesc(riscv64TargetInst::LUI),
                           inst->getSourceRef(),
                           { dst, hiImm });
            ib.buildTarget(riscv64TargetInst::getTargetDesc(riscv64TargetInst::ADDI64ri),
                           inst->getSourceRef(),
                           { dst, dst, loImm });
        }
        inst->eraseFromOwner();
        return true;
    }

    return false;
}

} // namespace EzTargets::RiscV64
