#include "EzTripleTestSuite.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticLogger.h"
#include "Function/ArgumentLocationDesc.h"
#include "Function/CallLoweringState.h"
#include "Function/MirFunctionBuilder.h"
#include "Block/MirBlockBuilder.h"
#include "Instruction/MirInstruction.h"
#include "Operand/MirOperand.h"
#include "Operand/MirOperandBuilder.h"
#include "Type/MirTypeTable.h"

MockCallingConvDesc::MockCallingConvDesc(MirBuilderContext *ctx, MirRegisterClass *gprClass) :
    m_calleeSaved(ctx->getGlobalAllocator()), m_callerSaved(ctx->getGlobalAllocator())
{
    m_raxRef = MirRegisterRef(gprClass, 0);
    m_rdiRef = MirRegisterRef(gprClass, 1);
    m_rsiRef = MirRegisterRef(gprClass, 2);
    m_rdxRef = MirRegisterRef(gprClass, 3);
    m_rcxRef = MirRegisterRef(gprClass, 4);
    m_r8Ref = MirRegisterRef(gprClass, 5);
    m_r9Ref = MirRegisterRef(gprClass, 6);
    m_rbpRef = MirRegisterRef(gprClass, 7);
    m_rspRef = MirRegisterRef(gprClass, 8);

    // Argument register sequence: RDI, RSI, RDX, RCX, R8, R9
    m_callerSaved.push_back(m_rdiRef);
    m_callerSaved.push_back(m_rsiRef);
    m_callerSaved.push_back(m_rdxRef);
    m_callerSaved.push_back(m_rcxRef);
    m_callerSaved.push_back(m_r8Ref);
    m_callerSaved.push_back(m_r9Ref);
    m_callerSaved.push_back(m_raxRef);

    // Callee saved: RBP, RSP
    m_calleeSaved.push_back(m_rbpRef);
    m_calleeSaved.push_back(m_rspRef);
}

ArgumentLocationDesc MockCallingConvDesc::getArgLoc(MirType *type, CallLoweringState *callState)
{
    size_t sizeInBytes = (type ? type->getTotalSizeInBits() + 7 : 64) / 8;
    MirRegisterRef reg;
    if (callState && m_raxRef.getClass() && callState->allocate(m_raxRef.getClass(), reg))
    {
        return ArgumentLocationDesc::Reg(reg, sizeInBytes);
    }
    return ArgumentLocationDesc::Stack(sizeInBytes, callState ? callState->allocateStack(type) : nullptr);
}

ArgumentLocationDesc MockCallingConvDesc::getReturnLoc(MirType *type, CallLoweringState *callState)
{
    size_t sizeInBytes = (type ? type->getTotalSizeInBits() + 7 : 64) / 8;
    return ArgumentLocationDesc::Reg(m_raxRef, sizeInBytes);
}

bool MockCallingConvDesc::canReturnInRegs(MirType *type) const
{
    if (!type)
        return true;

    return type->getTotalSizeInBits() <= 128;
}

#include "Instruction/MirInstructionBuilder.h"
#include "Instruction/MirTargetInstructionDesc.h"

bool MockInstructionSelector::select(MirBuilderContext *ctx, MirInstruction *inst)
{
    if (!inst)
        return false;

    if (inst->isSelected())
        return true;

    MirInstructionOpCode op = inst->getOpCode();
    auto *mockTarget = reinterpret_cast<MockTargetDesc *>(m_targetDesc);
    MirRegisterClass *gpr = mockTarget ? mockTarget->getGprClass() : nullptr;
    SourceReference *srcRef = inst->getSourceRef();

    MirInstructionBuilder ib(ctx, inst, InsertionType::InsertBefore);
    MirOperandBuilder ob(ctx);

    switch (op)
    {
        case MirInstructionOpCode::ADD:
        {
            auto *dst = inst->getOpAs<MirRegister>(0);
            auto *src1 = inst->getOpAs<MirRegister>(1);
            auto *src2Reg = inst->getOpAs<MirRegister>(2);
            auto *src2Imm = inst->getOpAs<MirInteger>(2);

            // Check for memory fold on src2
            if (src2Reg)
            {
                MirInstruction *def = getDefiningInstruction(src2Reg);
                if (def && def->getOpCode() == MirInstructionOpCode::LOAD && hasOneUse(src2Reg) && noInterveningStore(def, inst))
                {
                    if (dst && gpr) dst->setClass(gpr);
                    if (src1 && gpr) src1->setClass(gpr);
                    MirOperand *memOp = def->getOperand(1);
                    if (auto *mem = memOp ? memOp->get<MirMemory>() : nullptr)
                    {
                        if (mem->getBase() && gpr) mem->getBase()->setClass(gpr);
                        if (mem->getIndex() && gpr) mem->getIndex()->setClass(gpr);
                    }
                    ib.buildTarget(mockTarget ? mockTarget->getDescADD64rm() : nullptr, srcRef, { dst, src1, memOp });
                    def->eraseFromOwner();
                    inst->eraseFromOwner();
                    m_selectedCount++;
                    m_foldedCount++;
                    return true;
                }
            }

            // Commutative check for memory fold on src1
            if (src1 && src2Reg)
            {
                MirInstruction *def = getDefiningInstruction(src1);
                if (def && def->getOpCode() == MirInstructionOpCode::LOAD && hasOneUse(src1) && noInterveningStore(def, inst))
                {
                    if (dst && gpr) dst->setClass(gpr);
                    if (src2Reg && gpr) src2Reg->setClass(gpr);
                    MirOperand *memOp = def->getOperand(1);
                    if (auto *mem = memOp ? memOp->get<MirMemory>() : nullptr)
                    {
                        if (mem->getBase() && gpr) mem->getBase()->setClass(gpr);
                        if (mem->getIndex() && gpr) mem->getIndex()->setClass(gpr);
                    }
                    ib.buildTarget(mockTarget ? mockTarget->getDescADD64rm() : nullptr, srcRef, { dst, src2Reg, memOp });
                    def->eraseFromOwner();
                    inst->eraseFromOwner();
                    m_selectedCount++;
                    m_foldedCount++;
                    return true;
                }
            }

            if (src2Imm)
            {
                if (dst && gpr) dst->setClass(gpr);
                if (src1 && gpr) src1->setClass(gpr);
                ib.buildTarget(mockTarget ? mockTarget->getDescADD64ri() : nullptr, srcRef, { dst, src1, src2Imm });
                inst->eraseFromOwner();
                m_selectedCount++;
                return true;
            }

            if (src2Reg)
            {
                if (dst && gpr) dst->setClass(gpr);
                if (src1 && gpr) src1->setClass(gpr);
                if (src2Reg && gpr) src2Reg->setClass(gpr);
                ib.buildTarget(mockTarget ? mockTarget->getDescADD64rr() : nullptr, srcRef, { dst, src1, src2Reg });
                inst->eraseFromOwner();
                m_selectedCount++;
                return true;
            }
            break;
        }
        case MirInstructionOpCode::SUB:
        {
            auto *dst = inst->getOpAs<MirRegister>(0);
            auto *src1 = inst->getOpAs<MirRegister>(1);
            auto *src2 = inst->getOpAs<MirRegister>(2);
            if (dst && gpr) dst->setClass(gpr);
            if (src1 && gpr) src1->setClass(gpr);
            if (src2 && gpr) src2->setClass(gpr);
            ib.buildTarget(mockTarget ? mockTarget->getDescSUB64rr() : nullptr, srcRef, { dst, src1, src2 });
            inst->eraseFromOwner();
            m_selectedCount++;
            return true;
        }
        case MirInstructionOpCode::LOAD:
        {
            auto *dst = inst->getOpAs<MirRegister>(0);
            auto *memOp = inst->getOperand(1);
            MatchedAddressingMode mode;
            if (foldAddressingMode(ctx, memOp, mode, inst))
            {
                if (!mode.m_foldedInstructions.empty())
                {
                    MirInteger *dispOp = mode.m_disp != 0 ? ob.buildInt(ctx->getTypeTable()->i64(), FlexInt(mode.m_disp), srcRef) : nullptr;
                    memOp = ob.buildMem(ctx->getTypeTable()->i64(), mode.m_base, dispOp, mode.m_index, mode.m_scale, srcRef);
                    eraseFoldedInstructions(mode.m_foldedInstructions);
                    m_foldedCount += mode.m_foldedInstructions.size();
                }
            }
            if (dst && gpr) dst->setClass(gpr);
            if (auto *mem = memOp ? memOp->get<MirMemory>() : nullptr)
            {
                if (mem->getBase() && gpr) mem->getBase()->setClass(gpr);
                if (mem->getIndex() && gpr) mem->getIndex()->setClass(gpr);
            }
            ib.buildTarget(mockTarget ? mockTarget->getDescLOAD64() : nullptr, srcRef, { dst, memOp });
            inst->eraseFromOwner();
            m_selectedCount++;
            return true;
        }
        case MirInstructionOpCode::STORE:
        {
            auto *memOp = inst->getOperand(0);
            auto *val = inst->getOpAs<MirRegister>(1);
            MatchedAddressingMode mode;
            if (foldAddressingMode(ctx, memOp, mode, inst))
            {
                if (!mode.m_foldedInstructions.empty())
                {
                    MirInteger *dispOp = mode.m_disp != 0 ? ob.buildInt(ctx->getTypeTable()->i64(), FlexInt(mode.m_disp), srcRef) : nullptr;
                    memOp = ob.buildMem(ctx->getTypeTable()->i64(), mode.m_base, dispOp, mode.m_index, mode.m_scale, srcRef);
                    eraseFoldedInstructions(mode.m_foldedInstructions);
                    m_foldedCount += mode.m_foldedInstructions.size();
                }
            }
            if (val && gpr) val->setClass(gpr);
            if (auto *mem = memOp ? memOp->get<MirMemory>() : nullptr)
            {
                if (mem->getBase() && gpr) mem->getBase()->setClass(gpr);
                if (mem->getIndex() && gpr) mem->getIndex()->setClass(gpr);
            }
            ib.buildTarget(mockTarget ? mockTarget->getDescSTORE64() : nullptr, srcRef, { memOp, val });
            inst->eraseFromOwner();
            m_selectedCount++;
            return true;
        }
        case MirInstructionOpCode::BR_COND:
        {
            auto *cond = inst->getOpAs<MirRegister>(0);
            auto *trueTarget = inst->getOperand(1);
            auto *falseTarget = inst->getOperand(2);
            if (cond && gpr) cond->setClass(gpr);
            ib.buildTarget(mockTarget ? mockTarget->getDescBR_COND() : nullptr, srcRef, { cond, trueTarget, falseTarget });
            inst->eraseFromOwner();
            m_selectedCount++;
            return true;
        }
        case MirInstructionOpCode::MOV:
        {
            auto *dst = inst->getOpAs<MirRegister>(0);
            auto *src = inst->getOperand(1);
            if (dst && gpr) dst->setClass(gpr);
            if (auto *srcReg = src ? src->get<MirRegister>() : nullptr)
            {
                if (gpr) srcReg->setClass(gpr);
            }
            ib.buildTarget(mockTarget ? mockTarget->getDescMOV64rr() : nullptr, srcRef, { dst, src });
            inst->eraseFromOwner();
            m_selectedCount++;
            return true;
        }
        case MirInstructionOpCode::RET:
        {
            std::vector<MirOperand *> ops;
            for (size_t i = 0; i < inst->getOperandCount(); ++i)
            {
                auto *opnd = inst->getOperand(i);
                if (auto *reg = opnd ? opnd->get<MirRegister>() : nullptr)
                {
                    if (gpr) reg->setClass(gpr);
                }
                ops.push_back(opnd);
            }
            ib.buildTarget(mockTarget ? mockTarget->getDescRET() : nullptr, srcRef, ops);
            inst->eraseFromOwner();
            m_selectedCount++;
            return true;
        }
        default:
        {
            for (size_t i = 0; i < inst->getOperandCount(); ++i)
            {
                auto *opnd = inst->getOperand(i);
                if (auto *reg = opnd ? opnd->get<MirRegister>() : nullptr)
                {
                    if (gpr) reg->setClass(gpr);
                }
            }
            std::vector<MirOperand *> ops;
            for (size_t i = 0; i < inst->getOperandCount(); ++i)
            {
                ops.push_back(inst->getOperand(i));
            }
            ib.buildTarget(mockTarget ? mockTarget->getDescADD64rr() : nullptr, srcRef, ops);
            inst->eraseFromOwner();
            m_selectedCount++;
            return true;
        }
    }

    return false;
}

MockTargetDesc::MockTargetDesc(MirBuilderContext *ctx) :
    m_ctx(ctx), m_banks(ctx->getGlobalAllocator()), m_convs(ctx->getGlobalAllocator()), m_modeMatcher(&m_isel)
{
    m_isel.setTargetDesc(this);

    std::pmr::polymorphic_allocator<> alloc(ctx->getGlobalAllocator());

    m_gprBank = alloc.new_object<MirRegisterBank>("GPRBank", ctx->getGlobalAllocator());
    m_gprClass = alloc.new_object<MirRegisterClass>("GPR64", m_gprBank, ctx->getGlobalAllocator());
    m_gprBank->addClass("GPR64", m_gprClass);

    m_gprClass->addRegister("rax", 64, 0, {});
    m_gprClass->addRegister("rdi", 64, 0, {});
    m_gprClass->addRegister("rsi", 64, 0, {});
    m_gprClass->addRegister("rdx", 64, 0, {});
    m_gprClass->addRegister("rcx", 64, 0, {});
    m_gprClass->addRegister("r8", 64, 0, {});
    m_gprClass->addRegister("r9", 64, 0, {});
    m_gprClass->addRegister("rbp", 64, 0, {});
    m_gprClass->addRegister("rsp", 64, 0, {});
    m_gprClass->addRegister("rbx", 64, 0, {});
    m_gprClass->addRegister("r10", 64, 0, {});
    m_gprClass->addRegister("r11", 64, 0, {});
    m_gprClass->addRegister("r12", 64, 0, {});
    m_gprClass->addRegister("r13", 64, 0, {});
    m_gprClass->addRegister("r14", 64, 0, {});
    m_gprClass->addRegister("r15", 64, 0, {});

    m_mockCc = std::make_unique<MockCallingConvDesc>(ctx, m_gprClass);
    m_legalizer = std::make_unique<MirLegalizer>(ctx, this);

    m_banks.push_back(m_gprBank);
    m_convs.push_back(m_mockCc.get());

    m_descADD64rr = std::make_unique<MirTargetInstructionDesc>(
        "ADD64rr", 1,
        std::initializer_list<MirOperandFlag>{ MirOperandFlag::Write, MirOperandFlag::Read, MirOperandFlag::Read },
        std::initializer_list<MirRegisterClass *>{ m_gprClass, m_gprClass, m_gprClass },
        std::initializer_list<MirRegisterRef>{},
        std::initializer_list<MirRegisterRef>{},
        MirInstructionFlags::IsCommutative);

    m_descADD64ri = std::make_unique<MirTargetInstructionDesc>(
        "ADD64ri", 2,
        std::initializer_list<MirOperandFlag>{ MirOperandFlag::Write, MirOperandFlag::Read, MirOperandFlag::Read },
        std::initializer_list<MirRegisterClass *>{ m_gprClass, m_gprClass, nullptr },
        std::initializer_list<MirRegisterRef>{},
        std::initializer_list<MirRegisterRef>{});

    m_descADD64rm = std::make_unique<MirTargetInstructionDesc>(
        "ADD64rm", 3,
        std::initializer_list<MirOperandFlag>{ MirOperandFlag::Write, MirOperandFlag::Read, MirOperandFlag::Read },
        std::initializer_list<MirRegisterClass *>{ m_gprClass, m_gprClass, nullptr },
        std::initializer_list<MirRegisterRef>{},
        std::initializer_list<MirRegisterRef>{},
        MirInstructionFlags::ReadsMemory);

    m_descSUB64rr = std::make_unique<MirTargetInstructionDesc>(
        "SUB64rr", 4,
        std::initializer_list<MirOperandFlag>{ MirOperandFlag::Write, MirOperandFlag::Read, MirOperandFlag::Read },
        std::initializer_list<MirRegisterClass *>{ m_gprClass, m_gprClass, m_gprClass });

    m_descLOAD64 = std::make_unique<MirTargetInstructionDesc>(
        "LOAD64", 5,
        std::initializer_list<MirOperandFlag>{ MirOperandFlag::Write, MirOperandFlag::Read },
        std::initializer_list<MirRegisterClass *>{ m_gprClass, nullptr },
        std::initializer_list<MirRegisterRef>{},
        std::initializer_list<MirRegisterRef>{},
        MirInstructionFlags::ReadsMemory);

    m_descSTORE64 = std::make_unique<MirTargetInstructionDesc>(
        "STORE64", 6,
        std::initializer_list<MirOperandFlag>{ MirOperandFlag::Write, MirOperandFlag::Read },
        std::initializer_list<MirRegisterClass *>{ nullptr, m_gprClass },
        std::initializer_list<MirRegisterRef>{},
        std::initializer_list<MirRegisterRef>{},
        MirInstructionFlags::WritesMemory);

    m_descBR_COND = std::make_unique<MirTargetInstructionDesc>(
        "BR_COND", 7,
        std::initializer_list<MirOperandFlag>{ MirOperandFlag::Read, MirOperandFlag::Read, MirOperandFlag::Read },
        std::initializer_list<MirRegisterClass *>{ m_gprClass, nullptr, nullptr },
        std::initializer_list<MirRegisterRef>{},
        std::initializer_list<MirRegisterRef>{},
        MirInstructionFlags::IsBranch);

    m_descRET = std::make_unique<MirTargetInstructionDesc>(
        "RET", 8,
        std::initializer_list<MirOperandFlag>{ MirOperandFlag::Read },
        std::initializer_list<MirRegisterClass *>{ m_gprClass },
        std::initializer_list<MirRegisterRef>{},
        std::initializer_list<MirRegisterRef>{},
        MirInstructionFlags::IsTerminator | MirInstructionFlags::IsReturn);

    m_descMOV64rr = std::make_unique<MirTargetInstructionDesc>(
        "MOV64rr", 9,
        std::initializer_list<MirOperandFlag>{ MirOperandFlag::Write, MirOperandFlag::Read },
        std::initializer_list<MirRegisterClass *>{ m_gprClass, m_gprClass });
}

MirType *MockTargetDesc::getMemOperandDisplacementType() { return m_ctx ? m_ctx->getTypeTable()->i64() : nullptr; }

void EzTripleTestSuite::SetUp()
{
    m_diagCollector = std::make_unique<DiagnosticCollector>();
    m_typeTable = std::make_unique<MirTypeTable>(&m_arena);
    m_builderCtx = std::make_unique<MirBuilderContext>(nullptr, m_diagCollector.get(), m_typeTable.get(), &m_arena);
    m_targetDesc = std::make_unique<MockTargetDesc>(m_builderCtx.get());
    m_sourceManager = std::make_unique<SourceManager>(std::filesystem::current_path(), &m_arena);
    m_diagLogger = std::make_unique<DiagnosticLogger>(m_sourceManager.get());
    m_typeTable->initialize(64);
    m_builderCtx->setDefaultCallingConvention(m_targetDesc->getMockCallingConv());

    m_targetDesc->initialize();

    m_diagCollector->addListener(m_diagLogger.get());
    m_diagCollector->enableDiag(Diag_Trace);
    m_diagCollector->enableDiag(Diag_Debug);
}

void EzTripleTestSuite::TearDown()
{
    m_diagLogger.reset();
    m_targetDesc.reset();
    m_builderCtx.reset();
    m_typeTable.reset();
    m_diagCollector.reset();
    m_sourceManager.reset();
}

MirFunction *EzTripleTestSuite::createTestFunction(const std::string_view &name, MirType *retType)
{
    if (!retType)
    {
        retType = m_typeTable->i32();
    }
    MirFunctionBuilder builder(m_builderCtx.get());
    return builder.build(retType, {}, name);
}

MirBlock *EzTripleTestSuite::createBlock(MirFunction *func, const std::string_view &name)
{
    MirBlockBuilder builder(m_builderCtx.get(), func);
    return builder.build(nullptr, name);
}
