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
#include "Instruction/MirTargetInstructionDesc.h"
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
    // Builds the mock register references and caller/callee-saved register lists.
    MockCallingConvDesc(MirBuilderContext *ctx, MirRegisterClass *gprClass);

    // Assigns an argument to a caller-saved register when available, else a stack slot.
    ArgumentLocationDesc getArgLoc(MirType *type, CallLoweringState *callState) override;
    // Always places the return value in RAX.
    ArgumentLocationDesc getReturnLoc(MirType *type, CallLoweringState *callState) override;
    // Allows register returns for types up to 128 bits; null types are treated as register-returnable.
    bool canReturnInRegs(MirType *type) const override;
    // Uses caller cleanup semantics.
    bool isCalleeCleanup() const override { return false; }
    // Stack grows toward lower addresses.
    bool doesStackGrowsDownwards() const override { return true; }
    // Requests a frame pointer.
    bool hasFramePointer(MirFunction *func) const override { return true; }
    // Human-readable name of the mock calling convention.
    const char *getName() const override { return "MockCallingConv"; }
    // Returns the mock frame pointer register (RBP).
    MirRegisterRef getFramePointerReg() const override { return m_rbpRef; }
    // Returns the mock stack pointer register (RSP).
    MirRegisterRef getStackPointerReg() const override { return m_rspRef; }
    // 16-byte stack alignment, matching the System-V AMD64 ABI.
    size_t getStackAlignment() const override { return 16; }
    // No shadow space is required.
    size_t getShadowSpaceSize() const override { return 0; }
    // ABI classification is a no-op for the mock.
    void classify(MirType *type, std::pmr::vector<CallingConvTypeClass> &out) const override {}

    // Returns all callee-saved registers (RBP, RSP).
    const std::pmr::vector<MirRegisterRef> &getAllCalleeSavedRegs() override { return m_calleeSaved; }
    // Returns the callee-saved registers for any class (class is ignored by the mock).
    const std::pmr::vector<MirRegisterRef> &getCalleeSavedRegs(MirRegisterClass *_class) override
    {
        return m_calleeSaved;
    }
    // Returns all caller-saved registers (argument registers plus RAX).
    const std::pmr::vector<MirRegisterRef> &getAllCallerSavedRegs() override { return m_callerSaved; }
    // Returns the caller-saved registers for any class (class is ignored by the mock).
    const std::pmr::vector<MirRegisterRef> &getCallerSavedRegs(MirRegisterClass *_class) override
    {
        return m_callerSaved;
    }

    // Accessors exposing the individual mock argument/return registers.
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
    // Constructs the mock selector, optionally binding a mock target descriptor.
    explicit MockInstructionSelector(class MockTargetDesc *targetDesc = nullptr) :
        MirInstructionSelector(reinterpret_cast<TargetDesc *>(targetDesc))
    {
    }
    // Rebinds the mock target descriptor used to resolve target instruction descs.
    void setTargetDesc(class MockTargetDesc *targetDesc)
    {
        MirInstructionSelector::setTargetDesc(reinterpret_cast<TargetDesc *>(targetDesc));
    }

    // Selects a target instruction for inst, folding memory operands where possible.
    bool select(MirBuilderContext *ctx, MirInstruction *inst) override;

    // Counters used by tests to assert how many instructions were selected/folded.
    size_t m_selectedCount{ 0 };
    size_t m_foldedCount{ 0 };
};

#include "FrameLowerer/MirFrameLowerer.h"

/**
 * Mock Frame Lowerer.
 */
class MockFrameLowerer : public MirFrameLowerer
{
  public:
    // Records that a prologue was requested.
    void insertPrologue(FrameLowererCtx &ctx) override { m_prologueInserted = true; }
    // Records that an epilogue was requested.
    void insertEpilogue(FrameLowererCtx &ctx) override { m_epilogueInserted = true; }
    // Counts an ALLOC lowering and reports success.
    bool lowerAlloc(FrameLowererCtx &ctx) override
    {
        m_allocLoweredCount++;
        return true;
    }
    // Counts a DALLOC lowering and reports success.
    bool lowerDAlloc(FrameLowererCtx &ctx) override
    {
        m_dallocLoweredCount++;
        return true;
    }

    // Clears all recorded flags and counters between tests.
    void reset()
    {
        m_prologueInserted = false;
        m_epilogueInserted = false;
        m_allocLoweredCount = 0;
        m_dallocLoweredCount = 0;
    }

    // Observed prologue/epilogue insertions and ALLOC/DALLOC lowering counts.
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
    // Counters used by tests to observe spill, reload, and remat activity.
    size_t m_spillCount{ 0 };
    size_t m_reloadCount{ 0 };
    size_t m_rematCount{ 0 };

    // Clears all activity counters between tests.
    void reset()
    {
        m_spillCount = 0;
        m_reloadCount = 0;
        m_rematCount = 0;
    }

  protected:
    // Treats DALLOC instructions as the dynamic allocation marker.
    bool isInstructionDAlloc(MirInstruction *instr) override
    {
        return instr && instr->getOpCode() == MirInstructionOpCode::DALLOC;
    }

    // Treats MOVs with an immediate/float source operand as rematerializable.
    bool isRematerializable(MirRegister *vreg, MirInstruction *definingInst) override
    {
        if (!definingInst)
            return false;

        if (definingInst->getOpCode() == MirInstructionOpCode::MOV && definingInst->getOperands().size() >= 2)
        {
            auto *srcOp = definingInst->getOperands()[1];
            return srcOp &&
                    (srcOp->getType() == MirOperandType::Integer || srcOp->getType() == MirOperandType::FloatingPoint);
        }
        return false;
    }

    // Emits a LOAD from the given spill slot and counts the reload.
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

    // Emits a STORE into the given spill slot and counts the spill.
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

    // Re-emits the defining instruction with the same opcode and operands and counts the remat.
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
    // Populates the legalizer action definitions for the mock target.
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
                .legalFor({ { i8, i1 },
                            { i16, i1 },
                            { i32, i1 },
                            { i64, i1 },
                            { i32, i8 },
                            { i32, i16 },
                            { i64, i8 },
                            { i64, i16 },
                            { i64, i32 } });

        getActionDefinitions(MirInstructionOpCode::SEXT)
                .legalFor({ { i32, i8 }, { i32, i16 }, { i64, i8 }, { i64, i16 }, { i64, i32 } })
                .widenScalarTo(1, { i1 }, i8);

        getActionDefinitions(MirInstructionOpCode::TRUNC)
                .legalFor({ { i8, i32 },
                            { i16, i32 },
                            { i8, i64 },
                            { i16, i64 },
                            { i32, i64 },
                            { i1, i32 },
                            { i1, i64 },
                            { i8, i16 },
                            { i1, i8 },
                            { i1, i16 } });

        // 5. Data Movement & Bitcasts
        getActionDefinitions(MirInstructionOpCode::MOV).legalIfSameType().bitcastBetween(i32, f32);

        // 6. High-Level Procedural Lowerings
        getActionDefinitions(MirInstructionOpCode::CALL).lowerWith(&LegalizeActions::LegalizeCall);

        getActionDefinitions(MirInstructionOpCode::RET).lowerWith(&LegalizeActions::LegalizeReturn);

        // 7. ABI Token Lowering Primitives
        getActionDefinitions({ MirInstructionOpCode::POP_ARG,
                               MirInstructionOpCode::PUSH_ARG,
                               MirInstructionOpCode::PUSH_RET,
                               MirInstructionOpCode::POP_RET })
                .legalFor({ { bindToken, i32 },
                            { bindToken, i64 },
                            { bindToken, f32 },
                            { bindToken, f64 },
                            { bindToken, i8 },
                            { bindToken, i16 } });

        getActionDefinitions(MirInstructionOpCode::END_ARG).legalFor({ bindToken });
    }
};

/**
 * Mock Target Descriptor.
 */
class MockTargetDesc : public TargetDesc
{
  public:
    // Builds the mock register bank/class, calling convention, legalizer, and instruction descs.
    MockTargetDesc(MirBuilderContext *ctx);

    // Name reported by the mock target.
    const char *getName() const override { return "MockTarget"; }
    // Returns the mock frame lowerer.
    MirFrameLowerer *getFrameLowerer() override { return &m_frameLowerer; }
    // Returns the mock instruction selector.
    MirInstructionSelector *getInstructionSelector() override { return &m_isel; }
    // Returns the mock legalizer.
    MirLegalizer *getLegalizer() override { return m_legalizer.get(); }
    // Returns the mock legalizer info table.
    LegalizerInfo *getLegalizerInfo() override { return m_legalizerInfo.get(); }
    // Returns the mock register allocator.
    MirRegisterAllocator *getRegisterAllocator() override { return &m_regAlloc; }
    // Returns the mock register allocator with its test-only counters exposed.
    MockRegisterAllocator *getMockRegisterAllocator() { return &m_regAlloc; }
    // Returns the i64 type used for memory operand displacements.
    MirType *getMemOperandDisplacementType() override;
    // No instruction pointer register is modeled by the mock.
    MirRegisterRef getInstructionPtrReg() const override { return {}; }
    // Stack slots are 8 bytes wide.
    size_t getStackSlotSize() const override { return 8; }
    // Lazily constructs the mock legalizer info from the current type table.
    void initialize() override { m_legalizerInfo = std::make_unique<MockTargetLegalizerInfo>(m_ctx->getTypeTable()); }
    // Maps a libcall symbol id to its runtime helper name for lowering tests.
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
    // No binary descriptors are exposed by the mock.
    std::pmr::vector<TargetBinaryDesc *> getAvailableBinaryDescriptors() override { return {}; }
    // Returns the single mock calling convention.
    std::pmr::vector<CallingConvDesc *> getAvailableCallingConventions() override { return m_convs; }
    // Returns the single mock register bank.
    std::pmr::vector<MirRegisterBank *> getAvailableRegisterBanks() override { return m_banks; }

    // Accessors exposing the mock sub-components to tests.
    MockCallingConvDesc *getMockCallingConv() { return m_mockCc.get(); }
    MockInstructionSelector *getMockInstructionSelector() { return &m_isel; }
    MockFrameLowerer *getMockFrameLowerer() { return &m_frameLowerer; }
    // Returns the mock general-purpose register class.
    MirRegisterClass *getGprClass() override { return m_gprClass; }
    // Returns the addressing mode matcher used for memory folding.
    MirAddressingModeMatcher *getAddressingModeMatcher() override { return &m_modeMatcher; }

    // Accessors for the mock target instruction descriptors.
    MirTargetInstructionDesc *getDescADD64rr() const { return m_descADD64rr.get(); }
    MirTargetInstructionDesc *getDescADD64ri() const { return m_descADD64ri.get(); }
    MirTargetInstructionDesc *getDescADD64rm() const { return m_descADD64rm.get(); }
    MirTargetInstructionDesc *getDescSUB64rr() const { return m_descSUB64rr.get(); }
    MirTargetInstructionDesc *getDescLOAD64() const { return m_descLOAD64.get(); }
    MirTargetInstructionDesc *getDescSTORE64() const { return m_descSTORE64.get(); }
    MirTargetInstructionDesc *getDescBR_COND() const { return m_descBR_COND.get(); }
    MirTargetInstructionDesc *getDescRET() const { return m_descRET.get(); }
    MirTargetInstructionDesc *getDescMOV64rr() const { return m_descMOV64rr.get(); }

  private:
    std::unique_ptr<LegalizerInfo> m_legalizerInfo;
    MockInstructionSelector m_isel;
    X86AddressingModeMatcher m_modeMatcher;
    MockFrameLowerer m_frameLowerer;
    MockRegisterAllocator m_regAlloc;
    std::unique_ptr<MockCallingConvDesc> m_mockCc;
    std::unique_ptr<MirLegalizer> m_legalizer;
    MirBuilderContext *m_ctx;
    MirRegisterClass *m_gprClass{ nullptr };
    MirRegisterBank *m_gprBank{ nullptr };
    std::pmr::vector<MirRegisterBank *> m_banks;
    std::pmr::vector<CallingConvDesc *> m_convs;

    std::unique_ptr<MirTargetInstructionDesc> m_descADD64rr;
    std::unique_ptr<MirTargetInstructionDesc> m_descADD64ri;
    std::unique_ptr<MirTargetInstructionDesc> m_descADD64rm;
    std::unique_ptr<MirTargetInstructionDesc> m_descSUB64rr;
    std::unique_ptr<MirTargetInstructionDesc> m_descLOAD64;
    std::unique_ptr<MirTargetInstructionDesc> m_descSTORE64;
    std::unique_ptr<MirTargetInstructionDesc> m_descBR_COND;
    std::unique_ptr<MirTargetInstructionDesc> m_descRET;
    std::unique_ptr<MirTargetInstructionDesc> m_descMOV64rr;
};

/**
 * Base test harness for EzTriple unit tests.
 */
class EzTripleTestSuite : public ::testing::Test
{
  protected:
    // Builds the type table, builder context, source manager, and mock target before each test.
    void SetUp() override;
    // Releases all fixture-owned compiler objects after each test.
    void TearDown() override;

  public:
    // Returns the MIR builder context shared by the test.
    MirBuilderContext *getBuilderCtx() { return m_builderCtx.get(); }
    // Returns the mock target descriptor under test.
    MockTargetDesc *getTargetDesc() { return m_targetDesc.get(); }
    // Creates a MIR function with the given name and return type (defaults to i32).
    MirFunction *createTestFunction(const std::string_view &name = "test_func", MirType *retType = nullptr);
    // Creates a MIR basic block with the given name inside func.
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
