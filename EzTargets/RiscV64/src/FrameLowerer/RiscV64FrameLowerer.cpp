#include "RiscV64FrameLowerer.h"
#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Function/CallingConvDesc.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionStackFrame.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Instruction/MirTargetInstructionDesc.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Operand/MirRegisterClass.h"
#include "Type/MirType.h"
#include "Type/MirTypeTable.h"
#include "FlexNumber/FlexInt.h"
#include "riscv64TargetInstructionTable.h"

namespace EzTargets::RiscV64
{

namespace
{

struct FrameLoweringSetup
{
    MirFunction *m_func{ nullptr };
    CallingConvDesc *m_cc{ nullptr };
    MirFunctionAnalysisData *m_analysisData{ nullptr };
    MirType *m_ptrType{ nullptr };
    MirType *m_i64{ nullptr };
    bool m_hasFP{ false };
    MirRegisterRef m_fpRef;
    MirRegisterRef m_spRef;
    MirRegister *m_fpReg{ nullptr };
    MirRegister *m_spReg{ nullptr };
    MirRegister *m_raReg{ nullptr };
    const MirTargetInstructionDesc *m_descSd{ nullptr };
    const MirTargetInstructionDesc *m_descLd{ nullptr };
    const MirTargetInstructionDesc *m_descFsd{ nullptr };
    const MirTargetInstructionDesc *m_descFld{ nullptr };
    const MirTargetInstructionDesc *m_descAddi{ nullptr };
    const MirTargetInstructionDesc *m_descSub{ nullptr };
};

bool resolveFrameLoweringSetup(FrameLowererCtx &ctx, MirOperandBuilder &opBuilder, FrameLoweringSetup &setup)
{
    MirFunction *func = ctx.m_targetFunc;
    if (!func)
    {
        return false;
    }
    CallingConvDesc *cc = func->getCallingConv();
    if (!cc)
    {
        return false;
    }

    setup.m_func = func;
    setup.m_cc = cc;
    setup.m_analysisData = func->getAnalysisData();
    setup.m_ptrType = ctx.m_ctx->getTypeTable()->getPtr(ctx.m_ctx->getTypeTable()->_void());
    setup.m_i64 = ctx.m_ctx->getTypeTable()->i64();
    setup.m_hasFP = cc->hasFramePointer(func);
    setup.m_fpRef = cc->getFramePointerReg();
    setup.m_spRef = cc->getStackPointerReg();

    setup.m_fpReg = opBuilder.buildPhysReg(setup.m_ptrType, setup.m_fpRef.getId(), "s0", setup.m_fpRef.getClass());
    setup.m_spReg = opBuilder.buildPhysReg(setup.m_ptrType, setup.m_spRef.getId(), "sp", setup.m_spRef.getClass());
    // ra is x1
    setup.m_raReg = opBuilder.buildPhysReg(setup.m_ptrType, 1, "ra", setup.m_spRef.getClass());

    setup.m_descSd = EzTargets::RiscV64::riscv64TargetInst::getTargetDesc(EzTargets::RiscV64::riscv64TargetInst::SD);
    setup.m_descLd = EzTargets::RiscV64::riscv64TargetInst::getTargetDesc(EzTargets::RiscV64::riscv64TargetInst::LD);
    setup.m_descFsd = EzTargets::RiscV64::riscv64TargetInst::getTargetDesc(EzTargets::RiscV64::riscv64TargetInst::FSD);
    setup.m_descFld = EzTargets::RiscV64::riscv64TargetInst::getTargetDesc(EzTargets::RiscV64::riscv64TargetInst::FLD);
    setup.m_descAddi = EzTargets::RiscV64::riscv64TargetInst::getTargetDesc(EzTargets::RiscV64::riscv64TargetInst::ADDI64ri);
    setup.m_descSub = EzTargets::RiscV64::riscv64TargetInst::getTargetDesc(EzTargets::RiscV64::riscv64TargetInst::SUB64rr);

    return true;
}

bool isFprRef(const MirRegisterRef &regRef)
{
    if (auto *cls = regRef.getClass())
    {
        std::string_view name = cls->getName();
        return name.find("FPR") != std::string_view::npos;
    }
    return false;
}

} // namespace

void RiscV64FrameLowerer::insertPrologue(FrameLowererCtx &ctx)
{
    MirOperandBuilder opBuilder(ctx.m_ctx);
    FrameLoweringSetup setup;
    if (!resolveFrameLoweringSetup(ctx, opBuilder, setup))
    {
        return;
    }

    MirBlock *entryBlock = setup.m_func->getBlocks().empty() ? nullptr : setup.m_func->getBlocks().front();
    if (!entryBlock)
    {
        return;
    }

    SourceReference *srcRef = setup.m_func->getSourceRef();
    auto &instrList = entryBlock->getInstructions();
    auto it = instrList.begin();
    MirInstructionBuilder builder(ctx.m_ctx,
                                  entryBlock,
                                  it != instrList.end() ? InsertionType::InsertBefore : InsertionType::Append,
                                  it);

    // 1. Setup frame pointer and save ra:
    //    addi sp, sp, -16
    //    sd ra, 8(sp)
    //    sd s0, 0(sp)
    //    addi s0, sp, 16
    if (setup.m_hasFP)
    {
        MirInteger *neg16 = opBuilder.buildInt(setup.m_i64, FlexInt(static_cast<int64_t>(-16)));
        builder.buildTarget(setup.m_descAddi, srcRef, { setup.m_spReg, setup.m_spReg, neg16 });

        MirInteger *off8 = opBuilder.buildInt(setup.m_i64, FlexInt(static_cast<int64_t>(8)));
        builder.buildTarget(setup.m_descSd, srcRef, { setup.m_raReg, setup.m_spReg, off8 });

        MirInteger *off0 = opBuilder.buildInt(setup.m_i64, FlexInt(static_cast<int64_t>(0)));
        builder.buildTarget(setup.m_descSd, srcRef, { setup.m_fpReg, setup.m_spReg, off0 });

        MirInteger *pos16 = opBuilder.buildInt(setup.m_i64, FlexInt(static_cast<int64_t>(16)));
        builder.buildTarget(setup.m_descAddi, srcRef, { setup.m_fpReg, setup.m_spReg, pos16 });
    }

    // 2. Allocate stack frame space:
    //    addi sp, sp, -frameSize
    if (setup.m_analysisData && setup.m_analysisData->m_totalFrameSize > 0)
    {
        int64_t frameSize = static_cast<int64_t>(setup.m_analysisData->m_totalFrameSize);
        // Align to 16 bytes
        frameSize = (frameSize + 15) & ~15LL;
        MirInteger *sizeImm = opBuilder.buildInt(setup.m_i64, FlexInt(-frameSize));
        builder.buildTarget(setup.m_descAddi, srcRef, { setup.m_spReg, setup.m_spReg, sizeImm });
    }

    // 3. Save callee-saved registers:
    const auto &usedCallees = setup.m_func->getUsedCalleeSavedRegs();
    for (const MirRegisterRef &regRef : usedCallees)
    {
        if (setup.m_hasFP && regRef == setup.m_fpRef)
        {
            continue;
        }

        MirInteger *neg16 = opBuilder.buildInt(setup.m_i64, FlexInt(static_cast<int64_t>(-16)));
        builder.buildTarget(setup.m_descAddi, srcRef, { setup.m_spReg, setup.m_spReg, neg16 });

        MirRegister *calleeReg = opBuilder.buildPhysReg(setup.m_ptrType, regRef.getId(), "", regRef.getClass());
        MirInteger *off0 = opBuilder.buildInt(setup.m_i64, FlexInt(static_cast<int64_t>(0)));
        if (isFprRef(regRef))
        {
            builder.buildTarget(setup.m_descFsd, srcRef, { calleeReg, setup.m_spReg, off0 });
        }
        else
        {
            builder.buildTarget(setup.m_descSd, srcRef, { calleeReg, setup.m_spReg, off0 });
        }
    }
}

void RiscV64FrameLowerer::insertEpilogue(FrameLowererCtx &ctx)
{
    MirOperandBuilder opBuilder(ctx.m_ctx);
    FrameLoweringSetup setup;
    if (!resolveFrameLoweringSetup(ctx, opBuilder, setup))
    {
        return;
    }

    const auto &usedCallees = setup.m_func->getUsedCalleeSavedRegs();

    for (MirBlock *block : setup.m_func->getBlocks())
    {
        for (auto it = block->getInstructions().begin(); it != block->getInstructions().end(); ++it)
        {
            MirInstruction *inst = *it;
            if (!inst)
            {
                continue;
            }

            bool isRet = (inst->getOpCode() == MirInstructionOpCode::RET);
            if (!isRet && inst->getTargetDesc())
            {
                isRet = (inst->getTargetDesc()->getTargetFlags() & MirInstructionFlags::IsReturn) != 0;
            }

            if (!isRet)
            {
                continue;
            }

            MirInstructionBuilder epilogueBuilder(ctx.m_ctx, inst, InsertionType::InsertBefore);
            SourceReference *srcRef = inst->getSourceRef();

            // 1. Restore callee-saved registers in reverse order:
            for (auto rIt = usedCallees.rbegin(); rIt != usedCallees.rend(); ++rIt)
            {
                if (setup.m_hasFP && *rIt == setup.m_fpRef)
                {
                    continue;
                }

                MirRegister *calleeReg = opBuilder.buildPhysReg(setup.m_ptrType, rIt->getId(), "", rIt->getClass());
                MirInteger *off0 = opBuilder.buildInt(setup.m_i64, FlexInt(static_cast<int64_t>(0)));
                if (isFprRef(*rIt))
                {
                    epilogueBuilder.buildTarget(setup.m_descFld, srcRef, { calleeReg, setup.m_spReg, off0 });
                }
                else
                {
                    epilogueBuilder.buildTarget(setup.m_descLd, srcRef, { calleeReg, setup.m_spReg, off0 });
                }

                MirInteger *pos16 = opBuilder.buildInt(setup.m_i64, FlexInt(static_cast<int64_t>(16)));
                epilogueBuilder.buildTarget(setup.m_descAddi, srcRef, { setup.m_spReg, setup.m_spReg, pos16 });
            }

            // 2. Restore stack pointer and frame pointer:
            if (setup.m_hasFP)
            {
                // addi sp, s0, -16
                MirInteger *neg16 = opBuilder.buildInt(setup.m_i64, FlexInt(static_cast<int64_t>(-16)));
                epilogueBuilder.buildTarget(setup.m_descAddi, srcRef, { setup.m_spReg, setup.m_fpReg, neg16 });

                // ld s0, 0(sp)
                MirInteger *off0 = opBuilder.buildInt(setup.m_i64, FlexInt(static_cast<int64_t>(0)));
                epilogueBuilder.buildTarget(setup.m_descLd, srcRef, { setup.m_fpReg, setup.m_spReg, off0 });

                // ld ra, 8(sp)
                MirInteger *off8 = opBuilder.buildInt(setup.m_i64, FlexInt(static_cast<int64_t>(8)));
                epilogueBuilder.buildTarget(setup.m_descLd, srcRef, { setup.m_raReg, setup.m_spReg, off8 });

                // addi sp, sp, 16
                MirInteger *pos16 = opBuilder.buildInt(setup.m_i64, FlexInt(static_cast<int64_t>(16)));
                epilogueBuilder.buildTarget(setup.m_descAddi, srcRef, { setup.m_spReg, setup.m_spReg, pos16 });
            }
            else if (setup.m_analysisData && setup.m_analysisData->m_totalFrameSize > 0)
            {
                int64_t frameSize = static_cast<int64_t>(setup.m_analysisData->m_totalFrameSize);
                frameSize = (frameSize + 15) & ~15LL;
                MirInteger *sizeImm = opBuilder.buildInt(setup.m_i64, FlexInt(frameSize));
                epilogueBuilder.buildTarget(setup.m_descAddi, srcRef, { setup.m_spReg, setup.m_spReg, sizeImm });
            }
        }
    }
}

bool RiscV64FrameLowerer::lowerAlloc(FrameLowererCtx &ctx)
{
    MirInstruction *allocInst = *ctx.m_allocIt;
    if (!allocInst || allocInst->getOperandCount() < 1)
    {
        return false;
    }

    MirRegister *dst = allocInst->getOpAs<MirRegister>(0);
    if (!dst)
    {
        return false;
    }

    MirType *allocType = nullptr;
    if (dst->getMirType() && dst->getMirType()->getKind() == MirTypeKind::Pointer)
    {
        allocType = dst->getMirType()->getPointedType();
    }
    if (!allocType || allocType->getKind() == MirTypeKind::Void)
    {
        allocType = ctx.m_ctx->getTypeTable()->i64();
    }

    StackFrameObject *stackObj = ctx.m_targetFunc->getStackFrame()->createStaticStackObj(allocType);
    MirOperandBuilder opBuilder(ctx.m_ctx);
    MirReference *slotRef = opBuilder.buildRef(stackObj, allocInst->getSourceRef());

    CallingConvDesc *cc = ctx.m_targetFunc->getCallingConv();
    MirRegisterRef baseRef = cc ? cc->getFramePointerReg() : MirRegisterRef();
    MirType *ptrType = ctx.m_ctx->getTypeTable()->getPtr(ctx.m_ctx->getTypeTable()->_void());
    MirRegister *baseReg = opBuilder.buildPhysReg(ptrType, baseRef.getId(), "s0", baseRef.getClass());

    const auto *descADDI64ri = EzTargets::RiscV64::riscv64TargetInst::getTargetDesc(EzTargets::RiscV64::riscv64TargetInst::ADDI64ri);

    allocInst->setOpcode(MirInstructionOpCode::TARGET_INST);
    allocInst->setTargetDesc(descADDI64ri);

    MirInstructionBuilder ib(ctx.m_ctx, allocInst, InsertionType::InsertBefore);
    ib.clearOperands(allocInst);
    ib.addOperand(allocInst, dst);
    ib.addOperand(allocInst, baseReg);
    ib.addOperand(allocInst, slotRef);

    return true;
}

bool RiscV64FrameLowerer::lowerDAlloc(FrameLowererCtx &ctx)
{
    MirInstruction *dallocInst = *ctx.m_allocIt;
    if (!dallocInst || dallocInst->getOperandCount() < 2)
    {
        return false;
    }

    MirRegister *dst = dallocInst->getOpAs<MirRegister>(0);
    MirRegister *src = dallocInst->getOpAs<MirRegister>(1);
    if (!dst || !src)
    {
        return false;
    }

    CallingConvDesc *cc = ctx.m_targetFunc->getCallingConv();
    if (!cc)
    {
        return false;
    }

    MirOperandBuilder opBuilder(ctx.m_ctx);
    MirType *ptrType = ctx.m_ctx->getTypeTable()->getPtr(ctx.m_ctx->getTypeTable()->_void());
    MirRegisterRef spRef = cc->getStackPointerReg();
    MirRegister *spReg = opBuilder.buildPhysReg(ptrType, spRef.getId(), "sp", spRef.getClass());

    const auto *descSUB64rr = EzTargets::RiscV64::riscv64TargetInst::getTargetDesc(EzTargets::RiscV64::riscv64TargetInst::SUB64rr);
    const auto *descADDI64ri = EzTargets::RiscV64::riscv64TargetInst::getTargetDesc(EzTargets::RiscV64::riscv64TargetInst::ADDI64ri);

    MirInstructionBuilder ib(ctx.m_ctx, dallocInst, InsertionType::InsertBefore);
    // 1. sub sp, sp, src
    ib.buildTarget(descSUB64rr, dallocInst->getSourceRef(), { spReg, spReg, src });

    // 2. addi dst, sp, 0
    dallocInst->setOpcode(MirInstructionOpCode::TARGET_INST);
    dallocInst->setTargetDesc(descADDI64ri);
    MirInteger *imm0 = opBuilder.buildInt(ctx.m_ctx->getTypeTable()->i64(), FlexInt(static_cast<int64_t>(0)));
    ib.clearOperands(dallocInst);
    ib.addOperand(dallocInst, dst);
    ib.addOperand(dallocInst, spReg);
    ib.addOperand(dallocInst, imm0);

    return true;
}

} // namespace EzTargets::RiscV64
