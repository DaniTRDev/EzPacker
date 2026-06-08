#include <gtest/gtest.h>
#include "MirTestSuite.h"

class FunctionTest : public MirTestSuiteAsGtest
{
  public:
  private:
};

TEST_F(FunctionTest, TestFuncNoParameters)
{
    MirFunction *func = getTestFunc();
    MirFunctionVerifier verifier(func);
    verifier.paramCount(0).stackFrameVerifier().stackFrameObjCount(0);
}

TEST_F(FunctionTest, TestFunc1Parameter)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirOperandBuilder operandBuilder(ctx);

    MirFunctionBuilder builder(ctx);
    builder.buildParam(getTypeTable()->getInt8Type(), nullptr, "testParam");

    MirFunction *func = builder.build(ctx->getTypeTable()->getInt8Type(), nullptr, {}, "myFunc");

    MirFunctionVerifier verifier(func);
    verifier.paramCount(1).stackFrameVerifier().stackFrameObjCount(0);
    verifier.paramType({ ctx->getTypeTable()->getInt8Type()->getId() });
}

TEST_F(FunctionTest, TestFuncNParameters)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirOperandBuilder operandBuilder(ctx);

    MirFunctionBuilder builder(ctx);
    builder.buildParam(getTypeTable()->getInt8Type(), nullptr, "testParam");
    builder.buildParam(getTypeTable()->getInt16Type(), nullptr, "testParam2");
    builder.buildParam(getTypeTable()->getInt32Type(), nullptr, "testParam3");
    builder.buildParam(getTypeTable()->getInt64Type(), nullptr, "testParam4");

    MirFunction *func = builder.build(ctx->getTypeTable()->getInt8Type(), nullptr, {}, "myFunc");

    MirFunctionVerifier verifier(func);
    verifier.paramCount(4).stackFrameVerifier().stackFrameObjCount(0);
    verifier.paramType({ ctx->getTypeTable()->getInt8Type()->getId(),
                         ctx->getTypeTable()->getInt16Type()->getId(),
                         ctx->getTypeTable()->getInt32Type()->getId(),
                         ctx->getTypeTable()->getInt64Type()->getId() });
}

TEST_F(FunctionTest, TestFunc1LocalStackObj)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirOperandBuilder operandBuilder(ctx);

    MirFunctionBuilder builder(ctx);

    MirType *i8 = ctx->getTypeTable()->getInt8Type();
    size_t i8Size = i8->getTotalSizeInBytes();

    MirFunction *func = builder.build(i8, nullptr, {}, "myFunc");
    StackFrameObject *obj = builder.buildLocalStackObj(i8Size, i8Size);

    MirFunctionVerifier verifier(func);
    MirFunctionStackFrameVerifier stackFrameVerifier = verifier.stackFrameVerifier();

    stackFrameVerifier.stackFrameObjCount(1);
    stackFrameVerifier.checkStackFrameObj(obj->m_id, obj->m_offset, obj->m_align, obj->m_sizeInBytes, obj->m_source);
}

TEST_F(FunctionTest, TestFuncNLocalStackObj)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirOperandBuilder operandBuilder(ctx);

    MirFunctionBuilder builder(ctx);

    MirType *i8 = ctx->getTypeTable()->getInt8Type(), *i16 = ctx->getTypeTable()->getInt16Type();
    size_t i8Size = i8->getTotalSizeInBytes(), i16Size = i16->getTotalSizeInBytes();

    MirFunction *func = builder.build(i8, nullptr, {}, "myFunc");
    StackFrameObject *obj = builder.buildLocalStackObj(i8Size, i8Size),
                     *obj2 = builder.buildLocalStackObj(i16Size, i16Size);

    MirFunctionVerifier verifier(func);
    MirFunctionStackFrameVerifier stackFrameVerifier = verifier.stackFrameVerifier();

    stackFrameVerifier.stackFrameObjCount(2);
    stackFrameVerifier.checkStackFrameObj(obj->m_id, obj->m_offset, obj->m_align, obj->m_sizeInBytes, obj->m_source);
    stackFrameVerifier.checkStackFrameObj(obj2->m_id,
                                          obj2->m_offset,
                                          obj2->m_align,
                                          obj2->m_sizeInBytes,
                                          obj2->m_source);
}

TEST_F(FunctionTest, TestFunc1Spill1StackObj)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirOperandBuilder operandBuilder(ctx);

    MirFunctionBuilder builder(ctx);

    MirType *i8 = ctx->getTypeTable()->getInt8Type();
    size_t i8Size = i8->getTotalSizeInBytes();

    MirFunction *func = builder.build(i8, nullptr, {}, "myFunc");
    StackFrameObject *obj = builder.buildStackSpill(i8Size, i8Size);

    MirFunctionVerifier verifier(func);
    MirFunctionStackFrameVerifier stackFrameVerifier = verifier.stackFrameVerifier();

    stackFrameVerifier.stackFrameObjCount(1);
    stackFrameVerifier.checkStackFrameObj(obj->m_id, obj->m_offset, obj->m_align, obj->m_sizeInBytes, obj->m_source);
}

TEST_F(FunctionTest, TestFunc1SpillNStackObj)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirOperandBuilder operandBuilder(ctx);

    MirFunctionBuilder builder(ctx);

    MirType *i8 = ctx->getTypeTable()->getInt8Type(), *i16 = ctx->getTypeTable()->getInt16Type();
    size_t i8Size = i8->getTotalSizeInBytes(), i16Size = i16->getTotalSizeInBytes();

    MirFunction *func = builder.build(i8, nullptr, {}, "myFunc");
    StackFrameObject *obj = builder.buildStackSpill(i8Size, i8Size), *obj2 = builder.buildStackSpill(i16Size, i16Size);

    MirFunctionVerifier verifier(func);
    MirFunctionStackFrameVerifier stackFrameVerifier = verifier.stackFrameVerifier();

    stackFrameVerifier.stackFrameObjCount(2);
    stackFrameVerifier.checkStackFrameObj(obj->m_id, obj->m_offset, obj->m_align, obj->m_sizeInBytes, obj->m_source);
    stackFrameVerifier.checkStackFrameObj(obj2->m_id,
                                          obj2->m_offset,
                                          obj2->m_align,
                                          obj2->m_sizeInBytes,
                                          obj2->m_source);
}

TEST_F(FunctionTest, TestFunc1ParameterNStackObj)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirOperandBuilder operandBuilder(ctx);

    MirFunctionBuilder builder(ctx);

    MirType *i8 = ctx->getTypeTable()->getInt8Type(), *i16 = ctx->getTypeTable()->getInt16Type();
    size_t i8Size = i8->getTotalSizeInBytes(), i16Size = i16->getTotalSizeInBytes();

    MirFunction *func = builder.build(i8, nullptr, {}, "myFunc");
    StackFrameObject *obj = builder.buildStackParam(i8Size, i8Size, 0),
                     *obj2 = builder.buildStackParam(i16Size, i16Size, 2),
                     *obj3 = builder.buildStackParam(i16Size, i16Size, 4);

    MirFunctionVerifier verifier(func);
    MirFunctionStackFrameVerifier stackFrameVerifier = verifier.stackFrameVerifier();

    stackFrameVerifier.stackFrameObjCount(3);
    stackFrameVerifier.checkStackFrameObj(obj->m_id, obj->m_offset, obj->m_align, obj->m_sizeInBytes, obj->m_source);
    stackFrameVerifier.checkStackFrameObj(obj2->m_id,
                                          obj2->m_offset,
                                          obj2->m_align,
                                          obj2->m_sizeInBytes,
                                          obj2->m_source);
    stackFrameVerifier.checkStackFrameObj(obj3->m_id,
                                          obj3->m_offset,
                                          obj3->m_align,
                                          obj3->m_sizeInBytes,
                                          obj3->m_source);
}