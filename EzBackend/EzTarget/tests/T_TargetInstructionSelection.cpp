/**
 * @file T_TargetInstructionSelection.cpp
 * @brief Unit tests for the InstructionSelectorPass and InstructionSelectionTable.
 */
#include <gtest/gtest.h>
#include <EzTarget.h>
#include <EzMir.h>
#include <TargetInstructionSelector/InstructionSelectorPass.h>
#include <TargetInstructionSelector/InstructionSelectionTable.h>
#include <TargetInstructionSelector/InstructionSelectorContext.h>
#include <filesystem>

// Dummy target IDs for testing
constexpr MirTargetInstructionId TARGET_ADD_REG_REG = 100;
constexpr MirTargetInstructionId TARGET_ADD_REG_IMM = 101;
constexpr MirTargetInstructionId TARGET_ADD_REG_REG_8 = 102;
constexpr MirTargetInstructionId TARGET_UNCOND_JUMP = 103;

class InstructionSelectionTests : public ::testing::Test
{
  protected:
    std::shared_ptr<ErrorCollector> ec;
    std::shared_ptr<SourceManager> sm;
    std::shared_ptr<MirEmitterContext> emitterCtx;
    MirEmitter *emitter;

    TargetDesc *targetDesc;
    InstructionSelectionTable *selectionTable;
    InstructionSelectionContext *lowererCtx;
    MirPassManager *passManager;

    void SetUp() override
    {
        ec = std::make_shared<ErrorCollector>();
        sm = std::make_shared<SourceManager>(std::filesystem::current_path());
        emitterCtx = std::make_shared<MirEmitterContext>(ec, sm);
        emitter = new MirEmitter(emitterCtx.get());

        // We don't need a real ABI desc here
        targetDesc = new TargetDesc(nullptr, TargetEndianness::LittleEndian, "DummyTarget");

        selectionTable = new InstructionSelectionTable();

        // Setup some rules
        SelectionRule addRegRegRule;
        addRegRegRule.m_hardwareId = TARGET_ADD_REG_REG;
        addRegRegRule.m_operandKinds = { ExpectedOperandType::Register, ExpectedOperandType::Register };
        selectionTable->addRule(MirInstructionOpCode::ADD, addRegRegRule);

        SelectionRule addRegImmRule;
        addRegImmRule.m_hardwareId = TARGET_ADD_REG_IMM;
        addRegImmRule.m_operandKinds = { ExpectedOperandType::Register, ExpectedOperandType::Integer };
        selectionTable->addRule(MirInstructionOpCode::ADD, addRegImmRule);

        SelectionRule addRegReg8Rule;
        addRegReg8Rule.m_hardwareId = TARGET_ADD_REG_REG_8;
        addRegReg8Rule.m_operandKinds = { ExpectedOperandType::Register, ExpectedOperandType::Register };
        addRegReg8Rule.m_operandSizes = { 1, 1, 1 };
        selectionTable->addRule(MirInstructionOpCode::ADD, addRegReg8Rule);

        SelectionRule jmpRule;
        jmpRule.m_hardwareId = TARGET_UNCOND_JUMP;
        selectionTable->addRule(MirInstructionOpCode::JMP, jmpRule); // No constraints, match all JMP

        lowererCtx = new InstructionSelectionContext(selectionTable, emitter, targetDesc);

        passManager = new MirPassManager();
        passManager->addPass<InstructionSelectorPass>(lowererCtx);

        ec->beginScope();
    }

    void TearDown() override
    {
        ec->endScope(ErrorAction::Discard);
        delete passManager;
        delete lowererCtx;
        delete selectionTable;
        delete targetDesc;
        delete emitter;
    }
};

TEST_F(InstructionSelectionTests, SelectsAddRegReg)
{
    MirType *i64Type = emitterCtx->getIntegerTypeBySize(8);
    if (!i64Type)
    {
        i64Type = emitterCtx->createType(MirTypeKind::Integer, 8, nullptr, "i64");
    }

    MirFunction *func = emitterCtx->createFunction(emitterCtx->getTypes()->getInt8Type(), nullptr, "func");
    MirRegister *dest = emitter->createVirtualRegister(i64Type);
    MirRegister *src1 = emitter->createVirtualRegister(i64Type);

    MirInstruction *addInstr = emitter->emit(MirInstructionOpCode::ADD, { dest, src1 });

    auto instrList = func->getEntryPoint()->getInstructions();
    auto it = instrList->begin();
    bool modified = passManager->run(instrList, it, passManager);

    EXPECT_TRUE(modified);
    EXPECT_EQ(addInstr->getTargetId(), TARGET_ADD_REG_REG);
}

TEST_F(InstructionSelectionTests, SelectsAddRegImm)
{
    MirType *i64Type = emitterCtx->getIntegerTypeBySize(8);
    if (!i64Type)
    {
        i64Type = emitterCtx->createType(MirTypeKind::Integer, 8, nullptr, "i64");
    }

    MirFunction *func = emitterCtx->createFunction(emitterCtx->getTypes()->getInt8Type(), nullptr, "func");
    MirRegister *dest = emitter->createVirtualRegister(i64Type);
    MirInteger *imm = emitter->createImmediateInteger(i64Type, 42);

    MirInstruction *addInstr = emitter->emit(MirInstructionOpCode::ADD, { dest, imm });

    auto instrList = func->getEntryPoint()->getInstructions();
    auto it = instrList->begin();
    bool modified = passManager->run(instrList, it, passManager);

    EXPECT_TRUE(modified);
    EXPECT_EQ(addInstr->getTargetId(), TARGET_ADD_REG_IMM);
}

TEST_F(InstructionSelectionTests, SelectsAddRegReg8WithSizes)
{
    MirType *i8Type = emitterCtx->getIntegerTypeBySize(1);
    if (!i8Type)
    {
        i8Type = emitterCtx->createType(MirTypeKind::Integer, 1, nullptr, "i8");
    }

    MirFunction *func = emitterCtx->createFunction(emitterCtx->getTypes()->getInt8Type(), nullptr, "func");
    MirRegister *dest = emitter->createVirtualRegister(i8Type);
    MirRegister *src1 = emitter->createVirtualRegister(i8Type);

    MirInstruction *addInstr = emitter->emit(MirInstructionOpCode::ADD, { dest, src1 });

    auto instrList = func->getEntryPoint()->getInstructions();
    auto it = instrList->begin();

    bool modified = passManager->run(instrList, it, passManager);

    EXPECT_TRUE(modified);
    EXPECT_EQ(addInstr->getTargetId(), TARGET_ADD_REG_REG); // It hits the first matching rule!
}

TEST_F(InstructionSelectionTests, MatchesSizeConstraintsProperly)
{
    // Let's create a new table and pass to verify size matching specifically without early exit
    InstructionSelectionTable customTable;
    SelectionRule sizeRule;
    sizeRule.m_hardwareId = TARGET_ADD_REG_REG_8;
    sizeRule.m_operandSizes = { 1, 1 };
    customTable.addRule(MirInstructionOpCode::ADD, sizeRule);

    InstructionSelectionContext ctx(&customTable, emitter, targetDesc);
    InstructionSelectorPass customPass(&ctx);

    MirType *i8Type = emitterCtx->getIntegerTypeBySize(1);
    if (!i8Type)
    {
        i8Type = emitterCtx->createType(MirTypeKind::Integer, 1, nullptr, "i8");
    }

    MirFunction *func = emitterCtx->createFunction(emitterCtx->getTypes()->getInt8Type(), nullptr, "func");
    MirRegister *dest = emitter->createVirtualRegister(i8Type);
    MirRegister *src1 = emitter->createVirtualRegister(i8Type);

    MirInstruction *addInstr = emitter->emit(MirInstructionOpCode::ADD, { dest, src1 });

    auto instrList = func->getEntryPoint()->getInstructions();
    auto it = instrList->begin();

    bool modified = customPass.run(instrList, it, passManager);
    EXPECT_TRUE(modified);
    EXPECT_EQ(addInstr->getTargetId(), TARGET_ADD_REG_REG_8);

    // Now test with i64 types. It should FAIL to match the rule.
    MirType *i64Type = emitterCtx->getIntegerTypeBySize(8);
    if (!i64Type)
    {
        i64Type = emitterCtx->createType(MirTypeKind::Integer, 8, nullptr, "i64");
    }

    func = emitterCtx->createFunction(emitterCtx->getTypes()->getInt8Type(), nullptr, "func2");
    MirRegister *dest64 = emitter->createVirtualRegister(i64Type);
    MirRegister *src1_64 = emitter->createVirtualRegister(i64Type);

    MirInstruction *add64Instr = emitter->emit(MirInstructionOpCode::ADD, { dest64, src1_64 });
    auto instrList64 = func->getEntryPoint()->getInstructions();
    auto it64 = instrList64->begin();

    modified = customPass.run(instrList64, it64, passManager);
    EXPECT_FALSE(modified); // Should fail and emit error, not modify
    EXPECT_EQ(add64Instr->getTargetId(), TARGET_INSTR_SELECT_NONE);
    EXPECT_TRUE(ec->doesCurrentScopeHasFatalErrors()); // Error should be collected
}

TEST_F(InstructionSelectionTests, UnconditionalMatch)
{
    MirFunction *func = emitterCtx->createFunction(emitterCtx->getTypes()->getInt8Type(), nullptr, "testFunc");
    MirBlock *targetBlock = emitterCtx->createBlock();

    MirReference *blockRef = emitter->createBlockRef(targetBlock);
    MirInstruction *jmpInstr = emitter->emit(MirInstructionOpCode::JMP, { blockRef });

    auto instrList = func->getEntryPoint()->getInstructions();
    auto it = instrList->begin();
    bool modified = passManager->run(instrList, it, passManager);

    EXPECT_TRUE(modified);
    EXPECT_EQ(jmpInstr->getTargetId(), TARGET_UNCOND_JUMP);
}

TEST_F(InstructionSelectionTests, SkipsAlreadySelectedInstructions)
{
    MirType *i64Type = emitterCtx->getIntegerTypeBySize(8);
    if (!i64Type)
    {
        i64Type = emitterCtx->createType(MirTypeKind::Integer, 8, nullptr, "i64");
    }

    MirFunction *func = emitterCtx->createFunction(emitterCtx->getTypes()->getInt8Type(), nullptr, "testFunc");
    MirRegister *dest = emitter->createVirtualRegister(i64Type);
    MirRegister *src1 = emitter->createVirtualRegister(i64Type);

    MirInstruction *addInstr = emitter->emit(MirInstructionOpCode::ADD, { dest, src1 });
    addInstr->setTargetId(999); // Manually lowered

    auto instrList = func->getEntryPoint()->getInstructions();
    auto it = instrList->begin();
    bool modified = passManager->run(instrList, it, passManager);

    EXPECT_FALSE(modified);
    EXPECT_EQ(addInstr->getTargetId(), 999);
}

TEST_F(InstructionSelectionTests, FailsOnUnknownInstruction)
{
    MirType *i64Type = emitterCtx->getIntegerTypeBySize(8);
    if (!i64Type)
    {
        i64Type = emitterCtx->createType(MirTypeKind::Integer, 8, nullptr, "i64");
    }

    MirFunction *func = emitterCtx->createFunction(emitterCtx->getTypes()->getInt8Type(), nullptr, "testFunc");
    MirRegister *dest = emitter->createVirtualRegister(i64Type);
    MirRegister *src1 = emitter->createVirtualRegister(i64Type);

    // MUL is not in our table
    MirInstruction *mulInstr = emitter->emit(MirInstructionOpCode::MUL, { dest, src1 });

    auto instrList = func->getEntryPoint()->getInstructions();
    auto it = instrList->begin();
    bool modified = passManager->run(instrList, it, passManager);

    EXPECT_FALSE(modified);
    EXPECT_EQ(mulInstr->getTargetId(), TARGET_INSTR_SELECT_NONE);
    EXPECT_TRUE(ec->doesCurrentScopeHasFatalErrors());
}

TEST_F(InstructionSelectionTests, FailsOnMismatchedOperandCount)
{
    InstructionSelectionTable customTable;
    SelectionRule rule;
    rule.m_hardwareId = 123;
    rule.m_operandKinds = { ExpectedOperandType::Register }; // Only 1 operand
    customTable.addRule(MirInstructionOpCode::ADD, rule);

    InstructionSelectionContext ctx(&customTable, emitter, targetDesc);
    InstructionSelectorPass customPass(&ctx);

    MirType *i64Type = emitterCtx->getIntegerTypeBySize(8);
    if (!i64Type)
    {
        i64Type = emitterCtx->createType(MirTypeKind::Integer, 8, nullptr, "i64");
    }

    MirFunction *func = emitterCtx->createFunction(emitterCtx->getTypes()->getInt8Type(), nullptr, "testFunc");
    MirRegister *dest = emitter->createVirtualRegister(i64Type);
    MirRegister *src1 = emitter->createVirtualRegister(i64Type);

    // ADD has 2 operands
    MirInstruction *addInstr = emitter->emit(MirInstructionOpCode::ADD, { dest, src1 });

    auto instrList = func->getEntryPoint()->getInstructions();
    auto it = instrList->begin();
    bool modified = customPass.run(instrList, it, passManager);

    EXPECT_FALSE(modified);
    EXPECT_EQ(addInstr->getTargetId(), TARGET_INSTR_SELECT_NONE);
    EXPECT_TRUE(ec->doesCurrentScopeHasFatalErrors());
}
