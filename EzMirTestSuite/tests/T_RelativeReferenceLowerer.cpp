#include "EzMirTestSuite.h"
#include "Block/MirBlock.h"
#include "Class/MirClass.h"
#include "Class/MirClassBuilder.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionBuilder.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Type/MirTypeTable.h"
#include "MirPasses/Passes/RelativeReferenceLowererPass.h"

class TestRelativeReferenceLowerer : public MirTestSuiteAsGtest
{
  public:
  protected:
};

namespace
{

::testing::AssertionResult PassSucceededAndModified(MirPass *pass)
{
    if (!pass)
        return ::testing::AssertionFailure() << "Pass is nullptr";
    if (!pass->getResult())
        return ::testing::AssertionFailure() << "Pass result is nullptr";

    if (!pass->getResult()->m_executed)
        return ::testing::AssertionFailure() << "Pass was not marked as executed";
    if (!pass->getResult()->m_succeeded)
        return ::testing::AssertionFailure() << "Pass failed during execution";
    if (!pass->getResult()->m_modifiedMir)
        return ::testing::AssertionFailure() << "Pass did not modify the MIR as expected";

    return ::testing::AssertionSuccess();
}

::testing::AssertionResult
IsInstruction(MirInstruction *instr, MirInstructionOpCode expectedOpcode, size_t expectedOperandCount)
{
    if (!instr)
        return ::testing::AssertionFailure() << "Instruction is nullptr";

    if (instr->getOpCode() != expectedOpcode)
        return ::testing::AssertionFailure() << "Expected opcode " << static_cast<int>(expectedOpcode) << ", got "
                                             << static_cast<int>(instr->getOpCode());

    if (instr->getOperands().size() != expectedOperandCount)
        return ::testing::AssertionFailure()
                << "Expected " << expectedOperandCount << " operands, got " << instr->getOperands().size();

    return ::testing::AssertionSuccess();
}

::testing::AssertionResult
IsMemoryOperand(MirOperand *op, MirType *expectedType, MirRegister *expectedBase, int64_t expectedDispl)
{
    if (!op)
        return ::testing::AssertionFailure() << "Operand is nullptr";

    if (op->getType() != MirOperandType::Memory)
        return ::testing::AssertionFailure() << "Expected Memory operand, got type " << static_cast<int>(op->getType());

    if (op->getMirType() != expectedType)
        return ::testing::AssertionFailure()
                << "Expected operand MIR type " << expectedType << ", got " << op->getMirType();

    MirMemory *mem = static_cast<MirMemory *>(op);

    if (mem->getBase() != expectedBase)
        return ::testing::AssertionFailure() << "Base register mismatch";

    MirInteger *displ = mem->getDisplacement();
    if (!displ)
    {
        if (expectedDispl != 0)
            return ::testing::AssertionFailure() << "Expected displacement " << expectedDispl << ", got none";
    }
    else
    {
        if (displ->getValue().getI64() != expectedDispl)
            return ::testing::AssertionFailure()
                    << "Displacement mismatch: " << displ->getValue().getI64() << " != " << expectedDispl;
    }

    return ::testing::AssertionSuccess();
}

} // anonymous namespace

TEST_F(TestRelativeReferenceLowerer, TestClassFieldReferenceLowering)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirTypeTable *types = getTypeTable();
    MirOperandBuilder opBuilder(ctx);

    // 1. Construct a class: Player { i32 m_id, f64 m_score }
    MirClassBuilder cBuilder(ctx);
    cBuilder.appendField(types->i32(), "m_id");
    cBuilder.appendField(types->f64(), "m_score");
    MirClass *playerClass = cBuilder.build(nullptr, "Player");
    MirType *ptrToPlayer = types->getPtr(playerClass->getType());

    // Manually update offsets (simulating ClassOffsetMapperPass logic)
    // Field m_id (4 bytes) -> m_score offset = 8 (due to f64 alignment)
    playerClass->getFieldByName("m_id")->m_offset = 0;
    playerClass->getFieldByName("m_score")->m_offset = 8;
    int64_t expectedOffset = 8;

    // 2. Setup instruction: Load f64 score = player_ptr->m_score
    MirBlock *entry = getTestFunc()->getEntryPoint();
    MirInstructionBuilder ib(ctx, { InsertionType::InsertAfter, entry, entry->begin() });

    MirRegister *ptrReg = opBuilder.buildVReg(ptrToPlayer, "player_ptr");
    MirReference *fieldRef = opBuilder.buildRef(ptrReg, playerClass->getFieldByName("m_score"));
    MirRegister *destReg = opBuilder.buildVReg(types->f64(), "loaded_score");

    ib.ALLOC(ptrReg);
    ib.LOAD(destReg, fieldRef);

    // 3. Run Lowerer
    RelativeReferenceLowererPass *pass = runPass<RelativeReferenceLowererPass>(ctx);

    // 4. Verify memory lowering
    EXPECT_TRUE(PassSucceededAndModified(pass));

    ASSERT_GT(entry->getInstrCount(), 0);
    auto it = entry->begin();

    // ALLOC is the first instruction
    ASSERT_TRUE(IsInstruction(*it, MirInstructionOpCode::ALLOC, 1));
    ++it;

    // LOAD takes 2 operands
    ASSERT_TRUE(IsInstruction(*it, MirInstructionOpCode::LOAD, 2));

    // The reference operand is index 1. Verify it became a memory address at offset 8.
    EXPECT_TRUE(IsMemoryOperand((*it)->getOperands()[1], types->f64(), ptrReg, expectedOffset));
}

TEST_F(TestRelativeReferenceLowerer, TestClassMethodReferenceLowering)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirTypeTable *types = getTypeTable();
    MirOperandBuilder opBuilder(ctx);
    MirFunctionBuilder fBuilder(ctx);

    // 1. Construct class with virtual method
    MirFunction *method = fBuilder.build(types->getVoidType(), "doAction");
    MirClassBuilder cBuilder(ctx);
    cBuilder.appendMethod(method);
    MirClass *vClass = cBuilder.build(nullptr, "VClass");
    MirType *ptrToV = types->getPtr(vClass->getType());

    // Manually set offset (simulating ClassOffsetMapperPass)
    vClass->getMethodBySignature(types->getVoidType(), {}, "doAction")->m_offset = 0;
    int64_t expectedVTableOffset = 0;

    // 2. Setup instruction: Call [ptr + VTableOffset]
    MirBlock *entry = getTestFunc()->getEntryPoint();
    MirInstructionBuilder ib(ctx, { InsertionType::InsertAfter, entry, entry->begin() });

    MirRegister *ptrReg = opBuilder.buildVReg(ptrToV, "vclass_ptr");
    MirReference *methodRef =
            opBuilder.buildRef(ptrReg, vClass->getMethodBySignature(types->getVoidType(), {}, "doAction"));

    // As per instruction constraints: CALL requires a Destination Register (Write) and a Target (Read).
    // Even if returning void, a dummy register captures the structural constraint.
    MirRegister *destReg = opBuilder.buildVReg(types->i32(), "call_result");

    ib.ALLOC(ptrReg);
    ib.CALL(destReg, methodRef);

    // 3. Run Lowerer
    RelativeReferenceLowererPass *pass = runPass<RelativeReferenceLowererPass>(ctx);

    // 4. Verify lowering to function pointer memory access
    EXPECT_TRUE(PassSucceededAndModified(pass));

    ASSERT_GT(entry->getInstrCount(), 0);
    auto it = entry->begin();
    MirType *ptrToFunc = types->getPtr(method->getType());

    // ALLOC is the first instruction
    ASSERT_TRUE(IsInstruction(*it, MirInstructionOpCode::ALLOC, 1));
    ++it;

    // CALL takes 2 operands: Destination(Write), Target(Read)
    ASSERT_TRUE(IsInstruction(*it, MirInstructionOpCode::CALL, 2));

    // The target address is at index 1.
    EXPECT_TRUE(IsMemoryOperand((*it)->getOperands()[1], ptrToFunc, ptrReg, expectedVTableOffset));
}

TEST_F(TestRelativeReferenceLowerer, TestClassMethodReferenceLoweringOffset16)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirTypeTable *types = getTypeTable();
    MirOperandBuilder opBuilder(ctx);
    MirFunctionBuilder fBuilder(ctx);

    // 1. Construct class with virtual method
    MirFunction *method = fBuilder.build(types->getVoidType(), "doActionSecondary");
    MirClassBuilder cBuilder(ctx);

    // We append the method. To simulate a deeper vtable index, we manually force the offset to 16.
    cBuilder.appendMethod(method);
    MirClass *vClass = cBuilder.build(nullptr, "VClassOffset");
    MirType *ptrToV = types->getPtr(vClass->getType());

    // Manually set offset to 16 (e.g. simulating a 3rd method in a 64-bit vtable)
    vClass->getMethodBySignature(types->getVoidType(), {}, "doActionSecondary")->m_offset = 16;
    int64_t expectedVTableOffset = 16;

    // 2. Setup instruction: Call [ptr + VTableOffset]
    MirBlock *entry = getTestFunc()->getEntryPoint();
    MirInstructionBuilder ib(ctx, { InsertionType::InsertAfter, entry, entry->begin() });

    MirRegister *ptrReg = opBuilder.buildVReg(ptrToV, "vclass_ptr");
    MirReference *methodRef =
            opBuilder.buildRef(ptrReg, vClass->getMethodBySignature(types->getVoidType(), {}, "doActionSecondary"));

    MirRegister *destReg = opBuilder.buildVReg(types->i32(), "call_result");

    ib.ALLOC(ptrReg);
    ib.CALL(destReg, methodRef);

    // 3. Run Lowerer
    RelativeReferenceLowererPass *pass = runPass<RelativeReferenceLowererPass>(ctx);

    // 4. Verify lowering to function pointer memory access at offset 16
    EXPECT_TRUE(PassSucceededAndModified(pass));

    ASSERT_GT(entry->getInstrCount(), 0);
    auto it = entry->begin();
    MirType *ptrToFunc = types->getPtr(method->getType());

    // ALLOC is the first instruction
    ASSERT_TRUE(IsInstruction(*it, MirInstructionOpCode::ALLOC, 1));
    ++it;

    // CALL takes 2 operands: Destination(Write), Target(Read)
    ASSERT_TRUE(IsInstruction(*it, MirInstructionOpCode::CALL, 2));

    // The target address is at index 1. Expecting displacement of 16.
    EXPECT_TRUE(IsMemoryOperand((*it)->getOperands()[1], ptrToFunc, ptrReg, expectedVTableOffset));
}