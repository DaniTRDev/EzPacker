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
#include "Legalizer/MirLegalizeActionTable.h"
#include "Legalizer/MirLegalizer.h"
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
    void insertPrologue(FrameLowererCtx &ctx) override {}
    void insertEpilogue(FrameLowererCtx &ctx) override {}
    bool lowerAlloc(FrameLowererCtx &ctx) override { return false; }
    bool lowerDAlloc(FrameLowererCtx &ctx) override { return false; }
};

namespace
{

LegalizeQueryResult defaultUnsupportedQuery(size_t /*op1Type*/, size_t /*op2Type*/, size_t /*op3Type*/)
{
    return LegalizeQueryResult{ .m_action = LegalizeAction::Unsupported,
                                .m_compactId = 0,
                                .m_slot = 0,
                                .m_libcallOffset = 0 };
}

// 1. Homogeneous Arithmetic & Bitwise (ADD, SUB, XOR)
// - LEGAL:   i32, i64, f32, f64
// - WIDENS:  i1, i8, i16 -> i32 (slot 0)
// - NARROWS: i128, i256  -> i64 (slot 0)
LegalizeQueryResult queryAlu(size_t op1Type, size_t /*op2Type*/, size_t /*op3Type*/)
{
    auto t0 = static_cast<MirTypeCompactId>(op1Type);

    switch (t0)
    {
        case MirTypeCompactId::i32:
        case MirTypeCompactId::i64:
        case MirTypeCompactId::f32:
        case MirTypeCompactId::f64:
            return LegalizeQueryResult{ .m_action = LegalizeAction::Legal,
                                        .m_compactId = static_cast<uint8_t>(t0),
                                        .m_slot = 0,
                                        .m_libcallOffset = 0 };

        case MirTypeCompactId::i1:
        case MirTypeCompactId::i8:
        case MirTypeCompactId::i16:
            return LegalizeQueryResult{ .m_action = LegalizeAction::WidenScalar,
                                        .m_compactId = static_cast<uint8_t>(MirTypeCompactId::i32),
                                        .m_slot = 0,
                                        .m_libcallOffset = 0 };

        case MirTypeCompactId::i128:
        case MirTypeCompactId::i256:
            return LegalizeQueryResult{ .m_action = LegalizeAction::NarrowScalar,
                                        .m_compactId = static_cast<uint8_t>(MirTypeCompactId::i64),
                                        .m_slot = 0,
                                        .m_libcallOffset = 0 };

        default:
            return defaultUnsupportedQuery(op1Type, 0, 0);
    }
}

// 2. Division (DIV, IDIV)
// - LEGAL:   i32
// - WIDENS:  i8, i16 -> i32 (slot 0)
// - LIBCALL: i64     -> __divdi3 (offset 1)
// - NARROWS: i128    -> i64 (slot 0)
LegalizeQueryResult queryDiv(size_t op1Type, size_t /*op2Type*/, size_t /*op3Type*/)
{
    auto t0 = static_cast<MirTypeCompactId>(op1Type);

    switch (t0)
    {
        case MirTypeCompactId::i32:
            return LegalizeQueryResult{ .m_action = LegalizeAction::Legal,
                                        .m_compactId = static_cast<uint8_t>(t0),
                                        .m_slot = 0,
                                        .m_libcallOffset = 0 };

        case MirTypeCompactId::i1:
        case MirTypeCompactId::i8:
        case MirTypeCompactId::i16:
            return LegalizeQueryResult{ .m_action = LegalizeAction::WidenScalar,
                                        .m_compactId = static_cast<uint8_t>(MirTypeCompactId::i32),
                                        .m_slot = 0,
                                        .m_libcallOffset = 0 };

        case MirTypeCompactId::i64:
            return LegalizeQueryResult{
                .m_action = LegalizeAction::Libcall,
                .m_compactId = static_cast<uint8_t>(t0),
                .m_slot = 0,
                .m_libcallOffset = 1 // Offset in string pool: "__divdi3\0"
            };

        case MirTypeCompactId::i128:
            return LegalizeQueryResult{ .m_action = LegalizeAction::NarrowScalar,
                                        .m_compactId = static_cast<uint8_t>(MirTypeCompactId::i64),
                                        .m_slot = 0,
                                        .m_libcallOffset = 0 };

        default:
            return defaultUnsupportedQuery(op1Type, 0, 0);
    }
}

// 3. Comparisons (CMP_EQ, etc.)
// - Condition dest is always i1
// - Check operand 1 (first source operand)
// - LEGAL:   i32, i64, f32, f64
// - WIDENS:  i8, i16 -> i32 (slot 0 triggers widening of input registers)
// - NARROWS: i128    -> i64
LegalizeQueryResult queryCmp(size_t /*dstType*/, size_t srcType, size_t /*src2Type*/)
{
    auto tSrc = static_cast<MirTypeCompactId>(srcType);

    switch (tSrc)
    {
        case MirTypeCompactId::i32:
        case MirTypeCompactId::i64:
        case MirTypeCompactId::f32:
        case MirTypeCompactId::f64:
            return LegalizeQueryResult{ .m_action = LegalizeAction::Legal,
                                        .m_compactId = static_cast<uint8_t>(tSrc),
                                        .m_slot = 0,
                                        .m_libcallOffset = 0 };

        case MirTypeCompactId::i1:
        case MirTypeCompactId::i8:
        case MirTypeCompactId::i16:
            return LegalizeQueryResult{ .m_action = LegalizeAction::WidenScalar,
                                        .m_compactId = static_cast<uint8_t>(MirTypeCompactId::i32),
                                        .m_slot = 0,
                                        .m_libcallOffset = 0 };

        case MirTypeCompactId::i128:
            return LegalizeQueryResult{ .m_action = LegalizeAction::NarrowScalar,
                                        .m_compactId = static_cast<uint8_t>(MirTypeCompactId::i64),
                                        .m_slot = 0,
                                        .m_libcallOffset = 0 };

        default:
            return defaultUnsupportedQuery(srcType, 0, 0);
    }
}

// 4. Heterogeneous Sign Extension (SEXT)
// - LEGAL:   (i32:0, i8:1), (i64:0, i32:1)
// - WIDENS:  (slot 1 is i1) -> widen slot 1 to i8
LegalizeQueryResult querySext(size_t op1Type, size_t op2Type, size_t /*op3Type*/)
{
    auto dstType = static_cast<MirTypeCompactId>(op1Type);
    auto srcType = static_cast<MirTypeCompactId>(op2Type);

    if ((dstType == MirTypeCompactId::i32 && srcType == MirTypeCompactId::i8) ||
        (dstType == MirTypeCompactId::i64 && srcType == MirTypeCompactId::i32))
    {
        return LegalizeQueryResult{ .m_action = LegalizeAction::Legal,
                                    .m_compactId = static_cast<uint8_t>(dstType),
                                    .m_slot = 0,
                                    .m_libcallOffset = 0 };
    }

    if (srcType == MirTypeCompactId::i1)
    {
        return LegalizeQueryResult{ .m_action = LegalizeAction::WidenScalar,
                                    .m_compactId = static_cast<uint8_t>(MirTypeCompactId::i8),
                                    .m_slot = 1,
                                    .m_libcallOffset = 0 };
    }

    return defaultUnsupportedQuery(op1Type, op2Type, 0);
}

// 5. Data Movement (MOV)
// - LEGAL:   Equal types (i32 -> i32, f32 -> f32)
// - BITCAST: i32 <-> f32 (slot 1)
LegalizeQueryResult queryMov(size_t op1Type, size_t op2Type, size_t /*op3Type*/)
{
    auto dstType = static_cast<MirTypeCompactId>(op1Type);
    auto srcType = static_cast<MirTypeCompactId>(op2Type);

    if (dstType == srcType)
    {
        return LegalizeQueryResult{ .m_action = LegalizeAction::Legal,
                                    .m_compactId = static_cast<uint8_t>(dstType),
                                    .m_slot = 0,
                                    .m_libcallOffset = 0 };
    }

    if (dstType == MirTypeCompactId::i32 && srcType == MirTypeCompactId::f32)
    {
        return LegalizeQueryResult{ .m_action = LegalizeAction::Bitcast,
                                    .m_compactId = static_cast<uint8_t>(MirTypeCompactId::i32),
                                    .m_slot = 1,
                                    .m_libcallOffset = 0 };
    }

    return defaultUnsupportedQuery(op1Type, op2Type, 0);
}

// 5. Data Movement (POP_ARG)
// - LEGAL:   i32
LegalizeQueryResult queryPopArg(size_t op1Type, size_t op2Type, size_t /*op3Type*/)
{
    auto dstType = static_cast<MirTypeCompactId>(op1Type);
    auto srcType = static_cast<MirTypeCompactId>(op2Type);

    if (dstType == MirTypeCompactId::__bindToken && srcType == MirTypeCompactId::i32)
    {
        return LegalizeQueryResult{ .m_action = LegalizeAction::Legal,
                                    .m_compactId = 0,
                                    .m_slot = 0,
                                    .m_libcallOffset = 0 };
    }

    return defaultUnsupportedQuery(op1Type, op2Type, 0);
}

// 5. Data Movement (POP_ARG)
// - LEGAL:   bindngtoken
LegalizeQueryResult queryEndArg(size_t op1Type, size_t op2Type, size_t /*op3Type*/)
{
    auto dstType = static_cast<MirTypeCompactId>(op1Type);
    if (dstType == MirTypeCompactId::__bindToken)
    {
        return LegalizeQueryResult{ .m_action = LegalizeAction::Legal,
                                    .m_compactId = 0,
                                    .m_slot = 0,
                                    .m_libcallOffset = 0 };
    }

    return defaultUnsupportedQuery(op1Type, op2Type, 0);
}

} // anonymous namespace

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
    MirLegalizeActionTable *getLegalizeActionTable() { return &m_mockActionTable; }
    MirRegisterAllocator *getRegisterAllocator() override { return nullptr; }
    MirType *getMemOperandDisplacementType() override;
    MirRegisterRef getInstructionPtrReg() const override { return {}; }
    size_t getStackSlotSize() const override { return 8; }
    void initialize() override
    {
        constexpr size_t OpcodeCount = static_cast<size_t>(MirInstructionOpCode::OPCODE_COUNT);
        for (size_t i = 0; i <= OpcodeCount; ++i)
        {
            m_mockActionTable.m_queryTable[static_cast<uint16_t>(i)] = &defaultUnsupportedQuery;
        }

        // Register test opcodes
        m_mockActionTable.m_queryTable[static_cast<uint16_t>(MirInstructionOpCode::ADD)] = &queryAlu;
        m_mockActionTable.m_queryTable[static_cast<uint16_t>(MirInstructionOpCode::SUB)] = &queryAlu;
        m_mockActionTable.m_queryTable[static_cast<uint16_t>(MirInstructionOpCode::XOR)] = &queryAlu;
        m_mockActionTable.m_queryTable[static_cast<uint16_t>(MirInstructionOpCode::DIV)] = &queryDiv;
        m_mockActionTable.m_queryTable[static_cast<uint16_t>(MirInstructionOpCode::IDIV)] = &queryDiv;
        m_mockActionTable.m_queryTable[static_cast<uint16_t>(MirInstructionOpCode::CMP_EQ)] = &queryCmp;
        m_mockActionTable.m_queryTable[static_cast<uint16_t>(MirInstructionOpCode::SEXT)] = &querySext;
        m_mockActionTable.m_queryTable[static_cast<uint16_t>(MirInstructionOpCode::MOV)] = &queryMov;
        m_mockActionTable.m_queryTable[static_cast<uint16_t>(MirInstructionOpCode::POP_ARG)] = &queryPopArg;
        m_mockActionTable.m_queryTable[static_cast<uint16_t>(MirInstructionOpCode::END_ARG)] = &queryEndArg;
        m_mockActionTable.m_queryTable[static_cast<uint16_t>(MirInstructionOpCode::PUSH_RET)] = &queryPopArg;
    }
    std::string_view getLibcallStr(uint8_t symId)
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
    MirLegalizeActionTable m_mockActionTable;
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
