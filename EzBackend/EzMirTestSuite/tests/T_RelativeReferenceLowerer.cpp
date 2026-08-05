#include <gtest/gtest.h>
#include "../include/EzMirTestSuite.h"

class TestRelativeReferenceLowerer : public MirTestSuiteAsGtest
{
  public:
};

TEST_F(TestRelativeReferenceLowerer, TestConstantArrayElementLowering)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirTypeTable *typeTable = getTypeTable();
    MirOperandBuilder operandBuilder(ctx);

    // 1. Fetch the default managed test function entry block point from the base suite
    MirBlock *entryPoint = getTestFunc()->getEntryPoint();
    size_t entryPointId = entryPoint->getId();

    // 2. Set up structural types (e.g., an array holding 32-bit integers)
    MirType *i32Type = typeTable->i32();
    MirType *arrayShapeType = typeTable->getArray(i32Type, 8);
    MirType *ptrToArray = typeTable->getPtr(arrayShapeType);

    // Simulated virtual base register point
    MirRegister *arrayBaseVReg = operandBuilder.buildVReg(ptrToArray, "local_arr_ptr");

    // Create a high-level symbolic array reference targeting element slot index 3
    MirOperand *symbolicArrayRef = operandBuilder.build<MirReference>(i32Type,
                                                                      MirReferenceType::ConstantArrayElement,
                                                                      arrayBaseVReg->getRegId(),
                                                                      3,
                                                                      nullptr);

    // Build instruction payload attached to the managed block flow
    MirInstructionInsertionPoint ip{ .m_type = InsertionType::InsertAfter, .m_block = entryPoint };
    MirInstructionBuilder instrBuilder(ctx, ip);

    MirRegister *destReg = operandBuilder.buildVReg(i32Type, "loaded_val");
    instrBuilder.LOAD(destReg, symbolicArrayRef);

    // 3. Execute the pass using the test suite's native runPass framework
    RelativeReferenceLowererPass *pass = runPass<RelativeReferenceLowererPass>(ctx);
    RelativeReferenceLowererVerifier verifier(pass, ctx);

    // 4. Fluent verifier assertions check lower transformations perfectly
    verifier.executed().succeeded().mirModified().verifyInstruction(
            getTestFunc(),
            entryPointId,
            0,
            [&](MirInstructionVerifier &instr)
            {
                instr.opcode(MirInstructionOpCode::LOAD).operandCount(2);

                // Offset Verification: Element Index 3 * sizeof(i32)[4 bytes] = 12 bytes byte offset
                // Verify that the High-Level reference operand was correctly changed into a flat Memory operand
                instr.operandVerifier(1)
                        .type(MirOperandType::Memory)
                        .verifyMemory(i32Type,
                                      arrayBaseVReg,
                                      operandBuilder.buildInt(typeTable->i64(), FlexInt(int64_t(12))));
            });
}

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

    // Manually update offsets. This is done by ClassOffsetMapperPass, but we are unit-testing other pass.
    // Offset logic: Field m_id (4 bytes) -> m_score offset = 8 (due to f64 alignment)
    playerClass->getFieldByName("m_id")->m_offset = 8;
    playerClass->getFieldByName("m_score")->m_offset = 8;

    // 2. Setup instruction: Load f64 score = player_ptr->m_score
    MirBlock *entry = getTestFunc()->getEntryPoint();
    MirInstructionBuilder ib(ctx, { InsertionType::InsertAfter, entry });

    MirRegister *ptrReg = opBuilder.buildVReg(ptrToPlayer, "player_ptr");
    MirOperand *fieldRef = opBuilder.buildRef(ptrReg, playerClass->getFieldByName("m_score"));
    MirRegister *destReg = opBuilder.buildVReg(types->f64(), "loaded_score");

    ib.LOAD(destReg, fieldRef);

    // 3. Run Lowerer
    RelativeReferenceLowererPass *pass = runPass<RelativeReferenceLowererPass>(ctx);

    // 4. Verify memory lowering
    RelativeReferenceLowererVerifier(pass, ctx).executed().succeeded().mirModified().verifyInstruction(
            getTestFunc(),
            entry->getId(),
            0,
            [&](MirInstructionVerifier &instr)
            {
                // Offset logic: Field m_id (4 bytes) -> m_score offset = 8 (due to f64 alignment)
                int64_t expectedOffset = playerClass->getFieldByName("m_score")->m_offset;
                instr.operandVerifier(1)
                        .type(MirOperandType::Memory)
                        .verifyMemory(types->f64(), ptrReg, opBuilder.buildInt(types->i64(), FlexInt(expectedOffset)));
            });
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

    // Manually update offsets. This is done by ClassOffsetMapperPass, but we are unit-testing other pass.
    vClass->getMethodBySignature(types->getVoidType(), {}, "doAction")->m_offset = 0;

    // 2. Setup instruction: Call [ptr + VTableOffset]
    MirBlock *entry = getTestFunc()->getEntryPoint();
    MirInstructionBuilder ib(ctx, { InsertionType::InsertAfter, entry });

    MirRegister *ptrReg = opBuilder.buildVReg(ptrToV, "vclass_ptr");
    MirOperand *methodRef =
            opBuilder.buildRef(ptrReg, vClass->getMethodBySignature(types->getVoidType(), {}, "doAction"));

    ib.CALL(methodRef);

    // 3. Run Lowerer
    RelativeReferenceLowererPass *pass = runPass<RelativeReferenceLowererPass>(ctx);

    // 4. Verify lowering to function pointer memory access
    MirType *ptrToFunc = types->getPtr(method->getType());
    RelativeReferenceLowererVerifier(pass, ctx).executed().succeeded().mirModified().verifyInstruction(
            getTestFunc(),
            entry->getId(),
            0,
            [&](MirInstructionVerifier &instr)
            {
                int64_t vTableOffset = vClass->getMethodBySignature(types->getVoidType(), {}, "doAction")->m_offset;
                instr.operandVerifier(0)
                        .type(MirOperandType::Memory)
                        .verifyMemory(ptrToFunc, ptrReg, opBuilder.buildInt(types->i64(), FlexInt(vTableOffset)));
            });
}