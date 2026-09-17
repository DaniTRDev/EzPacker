#ifndef EZTRIPLETESTSUITE_EZ_TRIPLE_TEST_SUITE_H
#define EZTRIPLETESTSUITE_EZ_TRIPLE_TEST_SUITE_H

#include "gtest/gtest.h"
#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Descriptors/TargetDesc.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticLogger.h"
#include "Function/CallingConvDesc.h"
#include "Function/MirFunction.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "InstructionSelector/MirInstructionSelector.h"
#include "Legalizer/MirLegalizer.h"
#include "Legalizer/LegalizerInfo.h"
#include "Legalizer/Actions/LegalizeCallAction.h"
#include "Legalizer/Actions/LegalizeReturnAction.h"
#include "MirPasses/MirPassManager.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Operand/MirRegisterBank.h"
#include "Operand/MirRegisterClass.h"
#include "SourceManager/SourceManager.h"
#include "Type/MirTypeTable.h"

/**
 * Mock Calling Convention implementation (AMD64 System-V style).
 */
class MockCallingConvDesc : public CallingConvDesc
{
  public:
    MockCallingConvDesc(MirBuilderContext *ctx, MirRegisterClass *gprClass);

    ArgumentLocationDesc getArgLoc(MirType *type, CallLoweringState *callState) override;
    ArgumentLocationDesc getReturnLoc(MirType *type, CallLoweringState *callState) override;
    bool canReturnInRegs(MirType *type) const override;
    bool isCalleeCleanup() const override { return false; }
    bool doesStackGrowsDownwards() const override { return true; }
    bool hasFramePointer(MirFunction *func) const override { return true; }
    const char *getName() const override { return "MockCallingConv"; }
    MirRegisterRef getFramePointerReg() const override { return m_rbpRef; }
    MirRegisterRef getStackPointerReg() const override { return m_rspRef; }
    size_t getStackAlignment() const override { return 16; }
    size_t getShadowSpaceSize() const override { return 0; }
    void classify(MirType *type, std::pmr::vector<CallingConvTypeClass> &out) const override {}

    const std::pmr::vector<MirRegisterRef> &getAllCalleeSavedRegs() override { return m_calleeSaved; }
    const std::pmr::vector<MirRegisterRef> &getCalleeSavedRegs(MirRegisterClass *_class) override
    {
        return m_calleeSaved;
    }
    const std::pmr::vector<MirRegisterRef> &getAllCallerSavedRegs() override { return m_callerSaved; }
    const std::pmr::vector<MirRegisterRef> &getCallerSavedRegs(MirRegisterClass *_class) override
    {
        return m_callerSaved;
    }

    MirRegisterRef getRax() const { return m_raxRef; }
    MirRegisterRef getRdi() const { return m_rdiRef; }
    MirRegisterRef getRsi() const { return m_rsiRef; }
    MirRegisterRef getRdx() const { return m_rdxRef; }

  private:
    MirRegisterRef m_raxRef;
    MirRegisterRef m_rdiRef;
    MirRegisterRef m_rsiRef;
    MirRegisterRef m_rdxRef;
    MirRegisterRef m_rcxRef;
    MirRegisterRef m_r8Ref;
    MirRegisterRef m_r9Ref;
    MirRegisterRef m_rbpRef;
    MirRegisterRef m_rspRef;

    std::pmr::vector<MirRegisterRef> m_calleeSaved;
    std::pmr::vector<MirRegisterRef> m_callerSaved;
};

/**
 * Mock Instruction Selector.
 */
class MockInstructionSelector : public MirInstructionSelector
{
  public:
    bool select(MirBuilderContext *ctx, MirInstruction *inst) override;

    size_t m_selectedCount{ 0 };
};

#include "FrameLowerer/MirFrameLowerer.h"

/**
 * Mock Frame Lowerer.
 */
class MockFrameLowerer : public MirFrameLowerer
{
  public:
    void insertPrologue(FrameLowererCtx &ctx) override
    {
        m_prologueInserted = true;
    }
    void insertEpilogue(FrameLowererCtx &ctx) override
    {
        m_epilogueInserted = true;
    }
    bool lowerAlloc(FrameLowererCtx &ctx) override
    {
        m_allocLoweredCount++;
        return true;
    }
    bool lowerDAlloc(FrameLowererCtx &ctx) override
    {
        m_dallocLoweredCount++;
        return true;
    }

    void reset()
    {
        m_prologueInserted = false;
        m_epilogueInserted = false;
        m_allocLoweredCount = 0;
        m_dallocLoweredCount = 0;
    }

    bool m_prologueInserted{ false };
    bool m_epilogueInserted{ false };
    size_t m_allocLoweredCount{ 0 };
    size_t m_dallocLoweredCount{ 0 };
};

#include "RegisterAllocator/MirRegisterAllocator.h"
#include "Instruction/MirInstructionSet.h"

/**
 * Mock Register Allocator.
 */
class MockRegisterAllocator : public MirRegisterAllocator
{
  public:
    size_t m_spillCount{ 0 };
    size_t m_reloadCount{ 0 };
    size_t m_rematCount{ 0 };

    void reset()
    {
        m_spillCount = 0;
        m_reloadCount = 0;
        m_rematCount = 0;
    }

  protected:
    bool isInstructionDAlloc(MirInstruction *instr) override
    {
        return instr && instr->getOpCode() == MirInstructionOpCode::DALLOC;
    }

    bool isRematerializable(MirRegister *vreg, MirInstruction *definingInst) override
    {
        if (!definingInst)
            return false;

        if (definingInst->getOpCode() == MirInstructionOpCode::MOV && definingInst->getOperands().size() >= 2)
        {
            auto *srcOp = definingInst->getOperands()[1];
            return srcOp && (srcOp->getType() == MirOperandType::Integer || srcOp->getType() == MirOperandType::FloatingPoint);
        }
        return false;
    }

    MirInstruction *emitReload(RegisterAllocatorCtx *ctx,
                               MirBlock *block,
                               IntrusiveLinkedList<MirInstruction>::iterator it,
                               SourceReference *srcRef,
                               MirRegister *dstReg,
                               StackFrameObject *spillSlot) override
    {
        m_reloadCount++;
        MirOperandBuilder opBuilder(ctx->m_ctx);
        MirInstructionBuilder iBuilder(ctx->m_ctx, block, InsertionType::InsertBefore, it);
        MirReference *slotRef = opBuilder.buildRef(spillSlot, srcRef);
        return iBuilder.LOAD(srcRef, dstReg, slotRef);
    }

    MirInstruction *emitSpill(RegisterAllocatorCtx *ctx,
                              MirBlock *block,
                              IntrusiveLinkedList<MirInstruction>::iterator it,
                              SourceReference *srcRef,
                              StackFrameObject *spillSlot,
                              MirRegister *srcReg) override
    {
        m_spillCount++;
        MirOperandBuilder opBuilder(ctx->m_ctx);
        MirInstructionBuilder iBuilder(ctx->m_ctx, block, InsertionType::InsertAfter, it);
        MirReference *slotRef = opBuilder.buildRef(spillSlot, srcRef);
        return iBuilder.STORE(srcRef, slotRef, srcReg);
    }

    MirInstruction *reMaterialize(RegisterAllocatorCtx *ctx,
                                  MirBlock *block,
                                  IntrusiveLinkedList<MirInstruction>::iterator it,
                                  SourceReference *srcRef,
                                  MirRegister *dstReg,
                                  MirInstruction *defInst) override
    {
        m_rematCount++;
        MirInstructionBuilder iBuilder(ctx->m_ctx, block, InsertionType::InsertBefore, it);
        std::pmr::vector<MirOperand *> ops(ctx->m_allocator);
        ops.push_back(dstReg);
        for (size_t i = 1; i < defInst->getOperands().size(); ++i)
        {
            ops.push_back(defInst->getOperands()[i]);
        }
        return iBuilder.build(defInst->getOpCode(), srcRef, ops);
    }
};



/**
 * Modern fluent table-driven legalizer specification for MockTarget.
 */
class MockTargetLegalizerInfo : public LegalizerInfo
{
  public:
    MockTargetLegalizerInfo(MirTypeTable *tt)
    {
        auto *i1 = tt->i1();
        auto *i8 = tt->i8();
        auto *i16 = tt->i16();
        auto *i32 = tt->i32();
        auto *i64 = tt->i64();
        auto *i128 = tt->i128();
        auto *i256 = tt->i256();
        auto *f32 = tt->f32();
        auto *f64 = tt->f64();
        auto *bindToken = tt->__bindToken();

        // 1. Homogeneous Arithmetic & Bitwise (ADD, SUB, XOR, AND, OR)
        getActionDefinitions({ MirInstructionOpCode::ADD,
                               MirInstructionOpCode::SUB,
                               MirInstructionOpCode::XOR,
                               MirInstructionOpCode::AND,
                               MirInstructionOpCode::OR })
            .legalFor({ i32, i64, f32, f64 })
            .widenScalarTo(0, { i1, i8, i16 }, i32)
            .narrowScalarTo(0, { i128, i256 }, i64);

        // 2. Division & Libcalls
        getActionDefinitions({ MirInstructionOpCode::DIV, MirInstructionOpCode::IDIV })
            .legalFor({ i32 })
            .widenScalarTo(0, { i1, i8, i16 }, i32)
            .libcallFor(i64, "__divdi3")
            .narrowScalarTo(0, { i128 }, i64);

        // 3. Comparisons
        getActionDefinitions({ MirInstructionOpCode::CMP_EQ, MirInstructionOpCode::CMP_NE })
            .legalForTypesWithSource({ i32, i64, f32, f64 })
            .widenScalarSourceTo({ i1, i8, i16 }, i32)
            .narrowScalarSourceTo({ i128 }, i64);

        // 4. Extensions & Truncations
        getActionDefinitions(MirInstructionOpCode::ZEXT)
            .legalFor({ { i8, i1 }, { i16, i1 }, { i32, i1 }, { i64, i1 }, { i32, i8 }, { i32, i16 }, { i64, i8 }, { i64, i16 }, { i64, i32 } });

        getActionDefinitions(MirInstructionOpCode::SEXT)
            .legalFor({ { i32, i8 }, { i32, i16 }, { i64, i8 }, { i64, i16 }, { i64, i32 } })
            .widenScalarTo(1, { i1 }, i8);

        getActionDefinitions(MirInstructionOpCode::TRUNC)
            .legalFor({ { i8, i32 }, { i16, i32 }, { i8, i64 }, { i16, i64 }, { i32, i64 }, { i1, i32 }, { i1, i64 }, { i8, i16 }, { i1, i8 }, { i1, i16 } });

        // 5. Data Movement & Bitcasts
        getActionDefinitions(MirInstructionOpCode::MOV)
            .legalIfSameType()
            .bitcastBetween(i32, f32);

        // 6. High-Level Procedural Lowerings
        getActionDefinitions(MirInstructionOpCode::CALL)
            .lowerWith(&LegalizeActions::LegalizeCall);

        getActionDefinitions(MirInstructionOpCode::RET)
            .lowerWith(&LegalizeActions::LegalizeReturn);

        // 7. ABI Token Lowering Primitives
        getActionDefinitions({ MirInstructionOpCode::POP_ARG,
                               MirInstructionOpCode::PUSH_ARG,
                               MirInstructionOpCode::PUSH_RET,
                               MirInstructionOpCode::POP_RET })
            .legalFor({ { bindToken, i32 }, { bindToken, i64 }, { bindToken, f32 }, { bindToken, f64 }, { bindToken, i8 }, { bindToken, i16 } });

        getActionDefinitions(MirInstructionOpCode::END_ARG)
            .legalFor({ bindToken });
    }
};

/**
 * Mock Target Descriptor.
 */
class MockTargetDesc : public TargetDesc
{
  public:
    MockTargetDesc(MirBuilderContext *ctx);

    const char *getName() const override { return "MockTarget"; }
    MirFrameLowerer *getFrameLowerer() override { return &m_frameLowerer; }
    MirInstructionSelector *getInstructionSelector() override { return &m_isel; }
    MirLegalizer *getLegalizer() override { return m_legalizer.get(); }
    LegalizerInfo *getLegalizerInfo() override { return m_legalizerInfo.get(); }
    MirRegisterAllocator *getRegisterAllocator() override { return &m_regAlloc; }
    MockRegisterAllocator *getMockRegisterAllocator() { return &m_regAlloc; }
    MirType *getMemOperandDisplacementType() override;
    MirRegisterRef getInstructionPtrReg() const override { return {}; }
    size_t getStackSlotSize() const override { return 8; }
    void initialize() override
    {
        m_legalizerInfo = std::make_unique<MockTargetLegalizerInfo>(m_ctx->getTypeTable());
    }
    std::string_view getLibcallStr(uint8_t symId) override
    {
        switch (symId)
        {
            case 0:
                return "__returnNothing";
            case 1:
                return "__divdi3";
            case 2:
                return "__divdi3";
            case 3:
                return "__moddi3";
            case 4:
            case 5:
                return "__muldi3";
            default:
                return {};
        }
    }
    std::pmr::vector<TargetBinaryDesc *> getAvailableBinaryDescriptors() override { return {}; }
    std::pmr::vector<CallingConvDesc *> getAvailableCallingConventions() override { return m_convs; }
    std::pmr::vector<MirRegisterBank *> getAvailableRegisterBanks() override { return m_banks; }

    MockCallingConvDesc *getMockCallingConv() { return m_mockCc.get(); }
    MockInstructionSelector *getMockInstructionSelector() { return &m_isel; }
    MockFrameLowerer *getMockFrameLowerer() { return &m_frameLowerer; }
    MirRegisterClass *getGprClass() { return m_gprClass; }

  private:
    std::unique_ptr<LegalizerInfo> m_legalizerInfo;
    MockInstructionSelector m_isel;
    MockFrameLowerer m_frameLowerer;
    MockRegisterAllocator m_regAlloc;
    std::unique_ptr<MockCallingConvDesc> m_mockCc;
    std::unique_ptr<MirLegalizer> m_legalizer;
    MirBuilderContext *m_ctx;
    MirRegisterClass *m_gprClass{ nullptr };
    MirRegisterBank *m_gprBank{ nullptr };
    std::pmr::vector<MirRegisterBank *> m_banks;
    std::pmr::vector<CallingConvDesc *> m_convs;
};

/**
 * Base test harness for EzTriple unit tests.
 */
class EzTripleTestSuite : public ::testing::Test
{
  protected:
    void SetUp() override;
    void TearDown() override;

  public:
    MirBuilderContext *getBuilderCtx() { return m_builderCtx.get(); }
    MockTargetDesc *getTargetDesc() { return m_targetDesc.get(); }
    MirFunction *createTestFunction(const std::string_view &name = "test_func", MirType *retType = nullptr);
    MirBlock *createBlock(MirFunction *func, const std::string_view &name = "entry");

  private:
    std::pmr::monotonic_buffer_resource m_arena;
    std::unique_ptr<DiagnosticCollector> m_diagCollector;
    std::unique_ptr<DiagnosticLogger> m_diagLogger;
    std::unique_ptr<MirTypeTable> m_typeTable;
    std::unique_ptr<MirBuilderContext> m_builderCtx;
    std::unique_ptr<MockTargetDesc> m_targetDesc;
    std::unique_ptr<SourceManager> m_sourceManager;
};

#endif // EZTRIPLETESTSUITE_EZ_TRIPLE_TEST_SUITE_H
