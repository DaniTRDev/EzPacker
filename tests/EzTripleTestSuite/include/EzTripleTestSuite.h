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
#include "Instruction/MirInstructionBuilder.h"
#include "InstructionSelector/MirInstructionSelector.h"
#include "Legalizer/MirExpansionRuleRegistry.h"
#include "Legalizer/MirLegalizer.h"
#include "MirPasses/MirPassManager.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Operand/MirRegisterBank.h"
#include "Operand/MirRegisterClass.h"
#include "SourceManager/SourceManager.h"
#include "Type/IMirTargetTypeLayout.h"
#include "Type/MirTypeTable.h"

/**
 * Mock Target Type Layout for 64-bit testing.
 */
class MockTargetTypeLayout : public IMirTargetTypeLayout
{
  public:
    size_t getPointerSizeInBytes() const override { return 8; }

    size_t getTypeAlignmentInBytes(const MirType *type) const override
    {
        if (!type)
            return 1;
        size_t bytes = getTypeSizeInBytes(type);
        if (bytes >= 8)
            return 8;
        if (bytes >= 4)
            return 4;
        if (bytes >= 2)
            return 2;
        return 1;
    }

    size_t getTypeSizeInBytes(const MirType *type) const override
    {
        if (!type)
            return 0;
        return (type->getTotalSizeInBits() + 7) / 8;
    }
};

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
    bool hasFramePointer(MirFunction *func) const override
    {
        (void)func;
        return true;
    }
    const char *getName() const override { return "MockCallingConv"; }
    MirRegisterRef getFramePointerReg() const override { return m_rbpRef; }
    MirRegisterRef getStackPointerReg() const override { return m_rspRef; }
    size_t getStackAlignment() const override { return 16; }
    size_t getShadowSpaceSize() const override { return 0; }
    void classify(MirType *type, std::pmr::vector<CallingConvTypeClass> &out) const override
    {
        (void)type;
        (void)out;
    }

    const std::pmr::vector<MirRegisterRef> &getAllCalleeSavedRegs() override { return m_calleeSaved; }
    const std::pmr::vector<MirRegisterRef> &getCalleeSavedRegs(MirRegisterClass *_class) override
    {
        (void)_class;
        return m_calleeSaved;
    }
    const std::pmr::vector<MirRegisterRef> &getAllCallerSavedRegs() override { return m_callerSaved; }
    const std::pmr::vector<MirRegisterRef> &getCallerSavedRegs(MirRegisterClass *_class) override
    {
        (void)_class;
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
 * Mock Expansion Rule Registry.
 */
class MockExpansionRules : public MirExpansionRuleRegistry
{
  public:
    bool tryExpand(MirBuilderContext *ctx, MirInstruction *inst) override;

    bool m_expandInvoked{ false };
    MirInstructionOpCode m_lastExpandedOpCode{};
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
    void insertPrologue(FrameLowererCtx &ctx) override {}
    void insertEpilogue(FrameLowererCtx &ctx) override {}
    bool lowerAlloc(FrameLowererCtx &ctx) override { return false; }
    bool lowerDAlloc(FrameLowererCtx &ctx) override { return false; }
};

/**
 * Mock Target Descriptor.
 */
class MockTargetDesc : public TargetDesc
{
  public:
    MockTargetDesc(MirBuilderContext *ctx);

    const char *getName() const override { return "MockTarget"; }
    IMirTargetTypeLayout *getTypeLayout() override { return &m_typeLayout; }
    MirExpansionRuleRegistry *getExpansionRegistry() override { return &m_expansionRules; }
    MirFrameLowerer *getFrameLowerer() override { return &m_frameLowerer; }
    MirInstructionSelector *getInstructionSelector() override { return &m_isel; }
    MirLegalizer *getLegalizer() override { return m_legalizer.get(); }
    MirRegisterAllocator *getRegisterAllocator() override { return nullptr; }
    MirType *getMemOperandDisplacementType() override;
    MirRegisterRef getInstructionPtrReg() const override { return {}; }
    size_t getStackSlotSize() const override { return 8; }
    void initialize() override {}
    std::pmr::vector<TargetBinaryDesc *> getAvailableBinaryDescriptors() override { return {}; }
    std::pmr::vector<CallingConvDesc *> getAvailableCallingConventions() override { return m_convs; }
    std::pmr::vector<MirRegisterBank *> getAvailableRegisterBanks() override { return m_banks; }

    MockCallingConvDesc *getMockCallingConv() { return m_mockCc.get(); }
    MockExpansionRules *getMockExpansionRules() { return &m_expansionRules; }
    MockInstructionSelector *getMockInstructionSelector() { return &m_isel; }
    MockFrameLowerer *getMockFrameLowerer() { return &m_frameLowerer; }
    MirRegisterClass *getGprClass() { return m_gprClass; }

  private:
    MockTargetTypeLayout m_typeLayout;
    MockExpansionRules m_expansionRules;
    MockInstructionSelector m_isel;
    MockFrameLowerer m_frameLowerer;
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
