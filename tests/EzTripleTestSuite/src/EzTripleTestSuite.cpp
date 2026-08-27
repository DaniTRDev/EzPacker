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

bool MockExpansionRules::tryExpand(MirBuilderContext *ctx, MirInstruction *inst)
{
    if (!inst)
        return false;

    m_expandInvoked = true;
    m_lastExpandedOpCode = inst->getOpCode();
    return true;
}

bool MockInstructionSelector::select(MirBuilderContext *ctx, MirInstruction *inst)
{
    if (!inst)
        return false;

    m_selectedCount++;
    return true;
}

MockTargetDesc::MockTargetDesc(MirBuilderContext *ctx) :
    m_ctx(ctx), m_banks(ctx->getGlobalAllocator()), m_convs(ctx->getGlobalAllocator())
{
    std::pmr::polymorphic_allocator<> alloc(ctx->getGlobalAllocator());

    m_gprBank = alloc.new_object<MirRegisterBank>("GPRBank", ctx->getGlobalAllocator());
    m_gprClass = alloc.new_object<MirRegisterClass>("GPR64", m_gprBank, ctx->getGlobalAllocator());
    m_gprBank->addClass("GPR64", m_gprClass);

    m_mockCc = std::make_unique<MockCallingConvDesc>(ctx, m_gprClass);
    m_legalizer = std::make_unique<MirLegalizer>(ctx, this);

    m_banks.push_back(m_gprBank);
    m_convs.push_back(m_mockCc.get());
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
    m_typeTable->initialize(m_targetDesc->getTypeLayout());
    m_builderCtx->setDefaultCallingConvention(m_targetDesc->getMockCallingConv());

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
